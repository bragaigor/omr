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

#include "omrcomp.h"
#include "modronbase.h"
#include "ModronAssertions.h"
#include "SparseVirtualMemory.hpp"
#include "SparseOrderedFixedSizePool.hpp"
#include "Heap.hpp"

#define OMRVMEM_DEBUG

MM_SparseOrderedFixedSizePool *
MM_SparseOrderedFixedSizePool::newInstance(MM_EnvironmentBase *env, void *sparseHeapBase, uintptr_t sparsePoolSize)
{
	// TODO: Add trace points everywhere
	printf("Inside MM_SparseOrderedFixedSizePool::newInstance. Called with sparseHeapBase: %p, sparsePoolSize: %zu\n", sparseHeapBase, sparsePoolSize);
	MM_SparseOrderedFixedSizePool *sparsePool;

	sparsePool = (MM_SparseOrderedFixedSizePool *)env->getForge()->allocate(sizeof(MM_SparseOrderedFixedSizePool), OMR::GC::AllocationCategory::FIXED, OMR_GET_CALLSITE());
	if (sparsePool) {
		sparsePool = new(sparsePool) MM_SparseOrderedFixedSizePool(env, sparsePoolSize);
		if (!sparsePool->initialize(env, sparseHeapBase)) {
			sparsePool->kill(env);
			sparsePool = NULL;
		}
	}
	return sparsePool;
}

uintptr_t
MM_SparseOrderedFixedSizePool::entryHash(void *entry, void *userData)
{
	return (uintptr_t)((J9ObjDataTableEntry *)entry)->dataPtr;
}

uintptr_t
MM_SparseOrderedFixedSizePool::entryEquals(void *leftEntry, void *rightEntry, void *userData)
{
	uintptr_t lhs = (uintptr_t)((J9ObjDataTableEntry *)leftEntry)->dataPtr;
	uintptr_t rhs = (uintptr_t)((J9ObjDataTableEntry *)rightEntry)->dataPtr;
	return (lhs == rhs);
}

bool
MM_SparseOrderedFixedSizePool::initialize(MM_EnvironmentBase *env, void *sparseHeapBase)
{
	OMRPORT_ACCESS_FROM_OMRPORT(env->getPortLibrary());
	printf("Inside MM_SparseOrderedFixedSizePool::initialize\n");

	/* Initialize _freeListPool J9Pool */
	_freeListPool = pool_new(sizeof(MM_SparseHeapLinkedFreeHeader), 6, 0, 0, OMR_GET_CALLSITE(), OMRMEM_CATEGORY_MM, POOL_FOR_PORT(OMRPORTLIB));

	if (!_freeListPool) {
#if defined(OMRVMEM_DEBUG)
		printf("Failed to create memory pool for sparse heap\n");
		fflush(stdout);
#endif
		return false;
	}

	_objectToDataTable = hashTableNew(
		OMRPORTLIB, OMR_GET_CALLSITE(),
		11,
		sizeof(J9ObjDataTableEntry),
		sizeof(uintptr_t),
		0,
		OMRMEM_CATEGORY_MM,
		entryHash, 
		entryEquals,
		NULL,
		NULL);

	if (NULL == _objectToDataTable) {
#if defined(OMRVMEM_DEBUG)
		printf("Failed to create key value pair hash table for sparse heap\n");
		fflush(stdout);
#endif
		return false;
	}

	/* Initialize _heapFreeList with initial values */
	_heapFreeList = createNewFreeListNode(sparseHeapBase, _approxLargestFreeEntry);

	if (!_heapFreeList) {
#if defined(OMRVMEM_DEBUG)
		printf("Failed to create free List for sparse heap\n");
		fflush(stdout);
#endif
		return false;
        }

	printf("MM_SparseOrderedFixedSizePool initialization ompleted successfully!!!\n");

	return true;
}

