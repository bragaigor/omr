/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include <string.h>
#if defined(OSX)
#include <sys/mman.h>
#include <sys/errno.h>
#endif /* defined(OSX) */

#include "omrcomp.h"
#include "omrport.h"
#include "omr.h"

#include "EnvironmentBase.hpp"
#include "Forge.hpp"
#include "GCExtensionsBase.hpp"
#include "Math.hpp"
#include "Heap.hpp"
#include "HeapRegionManager.hpp"
#include "ModronAssertions.h"
#include "SparseVirtualMemory.hpp"
#include "SparseOrderedFixedSizePool.hpp"

#define OMRVMEM_DEBUG

/****************************************
 * Initialization
 ****************************************
 */

MM_SparseVirtualMemory*
MM_SparseVirtualMemory::newInstance(MM_EnvironmentBase* env, uint32_t memoryCategory, MM_Heap *in_heap)
{
	printf("Inside MM_SparseVirtualMemory::newInstance\n");
	MM_SparseVirtualMemory* vmem = NULL;
	vmem = (MM_SparseVirtualMemory*)env->getForge()->allocate(sizeof(MM_SparseVirtualMemory), OMR::GC::AllocationCategory::FIXED, OMR_GET_CALLSITE());

	if (vmem) {
		new (vmem) MM_SparseVirtualMemory(env, in_heap->getPageSize(), in_heap);
		if (!vmem->initialize(env, memoryCategory)) {
			vmem->kill(env);
			vmem = NULL;
		}
	}

	return vmem;
}

bool
MM_SparseVirtualMemory::initialize(MM_EnvironmentBase* env, uint32_t memoryCategory)
{
	uintptr_t in_heap_size = (uintptr_t)_heap->getHeapTop() - (uintptr_t)_heap->getHeapBase();
	uintptr_t maxHeapSize = _heap->getMaximumMemorySize();
	printf("getHeapTop: %zu, getHeapBase: %zu, in_heap_size: %zu, maxHeapSize: %zu\n", (uintptr_t)_heap->getHeapTop(), (uintptr_t)_heap->getHeapBase(), in_heap_size, maxHeapSize);
	uintptr_t regionSize = _heap->getHeapRegionManager()->getRegionSize();
	uintptr_t regionCount = in_heap_size / regionSize;

	// TODO: This must be ceil log, or should we just keep this way?
	uintptr_t ceilLog2 = MM_Math::floorLog2(regionCount) + 1;

	uintptr_t off_heap_size = (uintptr_t)((ceilLog2 * in_heap_size) / 2.0);
	printf("Inisde MM_SparseVirtualMemory::initialize.. in_heap_size: %zu, region size: %zu, num of region: %zu, attempting off-heap size: %zu\n", in_heap_size, regionSize, regionCount, off_heap_size);
#if !defined(J9ZOS390)
	Assert_MM_true(regionSize % _pageSize == 0);
#endif
	bool ret = MM_VirtualMemory::initialize(env, off_heap_size, NULL, NULL, 0, memoryCategory);

	if (ret) {
		void *sparseHeapBase = getHeapBase();
		_sparsePool = MM_SparseOrderedFixedSizePool::newInstance(env, sparseHeapBase, off_heap_size);
		if (NULL == _sparsePool) {
			ret = false;
		}
	}

	return ret;
}

void *
MM_SparseVirtualMemory::getAddressForData(void *proxyObjPtr, uintptr_t size)
{
	/* Commiting and decommiting memory sizes must be multiple of pagesize */
	uintptr_t adjustedSize = MM_Math::roundToCeiling(_pageSize, size);
	void *sparseHeapAddr = _sparsePool->findFreeEntry(adjustedSize);
	if (NULL != sparseHeapAddr) {
		_sparsePool->rememberObjectData(sparseHeapAddr, proxyObjPtr, adjustedSize);
	} else {
		/* Impossible to get here!! */
		printf("ERROR: size: %zu, there should always be free space at sparse heap!!!!\n", size);
		Assert_MM_unreachable();
	}

	bool ret = MM_VirtualMemory::commitMemory(sparseHeapAddr, adjustedSize);

	printf("Inside MM_SparseVirtualMemory::getAddressForData. original size: %zu, adjustedSize: %zu, sparseHeapAddr: %p, ret: %d\n", size, adjustedSize, sparseHeapAddr, (int)ret);

	if (!ret) {
		// TODO: Trace point?
#if defined(OMRVMEM_DEBUG)
		printf("Failed to commit memory! size: %zu\n", size);
		fflush(stdout);
#endif
		sparseHeapAddr = NULL;
	}

	return sparseHeapAddr;
}

