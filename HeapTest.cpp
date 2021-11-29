#include "HeapManager.h"
#include <Windows.h>
#include <assert.h>
#include <algorithm>
#include <vector>

bool HeapManager_UnitTest() {
	const size_t sizeHeap = 1024 * 1024;
	const unsigned int 	numDescriptors = 2048;

#ifdef USE_HEAP_ALLOC
	void* pHeapMemory = HeapAlloc(GetProcessHeap(), 0, sizeHeap);
#else
	// Get SYSTEM_INFO, which includes the memory page size
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	// round our size to a multiple of memory page size
	assert(SysInfo.dwPageSize > 0);
	size_t sizeHeapInPageMultiples = SysInfo.dwPageSize * ((sizeHeap + SysInfo.dwPageSize) / SysInfo.dwPageSize);
	void* pHeapMemory = VirtualAlloc(NULL, sizeHeapInPageMultiples, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
	assert(pHeapMemory);

	// Self Heap Test
	HeapManager* hm = CreateHeapManager(pHeapMemory, sizeHeap, numDescriptors);
	assert(hm);

	if (hm == nullptr)
		return false;

#ifdef TEST_SINGLE_LARGE_ALLOCATION
	// This is a test I wrote to check to see if using the whole block if it was almost consumed by 
	// an allocation worked. Also helped test my ShowFreeBlocks() and ShowOutstandingAllocations().
	{
#ifdef SUPPORTS_SHOWFREEBLOCKS
		hm->printblocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

		size_t largestBeforeAlloc = pHeapManager->getLargestFreeBlock();
		void* pPtr = pHeapManager->alloc(largestBeforeAlloc);

		if (pPtr) {
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After large allocation:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			hm->printblocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			pHeapManager->showOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif

			size_t largestAfterAlloc = pHeapManager->getLargestFreeBlock();
			bool success = pHeapManager->contains(pPtr) && pHeapManager->isAllocated(pPtr);
			assert(success);

			success = pHeapManager->free(pPtr);
			assert(success);

			pHeapManager->coalesce();

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After freeing allocation and garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			hm->printblocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			pHeapManager->showOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif

			size_t largestAfterCollect = pHeapManager->getLargestFreeBlock();
		}
	}
#endif

	std::vector<void*> AllocatedAddresses;

	long	numAllocs = 0;
	long	numFrees = 0;
	long	numCollects = 0;

	do {
		const size_t		maxTestAllocationSize = 1024;

		size_t	sizeAlloc = 1 + (rand() & (maxTestAllocationSize - 1));

#ifdef SUPPORTS_ALIGNMENT
		// pick an alignment
		const unsigned int	alignments[] = { 4, 8, 16, 32 };

		const unsigned int	index = rand() % (sizeof(alignments) / sizeof(alignments[0]));

		const unsigned int	alignment = alignments[index];

		void* pPtr = hm->alloc(sizeAlloc, alignment);

		// check that the returned address has the requested alignment
		assert((reinterpret_cast<uintptr_t>(pPtr) & (alignment - 1)) == 0);
#else
		void* pPtr = hm->alloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

		// if allocation failed see if garbage collecting will create a large enough block
		if (pPtr == nullptr) {
			hm->coalesce();

#ifdef SUPPORTS_ALIGNMENT
			pPtr = hm->alloc(sizeAlloc, alignment);
#else
			pPtr = hm->alloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

			// if not we're done. go on to cleanup phase of test
			if (pPtr == nullptr) {
				hm->printblocks();
				break;
			}
				
		}
		
		AllocatedAddresses.push_back(pPtr);
		numAllocs++;

		// randomly free and/or garbage collect during allocation phase
		const unsigned int freeAboutEvery = 10;
		const unsigned int garbageCollectAboutEvery = 40;

		if (!AllocatedAddresses.empty() && ((rand() % freeAboutEvery) == 0)) {
			void* pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			bool success = hm->contains(pPtr) && hm->isAllocated(pPtr);
			
			assert(success);


			success = hm->free(pPtr);
			assert(success);

			numFrees++;
		}

		if ((rand() % garbageCollectAboutEvery) == 0) {
			hm->coalesce();

			numCollects++;
		}


	} while (1);

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
	printf("After exhausting allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
	hm->printblocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	pHeapManager->showOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	printf("\n");
#endif

	// now free those blocks in a random order
	if (!AllocatedAddresses.empty()) {
		// randomize the addresses
		std::random_shuffle(AllocatedAddresses.begin(), AllocatedAddresses.end());

		// return them back to the heap manager
		while (!AllocatedAddresses.empty()) {
			void* pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			bool success = hm->contains(pPtr) && hm->isAllocated(pPtr);
			assert(success);

			success = hm->free(pPtr);
			assert(success);
		}

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After freeing allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		hm->printblocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		pHeapManager->showOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif

		// do garbage collection
		hm->coalesce();
		// our heap should be one single block, all the memory it started with

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		hm->printblocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		pHeapManager->showOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif

		// do a large test allocation to see if garbage collection worked
		void* pPtr = hm->alloc(sizeHeap / 2);
		assert(pPtr);

		if (pPtr) {
			bool success = hm->contains(pPtr) && hm->isAllocated(pPtr);
			assert(success);

			success = hm->free(pPtr);
			assert(success);

		}
	}
	hm->printblocks();
	hm = nullptr;

	if (pHeapMemory) {
#ifdef USE_HEAP_ALLOC
		HeapFree(GetProcessHeap(), 0, pHeapMemory);
#else
		VirtualFree(pHeapMemory, 0, MEM_RELEASE);
#endif
	}

	// we succeeded
	return true;
}