#if defined(OSX)
struct J9PortVmemIdentifier*
MM_SparseOrderedFixedSizePool::getIdentifierFromDataPtr(void *dataPtr)
{
	J9ObjDataTableEntry lookupEntry;
	J9ObjDataTableEntry *entry;

	lookupEntry.dataPtr = dataPtr;
	entry = (J9ObjDataTableEntry *)hashTableFind(_objectToDataTable, &lookupEntry);
	if (NULL == entry || (entry->dataPtr != dataPtr)) {
		printf("Failed retrieving entry from J9ObjDataTableEntry for dataPtr: %p!!!!!!\n", dataPtr);
		return NULL;
	}

	return entry->identifier;
}

void
MM_SparseOrderedFixedSizePool::recordDoubleMapIdentifier(void *dataPtr, struct J9PortVmemIdentifier *identifier)
{
	J9ObjDataTableEntry lookupEntry;
	J9ObjDataTableEntry *entry;

	lookupEntry.dataPtr = dataPtr;
	entry = (J9ObjDataTableEntry *)hashTableFind(_objectToDataTable, &lookupEntry);

	if (NULL == entry || (entry->dataPtr != dataPtr)) {
#if defined(OMRVMEM_DEBUG)
		printf("Failed trying to retrieve object pointer: %p!\n", dataPtr);
		fflush(stdout);
#endif
	} else {
		printf("Updating identifier of entry dataPtr: %p!!!!! address: %p, size: %zu\n", dataPtr, identifier->address, identifier->size);
		entry->identifier = identifier;
	}
}
#endif

bool
MM_SparseOrderedFixedSizePool::rememberObjectData(void *dataPtr, void* proxyObjPtr, uintptr_t size)
{
	uintptr_t count = hashTableGetCount(_objectToDataTable);
	printf("Inside MM_SparseOrderedFixedSizePool::rememberObjectData. dataPtr: %p, proxyObjPtr: %p, num of nodes in table: %zu\n", dataPtr, proxyObjPtr, count);
	J9ObjDataTableEntry entry;
	entry.dataPtr = dataPtr;
	entry.proxyObjPtr = proxyObjPtr;
	entry.size = size;
	void *result = hashTableAdd(_objectToDataTable, &entry);

	if (NULL == result) {
#if defined(OMRVMEM_DEBUG)
		printf("ERROR: Failed to insert entry in _objectToDataTable table!!! dataPtr: %p\n", dataPtr);
		fflush(stdout);
#endif
		return false;
	}

	return true;
}

uintptr_t
MM_SparseOrderedFixedSizePool::getDataSizeForDataPtr(void *dataPtr)
{
	J9ObjDataTableEntry lookupEntry;
	J9ObjDataTableEntry *entry;

	lookupEntry.dataPtr = dataPtr;
	entry = (J9ObjDataTableEntry *)hashTableFind(_objectToDataTable, &lookupEntry);

	if (NULL == entry || (entry->dataPtr != dataPtr)) {
#if defined(OMRVMEM_DEBUG)
		printf("Failed trying to retrieve object pointer: %p!\n", dataPtr);
		fflush(stdout);
#endif
		return 0;
	}

	return entry->size;
}

bool
MM_SparseOrderedFixedSizePool::removeEntryFromTable(void *dataPtr)
{
	printf("||||||| Inside MM_SparseOrderedFixedSizePool::removeEntryFromTable. dataPtr: %p\n", dataPtr);
	J9ObjDataTableEntry entryToRemove;
	entryToRemove.dataPtr = dataPtr;

	uint32_t ret = hashTableRemove(_objectToDataTable, &entryToRemove);

	if (1 == ret) {
#if defined(OMRVMEM_DEBUG)
		printf("Failed trying to remove entry from hash table! dataPtr: %p\n", dataPtr);
		fflush(stdout);
#endif
	}

	return 0 == ret;
}

