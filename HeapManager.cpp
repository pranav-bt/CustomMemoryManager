#include"HeapManager.h"
#include <iostream>
#include <cassert>



void* HeapManager::alloc(size_t datasize)
{
	return alloc(datasize, 4);
}

void* HeapManager::alloc(size_t datasize, int alignment) {
	MemoryBlock* root = tail;
	MemoryBlock* prevnext = tail->pPrevBlock;
	while (prevnext != nullptr)
	{
		if (prevnext->free)
		{
			size_t sizeavailable = reinterpret_cast<uintptr_t>(root) - reinterpret_cast<uintptr_t>(prevnext);
			size_t aligneddiff = (reinterpret_cast<uintptr_t>(root)-datasize +alignment)%alignment;
			size_t alignedsize = datasize + aligneddiff;
			if (sizeavailable >= (sizeof(MemoryBlock) + alignedsize))
			{
				MemoryBlock* prevofprevnext = prevnext->pPrevBlock;
				size_t remsize = sizeavailable - (sizeof(MemoryBlock) + alignedsize);
				MemoryBlock* newmemblock = CreateNewBlock(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(root) - alignedsize - sizeof(MemoryBlock)), alignedsize + sizeof(MemoryBlock));
				root->pPrevBlock = newmemblock;
				newmemblock->pNextBlock = root;
				newmemblock->free = false;
				if (remsize <= sizeof(MemoryBlock))
				{
					newmemblock->pPrevBlock = prevofprevnext;
					newmemblock->pPrevBlock->pNextBlock = newmemblock;
				}
				else
				{
					newmemblock->pPrevBlock = prevnext;
					prevnext->pNextBlock = newmemblock;
					prevnext->DataSize = reinterpret_cast<uintptr_t>(newmemblock) - reinterpret_cast<uintptr_t>(prevnext) - sizeof(MemoryBlock);
				}

				return newmemblock->pBaseAddress;
				
			}
		}
		root = prevnext;
		prevnext = prevnext->pPrevBlock;
	}
	return nullptr;
}

bool HeapManager::free(void* ptr) 
{
	uintptr_t pointer = reinterpret_cast<uintptr_t>(ptr) - sizeof(MemoryBlock);
	MemoryBlock* mempointer = reinterpret_cast<MemoryBlock*>(pointer);
	mempointer->free = true;
	return true;
}

void HeapManager::coalesce()
{
	MemoryBlock* root = head;
	MemoryBlock* next = root->pNextBlock;
	while (next != tail)
	{
		if (root->free == true && next->free == true)
		{
			root->pNextBlock = next->pNextBlock;
			next->pNextBlock->pPrevBlock = root;
			root->DataSize = reinterpret_cast<uintptr_t>(root->pNextBlock) - reinterpret_cast<uintptr_t>(root) - sizeof(MemoryBlock);
		}
		else
		{
			root = root->pNextBlock;
		}
		next = next->pNextBlock;
	}
}

MemoryBlock* HeapManager::CreateNewBlock(void* pointer, size_t size)
{
	if (size < sizeof(MemoryBlock))
	{
		return nullptr;
	}
	MemoryBlock* memblock = reinterpret_cast<MemoryBlock*>(pointer);
	memblock->pBaseAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pointer) + sizeof(MemoryBlock));
	memblock->DataSize = size - sizeof(MemoryBlock);
	memblock->pNextBlock = nullptr;
	memblock->pPrevBlock = nullptr;
	memblock->free = true;
	return memblock;
}

void HeapManager::Assign(void* start, size_t size, int num_descriptors) {
	_heapSize = size;
	_numDescriptors = num_descriptors;
	head = reinterpret_cast<MemoryBlock*>(start);
	assert(head);
	head->DataSize = 0;
	head->pBaseAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(start) + sizeof(MemoryBlock));
	head->pNextBlock = nullptr;
	head->pPrevBlock = nullptr;
	head->free = false;

	MemoryBlock* FirstBlock = reinterpret_cast<MemoryBlock*>(reinterpret_cast<uintptr_t>(head->pBaseAddress) + head->DataSize);
	assert(FirstBlock);
	FirstBlock->DataSize = size - 3 * sizeof(MemoryBlock);
	FirstBlock->pBaseAddress = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(head->pBaseAddress) + head->DataSize) + sizeof(MemoryBlock));
	FirstBlock->pNextBlock = nullptr;
	FirstBlock->pPrevBlock = nullptr;
	FirstBlock->free = true;

	tail = reinterpret_cast<MemoryBlock*>(reinterpret_cast<uintptr_t>(FirstBlock->pBaseAddress) + FirstBlock->DataSize);
	assert(tail);
	tail->DataSize = 0;
	tail->pBaseAddress = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(FirstBlock->pBaseAddress) + FirstBlock->DataSize) + sizeof(MemoryBlock));
	tail->pNextBlock = nullptr;
	tail->pPrevBlock = nullptr;
	tail->free = false;

	head->pNextBlock = FirstBlock;
	FirstBlock->pNextBlock = tail;
	tail->pPrevBlock = FirstBlock;
	FirstBlock->pPrevBlock = head;
	printblocks();
}

void HeapManager::printblocks()
{
	std::cout << "---------------------\n";
	MemoryBlock* root = head->pNextBlock;
	while (root != tail)
	{
		std::cout << root << "\t" << "Free: " << root->free << "\t" << "Datasize: " << root->DataSize << "\n";
		root = root->pNextBlock;
	}
	std::cout << "---------------------\n";
}


HeapManager* CreateHeapManager(void* pHeapMemory, size_t heapSize, int numDescriptors) 
{
	HeapManager* hm = static_cast<HeapManager*>(pHeapMemory);
	size_t hSize = sizeof(HeapManager);
	hm->Assign(static_cast<char*>(pHeapMemory) + hSize, heapSize - hSize, numDescriptors);
	return hm;
}

bool HeapManager::contains(void* ptr)
{
	return true;
}
bool HeapManager::isAllocated(void* ptr)
{
	return true;
}