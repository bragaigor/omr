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
 *
******************************************************************************/

#if !defined(SPARSEORDEREDFIXEDSIZEPOOL_HPP_)
#define SPARSEORDEREDFIXEDSIZEPOOL_HPP_

#include "omrpool.h"
#include "BaseVirtual.hpp"
#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"
#include "SparseHeapLinkedFreeHeader.hpp"

/**
 *
 * @ingroup GC_Base_Core
 */

typedef struct J9ObjDataTableEntry {
	void *dataPtr; /**< Object data pointer related to proxy object */
	void *proxyObjPtr; /**< Pointer to proxy object that resides at in-heap */
	uintptr_t size; /**< Total size of data pointed to by value */
} J9ObjDataTableEntry;

class MM_SparseOrderedFixedSizePool : public MM_BaseVirtual
{
/*
 * Data members
 */
public:
protected:
	/* Minimum size of free chunks in this pool */
	uintptr_t _approxLargestFreeEntry;  /**< largest free entry found at the end of a global GC cycle */
	void *_largestFreeEntryAddr; /**< Largest free entry location */
	uintptr_t _approximateFreeMemorySize;  /**< The approximate number of bytes free that could be made part of the free list */
	uintptr_t _lastFreeBytes; /**< Number of bytes free at end of last GC */
	uintptr_t _freeListPoolFreeNodesCount; /**< Number of free list nodes. There's always at least one node in list therefore >= 1 */
	uintptr_t _freeListPoolAlocBytes; /**< Byte amount allocated from sparse heap */

	MM_GCExtensionsBase *_extensions; /**< GC Extensions for this JVM */
	J9Pool *_freeListPool; /**< Memory pool to be used to create MM_SparseHeapLinkedFreeHeader nodes */
	MM_SparseHeapLinkedFreeHeader *_heapFreeList; /**< List of free node regions available at heap */

private:
	/* Map from object addresses to its corresponsing data address at sparse heap */
	J9HashTable *_objectToDataTable;

/*
 * Function members
 */
public:

	static MM_SparseOrderedFixedSizePool *newInstance(MM_EnvironmentBase *env, void *sparseHeapBase, uintptr_t sparsePoolSize);

	MM_SparseOrderedFixedSizePool(MM_EnvironmentBase *env, uintptr_t sparsePoolSize) 
		: MM_BaseVirtual()
		, _approxLargestFreeEntry(sparsePoolSize)
		, _largestFreeEntryAddr(NULL)
		, _approximateFreeMemorySize(sparsePoolSize)
		, _lastFreeBytes(0)
		, _freeListPoolFreeNodesCount(0)
		, _freeListPoolAlocBytes(0)
		, _extensions(env->getExtensions())
		, _freeListPool(NULL)
		, _heapFreeList(NULL)
		, _objectToDataTable(NULL)
	{
		_typeId = __FUNCTION__;
	}

	/**
	 * Finds first available free region that fits parameter size
	 *
	 * @param: region size
	 * @return address of free region or NULL if there's no such contiguous free region
	 */

	void *findFreeEntry(uintptr_t size);
	/**
	 * A region was freed, now we insert that back into the freeList ordered by address
	 *
	 * @param address	void*		Address associated to region to be returned
	 * @param size		uintptr_t	Size of region to be returned to freeList
	 */
	bool returnEntryToFreeList(void *address, uintptr_t size);

	/**
	 * Add mapping from object to data location to hash table
	 *
	 * @param dataPtr	void*		data location pointer
	 * @param proxyObjPtr	void*		Proxy object associated to dataPtr
	 * @param size		uintptr_t	Size of region cosumed by dataPtr
	 *
	 * @return true if object is added to hash table successfully, false otherwise
	 */
	bool rememberObjectData(void *dataPtr, void *proxyObjPtr, uintptr_t size);

	/**
	 * Removed entry pointed by the object pointer from the hash table
	 *
	 * @param dataPtr	void*		Data pointer
	 *
	 * @return true if key associated to dataPtr is removed successfully, false otherwise
	 */
	bool removeEntryFromTable(void *dataPtr);

	/**
	 * Get data size in bytes associated with data pointer
	 *
	 * @param dataPtr	void*		Data pointer
	 * @return size of data pointer in bytes
	 */
	uintptr_t getDataSizeForDataPtr(void *dataPtr);

	MMINLINE uintptr_t getLargestFreeEntry()
	{
		return _approxLargestFreeEntry;
	}

	MMINLINE void setLargestFreeEntry(uintptr_t approxLargestFreeEntry)
	{
		_approxLargestFreeEntry = approxLargestFreeEntry;
	}

	/**
	 *
	 *
	 */
	bool updateCopiedObject(void *dataPtr, void *proxyObjPtr);

protected:
private:
	bool initialize(MM_EnvironmentBase *env, void *sparseHeapBase);
	void tearDown(MM_EnvironmentBase *env);
	void kill(MM_EnvironmentBase *env);
	void updateFreeListNode(MM_SparseHeapLinkedFreeHeader *node, void *address, uintptr_t size, MM_SparseHeapLinkedFreeHeader *next);
	MM_SparseHeapLinkedFreeHeader *createNewFreeListNode(void *dataAddr, uintptr_t size);
	/**
	 * Frees all nodes from free List
	 */
	void freeAllList();

	static uintptr_t entryHash(void *entry, void *userData);
	static uintptr_t entryEquals(void *leftEntry, void *rightEntry, void *userData);
};

#endif /* SPARSEORDEREDFIXEDSIZEPOOL_HPP_ */
