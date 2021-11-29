#include <cstdlib>
#include <iostream>

#include "HeapManager.h"

bool HeapManager_CustomTest(void* heapStart, size_t sizeHeap, int num_descriptors) {
	// Self Heap Test
	HeapManager* hm = CreateHeapManager(heapStart, sizeHeap, num_descriptors);
	do {
		const size_t		maxTestAllocationSize = 10;

		size_t	sizeAlloc = 1 + (rand() & (maxTestAllocationSize - 1));

#ifdef SUPPORTS_ALIGNMENT
		// pick an alignment
		const unsigned int	alignments[] = { 4, 8, 16, 32, 64 };

		const unsigned int	index = rand() % (sizeof(alignments) / sizeof(alignments[0]));

		const unsigned int	alignment = alignments[index];

	//	void* pPtr = alloc(pHeapManager, sizeAlloc, alignment);

		// check that the returned address has the requested alignment
		assert((reinterpret_cast<uintptr_t>(pPtr) & (alignment - 1)) == 0);
#else
		void* pPtr = hm->alloc(4);
#endif // SUPPORT_ALIGNMENT

		// if allocation failed see if garbage collecting will create a large enough block
		if (pPtr == nullptr) {
		//	hm->collect();

#ifdef SUPPORTS_ALIGNMENT
			pPtr = alloc(pHeapManager, sizeAlloc, alignment);
#else
			pPtr = hm->alloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

			// if not we're done. go on to cleanup phase of test
			if (pPtr == nullptr)
				break;
		}


	} while (1);
	return false;
}
