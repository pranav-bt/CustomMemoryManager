#pragma once
#define SUPPORTS_ALIGNMENT
#define SUPPORTS_SHOWFREEBLOCKS
struct MemoryBlock
{
	void* pBaseAddress;
	MemoryBlock* pNextBlock;
	MemoryBlock* pPrevBlock;
	size_t DataSize;
	bool free;
};


class HeapManager {
private:
	size_t _heapSize;
	int _numDescriptors;
	MemoryBlock * head, *tail;
	//uintptr_t heapstart;
public:
	MemoryBlock* FirstBlock;
	void* alloc(size_t size);
	void* alloc(size_t size, int alignment);
	void coalesce();
	bool free(void* ptr);
	void Assign(void* start, size_t size, int num_descriptors);
	MemoryBlock* CreateNewBlock(void* pointer, size_t size);
	void printblocks();
	bool contains(void* ptr);
	bool isAllocated(void* ptr);
};



HeapManager* CreateHeapManager(void* pHeapMemory, size_t heapSize, int numDescriptors);
bool HeapManager_UnitTest();