bool
MM_SparseVirtualMemory::decommitMemory(MM_EnvironmentBase* env, void* address, uintptr_t size)
{
	bool ret = false;
#if defined(OSX)
	ret = decommitOSXMemory(env, address, size);
#else /* defined(OSX) */
	void *highValidAddress = (void *)((uintptr_t)address + size);
	ret = MM_VirtualMemory::decommitMemory(address, size, address, highValidAddress);
#endif /* defined(OSX) */

	return ret;
}

bool
MM_SparseVirtualMemory::removeObjFromPoolAndFreeSparseRegion(MM_EnvironmentBase* env, void *dataPtr)
{
	uintptr_t dataSize = _sparsePool->getDataSizeForDataPtr(dataPtr);
	bool ret = false;

	if ((NULL != dataPtr) && (0 != dataSize)) {

		Assert_MM_true(0 == (dataSize % _pageSize));
		ret = decommitMemory(env, dataPtr, dataSize);
		if (ret) {
			_sparsePool->returnEntryToFreeList(dataPtr, dataSize);
			_sparsePool->removeEntryFromTable(dataPtr);
		} else {
			/* TODO: Assert Fatal in case of failure? */
#if defined(OMRVMEM_DEBUG)
			printf("ERROR: Failed to decommit memory. dataPtr: %p, dataSize: %zu\n", dataPtr, dataSize);
			fflush(stdout);
#endif
			Assert_MM_true(false);
		}
		printf("Returned address: %p, with dataPtr: %p and size: %zu back to pool\n", dataPtr, dataPtr, dataSize);
	}

	return ret;
}

#if defined(OSX)
void
MM_SparseVirtualMemory::recordDoubleMapIdentifier(void *dataPtr, struct J9PortVmemIdentifier *identifier)
{
	_sparsePool->recordDoubleMapIdentifier(dataPtr, identifier);
}
#endif /* defined(OSX) */

bool
MM_SparseVirtualMemory::updateCopiedObject(void *dataPtr, void *objPtr)
{
	return _sparsePool->updateCopiedObject(dataPtr, objPtr);
}

// TODO: Should this be moved to port library?
#if defined(OSX)
/* Should this be somewhere in port library instead? */
bool
MM_SparseVirtualMemory::decommitOSXMemory(MM_EnvironmentBase* env, void *dataPtr, uintptr_t dataSize)
{
	int rc = 0;
	OMRPORT_ACCESS_FROM_OMRPORT(env->getPortLibrary());
	struct J9PortVmemIdentifier *identifier = _sparsePool->getIdentifierFromDataPtr(dataPtr);

	if (0 == identifier->size || NULL == identifier->address) {
		printf("___-----___----- Maybe this address was already returned to the pool!!! dataPtr: %p, dataSize: %zu\n", dataPtr, dataSize);
		Assert_MM_unreachable();
	} else {
		printf("------------ dataPtr: %p, identifier->address: %p, dataSize: %zu, identifier->size: %zu\n", dataPtr, identifier->address, dataSize, identifier->size);
		Assert_GC_true_with_message4(env, ((identifier->size == dataSize) && (identifier->address == dataPtr)),
			"dataPtr: %p, identifier->address: %p, dataSize: %zu, identifier->size: %zu\n", dataPtr, identifier->address, dataSize, identifier->size);
		Assert_MM_true((getHeapBase() <= dataPtr) && (getHeapTop() > dataPtr));

		rc = omrvmem_release_double_mapped_region(dataPtr, dataSize, identifier);

		if (-1 == rc) {
			printf("omrvmem_release_double_mapped_region returned -1!!!!!! Failed trying to release double mapped region!!!\n");
		}
	}
	return -1 != rc;
}
#endif /* defined(OSX) */
