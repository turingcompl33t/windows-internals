// heap_walk.cpp
//
// Demo of manually walking the allocations within a user-created heap.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 heap_walk.cpp

#include <windows.h>
#include <cstdio>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

VOID walk_heap(HANDLE heap_handle)
{
    // lock the heap to prevent other threads from accessing the heap during enumeration
    // not actually necessary in this single-threaded application, but illustrative
    if (::HeapLock(heap_handle) == FALSE) 
    {
        printf("Failed to lock heap %u\n", ::GetLastError());
        return;
    }

    printf("Walking heap %#p...\n\n", heap_handle);

    auto entry = PROCESS_HEAP_ENTRY{};
    entry.lpData = NULL;

    while (::HeapWalk(heap_handle, &entry) != FALSE) 
    {
        if ((entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) 
        {
            printf("Allocated block");

            if ((entry.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) != 0) 
            {
                printf(", movable with HANDLE %#p", entry.Block.hMem);
            }

            if ((entry.wFlags & PROCESS_HEAP_ENTRY_DDESHARE) != 0) 
            {
                printf(", DDESHARE");
            }
        }
        else if ((entry.wFlags & PROCESS_HEAP_REGION) != 0) 
        {
            printf("Region\n  %d bytes committed\n" \
                   "  %d bytes uncommitted\n  First block address: %#p\n" \
                   "  Last block address: %#p\n",
                     entry.Region.dwCommittedSize,
                     entry.Region.dwUnCommittedSize,
                     entry.Region.lpFirstBlock,
                     entry.Region.lpLastBlock);
        }
        else if ((entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) != 0) 
        {
            printf("Uncommitted range\n");
        }
        else 
        {
            printf("Block\n");
        }

        printf("  Data portion begins at: %#p\n  Size: %d bytes\n" \
               "  Overhead: %d bytes\n  Region index: %d\n\n",
                 entry.lpData,
                 entry.cbData,
                 entry.cbOverhead,
                 entry.iRegionIndex);
    }

    auto err = ::GetLastError();
    if (err != ERROR_NO_MORE_ITEMS) 
    {
        printf("HeapWalk failed %u\n", err);
    }

    // unlock the heap to allow other threads to access the heap after enumeration has completed
    if (::HeapUnlock(heap_handle) == FALSE) 
    {
        printf("Failed to unlock heap %u\n", ::GetLastError());
    }
}

int main()
{
    // create a new heap with default parameters
    auto heap_handle = ::HeapCreate(0, 0, 0);
    if (heap_handle == NULL) 
    {
        printf("Failed to create a new heap %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    // walk the custom and default process heaps
    walk_heap(::GetProcessHeap());
    walk_heap(heap_handle);

    // destroy the custom heap
    if (::HeapDestroy(heap_handle) == FALSE) 
    {
        printf("Failed to destroy heap with LastError %u\n", ::GetLastError());
    }

    return STATUS_SUCCESS_I;
}