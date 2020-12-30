#ifndef OSLABS_PAGEALLOCATOR_H
#define OSLABS_PAGEALLOCATOR_H

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

class PageAllocator
{
public:
    explicit PageAllocator(size_t size);
    void *mem_alloc(size_t size);
    void *mem_realloc(void* addr, size_t size);
    void mem_free(void* addr);
    void mem_free();
    void mem_dump();
    ~PageAllocator();

private:
    enum class PageState
    {
        Free,
        Divided,
        MultiplePage,
    };
    struct Header
    {
        PageState state;
        size_t size;
        void* startPointer;
        unsigned short blocks;
    };
    struct BlockHeader
    {
        void* next;
    };

    size_t const pageSize = 4 * 1024;
    size_t const minAllocationSize = 16;
    size_t pageAmount;
    
    void* startPointer;
    
    std::vector<void*> freePages;
    std::map<void*, Header> headers;
    std::map<size_t, std::vector<void*>> freeClassPages;
    
    inline bool CanFitInOnePage(size_t size) const
    {
        return size < pageSize / 2;
    }
    
    inline size_t GetClassSize(size_t size)
    {
        auto next = (size_t) pow(2, ceil(log(size) / log(2)));
        return std::max(minAllocationSize, next);
    }
    
    inline void* GetPage(void* addr)
    {
        size_t pageNumber = ((char*) addr - (char*) startPointer) / pageSize;
        return (char*) startPointer + pageNumber * pageSize;
    }
    
    void* FindClassPage(size_t classSize);
    void* DivideFreePage(size_t classSize);
    void* AllocateBlock(void* page);
    void* AllocateMultiplePages(size_t size);
    
    void FreeMultiplePages(void* firstPage, size_t amount);
    void FreeBlock(void* page, void* addr);
    
    void MoveMemory(void* addr, void* newAddr);
};


#endif //OSLABS_PAGEALLOCATOR_H
