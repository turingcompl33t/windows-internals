// HeapWalk.cpp
// Demo of manually walking the allocations within a user-created heap.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

int __cdecl _tmain()
{
    DWORD LastError;
    HANDLE hHeap;
    PROCESS_HEAP_ENTRY Entry;

    // create a new heap with default parameters
    hHeap = HeapCreate(0, 0, 0);
    if (hHeap == NULL) 
    {
        _tprintf(TEXT("Failed to create a new heap with LastError %d.\n"),
                 GetLastError());
        return 1;
    }

    hHeap = GetProcessHeap();

    // lock the heap to prevent other threads from accessing the heap during enumeration
    if (HeapLock(hHeap) == FALSE) 
    {
        _tprintf(TEXT("Failed to lock heap with LastError %d.\n"),
                 GetLastError());
        return 1;
    }

    _tprintf(TEXT("Walking heap %#p...\n\n"), hHeap);

    Entry.lpData = NULL;
    while (HeapWalk(hHeap, &Entry) != FALSE) 
    {
        if ((Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) 
        {
            _tprintf(TEXT("Allocated block"));

            if ((Entry.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) != 0) 
            {
                _tprintf(TEXT(", movable with HANDLE %#p"), Entry.Block.hMem);
            }

            if ((Entry.wFlags & PROCESS_HEAP_ENTRY_DDESHARE) != 0) 
            {
                _tprintf(TEXT(", DDESHARE"));
            }
        }
        else if ((Entry.wFlags & PROCESS_HEAP_REGION) != 0) 
        {
            _tprintf(TEXT("Region\n  %d bytes committed\n") \
                     TEXT("  %d bytes uncommitted\n  First block address: %#p\n") \
                     TEXT("  Last block address: %#p\n"),
                     Entry.Region.dwCommittedSize,
                     Entry.Region.dwUnCommittedSize,
                     Entry.Region.lpFirstBlock,
                     Entry.Region.lpLastBlock);
        }
        else if ((Entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) != 0) 
        {
            _tprintf(TEXT("Uncommitted range\n"));
        }
        else 
        {
            _tprintf(TEXT("Block\n"));
        }

        _tprintf(TEXT("  Data portion begins at: %#p\n  Size: %d bytes\n") \
                 TEXT("  Overhead: %d bytes\n  Region index: %d\n\n"),
                 Entry.lpData,
                 Entry.cbData,
                 Entry.cbOverhead,
                 Entry.iRegionIndex);
    }

    LastError = GetLastError();
    if (LastError != ERROR_NO_MORE_ITEMS) 
    {
        _tprintf(TEXT("HeapWalk failed with LastError %d.\n"), LastError);
    }

    // unlock the heap to allow other threads to access the heap after enumeration has completed
    if (HeapUnlock(hHeap) == FALSE) {
        _tprintf(TEXT("Failed to unlock heap with LastError %d.\n"),
                 GetLastError());
    }

    // destroy the heap
    if (HeapDestroy(hHeap) == FALSE) 
    {
        _tprintf(TEXT("Failed to destroy heap with LastError %d.\n"),
                 GetLastError());
    }

    return 0;
}