void *
MM_SparseOrderedFixedSizePool::findFreeEntry(uintptr_t size)
{
	Assert_MM_true(_freeListPoolFreeNodesCount > 0);
	MM_SparseHeapLinkedFreeHeader *previous = NULL;
	MM_SparseHeapLinkedFreeHeader *current = _heapFreeList;
	void *returnAddr = NULL;
	uintptr_t currSize = 0;

	/* Find big enough free space */
	while(NULL != current) {
		if(current->_size >= size) {
			break;
		}
		previous = current;
		current = current->_next;
	}

	/* Impossible for current to be NULL since sparse heap will always have available space no matter what */
	Assert_MM_true(NULL != current);

	currSize = current->_size;
	returnAddr = current->_address;
	if(currSize == size) {
		/* Remove current node from FreeList since we're using all of it */
		if (NULL == previous) {
			_heapFreeList = current->_next;
		} else {
			previous->_next = current->_next;
		}
		pool_removeElement(_freeListPool, current);
		_freeListPoolFreeNodesCount--;
	} else {
		/* Update current entry */
		current->setAddress((void*)((uintptr_t)returnAddr + size));
		current->shrinkSize(size);
		/* Update largest free entry */
		if (_largestFreeEntryAddr == returnAddr) {
			_approxLargestFreeEntry -= size;
			_largestFreeEntryAddr = current->_address;
		}
	}

	Assert_MM_true(NULL != returnAddr);
	_approximateFreeMemorySize -= size;
	_freeListPoolAlocBytes += size;

	return returnAddr;
}

bool
MM_SparseOrderedFixedSizePool::returnEntryToFreeList(void *dataAddr, uintptr_t size)
{
	MM_SparseHeapLinkedFreeHeader *previous = NULL;
	MM_SparseHeapLinkedFreeHeader *current = _heapFreeList;
	void *endAddress = (void*)((uintptr_t)dataAddr + size);

	/* First find where we should include the new node if needed */
	while (NULL != current) {
		/* Lazily update largest free entry. Better way to update it without too much overhead? */
		if (current->_size > _approxLargestFreeEntry) {
			_approxLargestFreeEntry = current->_size;
			_largestFreeEntryAddr = current->_address;
		}
		if (dataAddr < current->_address) {
			break;
		}
		previous = current;
		current = current->_next;
	}

	/* Where should we insert the newly space? The list is constructed in such a way that there
	 * will always be at least one node in it.
	 *** Insert between previous and current ***
	 * 1 -> previous -> SPACE -> <here> -> SPACE -> current
	 * 2 -> previous -> <here> -> SPACE -> current
	 * 3 -> previous -> SPACE -> <here> -> current
	 * 4 -> previous -> <here> -> current
	 *** Search reached end of freeList; therefore, current is NULL, insert after previous ***
	 * 5 -> previous -> <here> -> NULL
	 * 6 -> previous -> SPACE -> <here> -> NULL
	 *** previous is NULL therefore insert before current and make <here> head of freeList ***
	 * 7 -> <here> -> SPACE -> current
	 * 8 -> <here> -> current
	 */

	/* Case 7 and 8: previous is NULL */
	if (NULL == previous) {
		void *currentAddr = current->_address;
		if (endAddress == currentAddr) {
			current->expandSize(size);
			current->setAddress(dataAddr);
		} else {
			MM_SparseHeapLinkedFreeHeader *newNode = createNewFreeListNode(dataAddr, size);
			newNode->_next = current;
			_heapFreeList = newNode;
		}
        } else {
		/* Should we merge with previous */
		void *previousHighAddr = (void *)((uintptr_t)previous->_address + previous->_size);
		/* Case 2 and 5 */
		if (previousHighAddr == dataAddr) {
			previous->expandSize(size);
			/* Case 4: where node is right between previous and current therefore merge everything */
			if ((NULL != current) && (current->_address == endAddress)) {
				previous->expandSize(current->_size);
				previous->_next = current->_next;
				pool_removeElement(_freeListPool, current);
				_freeListPoolFreeNodesCount--;
			}
		/* Case 3: Merge node to current only */
		} else if ((NULL != current) && (current->_address == endAddress)) {
			current->expandSize(size);
			current->setAddress(dataAddr);
		/* Cases 1 and 6: Insert new node between nodes or at the end of the list */
		} else {
			Assert_MM_true(previousHighAddr < dataAddr);
			Assert_MM_true((NULL == current) || (current->_address > endAddress));
			MM_SparseHeapLinkedFreeHeader *newNode = createNewFreeListNode(dataAddr, size);
			previous->_next = newNode;
			newNode->_next = current;
		}
	}

	_approximateFreeMemorySize += size;
	_freeListPoolAlocBytes -= size;
	_lastFreeBytes = size;

#if defined(OMRVMEM_DEBUG)
	printf("Returning entry with address: %p, and size: %zu. _freeListPoolFreeNodesCount: %zu, _approximateFreeMemorySize: %zu, _freeListPoolAlocBytes: %zu\n", dataAddr, size, _freeListPoolFreeNodesCount, _approximateFreeMemorySize, _freeListPoolAlocBytes);
	fflush(stdout);
#endif


	return true;
}

