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

/**
 * @file
 * @ingroup GC_Base_Core
 */

#if !defined(SPARSEVIRTUALMEMORY_HPP_)
#define SPARSEVIRTUALMEMORY_HPP_

#include "omrcomp.h"
#include "omrport.h"
#include "modronbase.h"
#include "modronopt.h"

#include "BaseVirtual.hpp"
#include "VirtualMemory.hpp"

class MM_GCExtensions;
class MM_GCExtensionsBase;
class MM_Heap;
class MM_SparseOrderedFixedSizePool;
struct J9PortVmemParams;

/**
 * @todo Provide class documentation
 * @ingroup GC_Base_Core
 */
class MM_SparseVirtualMemory : public MM_VirtualMemory {
/*
 * Data members
 */
private:
	MM_Heap *_heap; /**< reference to in-heap */
	MM_SparseOrderedFixedSizePool *_sparsePool; /**< Structure that manages data and free region of sparse virtual memory */
protected:
public:
/*
 * Function members
 */
private:

	bool initialize(MM_EnvironmentBase* env, uint32_t memoryCategory);
#if defined(OSX)
	/**
	 * OSX is too lazy when it comes to releasing memory. Simply calling msync, or
	 * madvise is not enough to decommit memory. Therefore, we need to force the
	 * OS to return the pages to the OS, and we do so by mmaping the region in
	 * interest.
	 *
	 * @param dataSize	uintptr_t	size of region to be mmaped
	 * @param dataPtr	dataPtr		Region location that'll be mmaped
	 *
	 * @return true if sparse region was successfully mmaped, false otherwise
	 */
	bool decommitOSXMemory(MM_EnvironmentBase* env, void *dataPtr, uintptr_t dataSize);
#endif /* defined(OSX) */

protected:

	// TODO: Fix heap alignment param
	MM_SparseVirtualMemory(MM_EnvironmentBase* env, uintptr_t pageSize, MM_Heap *in_heap)
		: MM_VirtualMemory(env, env->getExtensions()->heapAlignment, pageSize, env->getExtensions()->requestedPageFlags, 0, OMRPORT_VMEM_MEMORY_MODE_READ | OMRPORT_VMEM_MEMORY_MODE_WRITE)
		, _heap(in_heap)
		, _sparsePool(NULL)
	{
		_typeId = __FUNCTION__;
	}

public:

	static MM_SparseVirtualMemory* newInstance(MM_EnvironmentBase* env, uint32_t memoryCategory, MM_Heap *in_heap);

	/* TODO: DELETE */
	bool updateCopiedObject(void *srcObj, void *dstObj);

#if defined(OSX)
	void recordDoubleMapIdentifier(void *dataPtr, struct J9PortVmemIdentifier *identifier);
#endif /* defined(OSX) */

	/**
	 * Find free space at sparse heap address space that satisfy size
	 *
	 * @param size		uintptr_t	size requested by object pointer
	 * @param proxyObjPtr	void*		Proxy object that will be associated to data at sparse heap
	 *
	 * @return data pointer at sparse heap that satisfies requested size
	 */
	void *getAddressForData(void *proxyObjPtr, uintptr_t size);

	/**
	 * Once object is collected by GC, we need to free the sparse region associated
	 * with the object pointer. Therefore we decommit sparse region and return free
	 * region to sparse free region pool.
	 *
	 * @param dataPtr	void*		Data pointer
	 * @return true if region associated to object was decommited and freed successfully, false otherwise
	 */
	bool removeObjFromPoolAndFreeSparseRegion(MM_EnvironmentBase* env, void *dataPtr);

	/**
	 * Decommits/Releases memory, returning the associated pages to the OS
	 *
	 * @param address	void*		Address to be decommited
	 * @param size		uintptr_t	Size of region to be decommited
	 *
	 * @return true if memory was successfully decommited, false otherwise
	 */
	virtual bool decommitMemory(MM_EnvironmentBase* env, void* address, uintptr_t size);
	/* tell the compiler we want both decommit from Base class and ours */
	using MM_VirtualMemory::decommitMemory;

	MMINLINE uintptr_t getReservedSize()
	{
		return _reserveSize;
	}
};

#endif /* SPARSEVIRTUALMEMORY_HPP_ */
