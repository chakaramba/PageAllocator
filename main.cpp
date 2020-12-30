#include <iostream>
#include "PageAllocator.h"

void dummyTest()
{
    auto allocator = PageAllocator(32 * 1024);
    allocator.mem_dump();
    auto loc1 = allocator.mem_alloc(256);
    auto loc2 = allocator.mem_alloc(128);
    auto loc3 = allocator.mem_alloc(32);
    allocator.mem_dump();
    allocator.mem_free(loc1);
    allocator.mem_free(loc2);
    allocator.mem_free(loc3);
    allocator.mem_dump();
}

void allocationInOnePageTest()
{
    PageAllocator allocator = PageAllocator(32 * 1024);
    allocator.mem_dump();

    for (int i = 0; i < 4; ++i)
    {
        allocator.mem_alloc(1024);
    }
    
    allocator.mem_dump();
}

void multipageAllocationTest()
{
    PageAllocator allocator = PageAllocator(32 * 1024);
    allocator.mem_dump();
    
    allocator.mem_alloc(4 * 1024);
    allocator.mem_alloc(3 * 4 * 1024);
    
    allocator.mem_dump();
}

void freeOnePageTest()
{
    PageAllocator allocator = PageAllocator(32 * 1024);

    auto *loc1 = allocator.mem_alloc(512);
    allocator.mem_alloc(512);
    allocator.mem_alloc(512);
    auto *loc3 =  allocator.mem_alloc(512);

    allocator.mem_free(loc1);
    allocator.mem_free(loc3);

    allocator.mem_dump();
}

void freeMultipageTest()
{
    PageAllocator allocator = PageAllocator(32 * 1024);

    auto *loc1 = allocator.mem_alloc(4 * 1024);
    allocator.mem_alloc(2 * 4 * 1024);
    auto *loc3 = allocator.mem_alloc(4 * 1024);

    allocator.mem_free(loc1);
    allocator.mem_free(loc3);

    allocator.mem_dump();
}

void freeAll() {
    PageAllocator allocator = PageAllocator(32 * 1024);

    allocator.mem_alloc(2 * 4 * 1024);
    for (int i = 0; i < 4; ++i)
    {
        allocator.mem_alloc(1024);
    }
    
    allocator.mem_free();

    allocator.mem_dump();
}

void reallocationTest() {
    PageAllocator allocator = PageAllocator(32 * 1024);

    auto *loc = allocator.mem_alloc(512);
    loc = allocator.mem_realloc(loc, 4 * 1024);
    
    allocator.mem_dump();
}

int main() {
    try
    {
        reallocationTest();
    }
    catch (std::exception ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}