void
MM_SparseOrderedFixedSizePool::tearDown(MM_EnvironmentBase *env) 
{
	if (_freeListPool) {
		pool_kill(_freeListPool);
	}

	if (_heapFreeList) {
		freeAllList();
	}
}

MM_SparseHeapLinkedFreeHeader *
MM_SparseOrderedFixedSizePool::createNewFreeListNode(void *dataAddr, uintptr_t size)
{
	MM_SparseHeapLinkedFreeHeader *newFreeListNode = (MM_SparseHeapLinkedFreeHeader *)pool_newElement(_freeListPool);
	if (NULL != newFreeListNode) {
		newFreeListNode->setAddress(dataAddr);
		newFreeListNode->setSize(size);
		newFreeListNode->_next = NULL;
		_freeListPoolFreeNodesCount++;
	}

	return newFreeListNode;
}

/**
 * Teardown and free the receiver by invoking the virtual #tearDown() function
 */
void
MM_SparseOrderedFixedSizePool::kill(MM_EnvironmentBase *env)
{
	tearDown(env);
	env->getForge()->free(this);
}

void
MM_SparseOrderedFixedSizePool::updateFreeListNode(MM_SparseHeapLinkedFreeHeader *node, void *address, uintptr_t size, MM_SparseHeapLinkedFreeHeader *next)
{
	node->setAddress(address);
	node->setSize(size);
	node->_next = next;
}

bool
MM_SparseOrderedFixedSizePool::updateCopiedObject(void *dataPtr, void *proxyObjPtr)
{
	J9ObjDataTableEntry lookupEntry;
	J9ObjDataTableEntry *entry;

	lookupEntry.dataPtr = dataPtr;
	entry = (J9ObjDataTableEntry *)hashTableFind(_objectToDataTable, &lookupEntry);

	if (NULL == entry || (entry->dataPtr != dataPtr)) {
#if defined(OMRVMEM_DEBUG)
		printf("|||||||||||||||||||||| ERROR: Failed trying to retrieve object pointer: %p, to update to proxyObjPtr: %p!!!\n", dataPtr, proxyObjPtr);
		fflush(stdout);
#endif
		Assert_MM_true(false);
		return false;
	}

	printf("Updating data in hash table for dataPtr: %p, from ptr: %p, to: %p\n", dataPtr, entry->proxyObjPtr, proxyObjPtr);
	entry->proxyObjPtr = proxyObjPtr;

	return true;
}

void
MM_SparseOrderedFixedSizePool::freeAllList()
{
        MM_SparseHeapLinkedFreeHeader *current = _heapFreeList;
        while (NULL != current) {
                MM_SparseHeapLinkedFreeHeader *temp = current;
                current = current->_next;
		pool_removeElement(_freeListPool, temp);
        }
}
