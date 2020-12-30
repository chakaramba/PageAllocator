#include "PageAllocator.h"
#include <string>
#include <cassert>
#include <iostream>
#include <algorithm>

PageAllocator::PageAllocator(size_t size)
{
    pageAmount = ceil((double) size / pageSize);
    size = pageAmount * pageSize;
    startPointer = malloc(size);
    for (int i = 0; i < pageAmount; ++i)
    {
        void* page = (void*) ((char*) (startPointer) + i * pageSize);
        freePages.push_back(page);
        Header header = Header();
        header.state = PageState::Free;
        headers.insert(std::pair<void*, Header>(page, header));
    }
    for (size_t i = minAllocationSize; i <= pageSize / 2; i *= 2)
    {
        freeClassPages.insert(std::pair<size_t, std::vector<void*>>(i, std::vector<void*>()));
    }
}

void *PageAllocator::mem_alloc(size_t size)
{
    if (CanFitInOnePage(size))
    {
        auto classSize = GetClassSize(size);
        auto page = FindClassPage(classSize);
        if (page == nullptr) page = DivideFreePage(classSize);
        return AllocateBlock(page);
    }
    return AllocateMultiplePages(size);
}

void *PageAllocator::FindClassPage(size_t classSize)
{
    auto pages = freeClassPages[classSize];
    if (pages.empty()) return nullptr;
    return pages.front();
}

void *PageAllocator::DivideFreePage(size_t classSize)
{
    assert(!freePages.empty() && "No free pages available");
    auto page = freePages.back();
    freePages.pop_back();
    
    auto header = headers[page];
    header.size = classSize;
    header.startPointer = page;
    header.state = PageState::Divided;
    header.blocks = pageSize / classSize;
    headers[page] = header;

    for (int i = 0; i < header.blocks - 1; ++i)
    {
        void* block = (int *) ((char*) page + i * header.size);
        auto blockHeader = (BlockHeader*) block;
        void* nextBlock = (int *) ((char*) page + (i + 1) * header.size);
        blockHeader->next = nextBlock;
    }
    
    freeClassPages[classSize].push_back(page);
    return page;
}

void *PageAllocator::AllocateBlock(void *page)
{
    auto header = headers[page];
    auto addr = header.startPointer;
    auto next = ((BlockHeader*) addr)->next;
    header.startPointer = next;
    header.blocks--;
    if (header.blocks == 0)
    {
        auto pages = freeClassPages[header.size];
        pages.erase(std::remove(pages.begin(), pages.end(), page), pages.end());
    }
    headers[page] = header;
    return addr;
}

void *PageAllocator::AllocateMultiplePages(size_t size)
{
    size_t amount = ceil((double) size / pageSize);
    assert(amount <= freePages.size() && "No room for multipage allocation");
    std::vector<void*> pages;
    for (int i = 0; i < pageAmount; ++i)
    {
        void* page = (void*) ((char*) (startPointer) + i * pageSize);
        auto header = headers[page];
        if(header.state == PageState::Free)
        {
            pages.push_back(page);
            if (pages.size() == amount) break;
        }
        else
        {
            pages.clear();
        }
    }
    assert(pages.size() == amount && "Not enough continuous pages for allocation");
    for(auto const& page: pages)
    {
        auto header = headers[page];
        header.state = PageState::MultiplePage;
        header.blocks = amount;
        header.size = amount * pageSize;
        headers[page] = header;
        freePages.erase(std::remove(freePages.begin(), freePages.end(), page), freePages.end());
    }
    return pages.front();
}

void *PageAllocator::mem_realloc(void *addr, size_t size)
{
    auto newAddr = mem_alloc(size);
    if (newAddr == nullptr) return nullptr;
    
    MoveMemory(addr, newAddr);
    mem_free(addr);
    return newAddr;
}

void PageAllocator::MoveMemory(void *addr, void *newAddr)
{
    auto page = GetPage(addr);
    auto newPage = GetPage(newAddr);
    
    auto size = headers[page].size;
    auto newSize = headers[newPage].size;
    
    memcpy(addr, newAddr, std::min(size, newSize));
}

void PageAllocator::mem_free(void *addr)
{
    auto page = GetPage(addr);
    auto header = headers[page];
    if (header.state == PageState::Divided) FreeBlock(page, addr);
    else FreeMultiplePages(page, header.blocks);
}

void PageAllocator::FreeBlock(void *page, void *addr)
{
    auto header = headers[page];
    
    auto next = header.startPointer;
    header.startPointer = addr;
    ((BlockHeader*) addr)->next = next;

    header.blocks++;
    if (header.blocks == pageSize / header.size)
    {
        header.state = PageState::Free;
        freePages.push_back(page);
        auto pages = freeClassPages[header.size];
        pages.erase(std::remove(pages.begin(), pages.end(), page), pages.end());
    }
    headers[page] = header;
}

void PageAllocator::FreeMultiplePages(void *firstPage, size_t amount)
{
    for (int i = 0; i < amount; ++i)
    {
        void* page = ((char*) (firstPage) + i * pageSize);
        auto header = headers[page];
        header.state = PageState::Free;
        headers[page] = header;
        freePages.push_back(page);
    }
}

void PageAllocator::mem_free()
{
    freePages.clear();
    for (int i = 0; i < pageAmount; ++i)
    {
        void* page = (void*) ((char*) (startPointer) + i * pageSize);
        auto header = headers[page];
        header.state = PageState::Free;
        headers[page] = header;
        freePages.push_back(page);
    }
    for(auto &classes: freeClassPages)
    {
        classes.second.clear();
    }
}

void PageAllocator::mem_dump()
{
    std::cout << "Page amount: " << pageAmount << std::endl;
    std::cout << "Page size: " << pageSize << std::endl;
    std::cout << "Free pages: " << freePages.size() << std::endl;
    for (int i = 0; i < pageAmount; ++i)
    {
        void* page = (void*) ((char*) (startPointer) + i * pageSize);
        auto header = headers[page];
        auto state = header.state == PageState::Free ? "Free" :
                header.state == PageState::Divided ? "Divided" : "Multiple";
        auto classSize = header.state == PageState::Divided ? " | Class size: " + std::to_string(header.size) : "";
        auto freePlace = header.state == PageState::Divided ? " | Free blocks: " + std::to_string(header.blocks) : "";
        std::cout << "Page " << page << " " << state << classSize << freePlace << std::endl;
    }
    std::cout << std::endl;
}

PageAllocator::~PageAllocator()
{
    free(startPointer);
}
