// WalkChangeJournal.cpp
// Walk the NTFS change journal.

#define UNICODE
#define _UNICODE

#include <Windows.h>
#include <WinIoCtl.h>
#include <stdio.h>

#define BUF_LEN 4096

void main()
{
    HANDLE hVol;
    CHAR Buffer[BUF_LEN];

    USN_JOURNAL_DATA JournalData;
    READ_USN_JOURNAL_DATA ReadData = {0, 0xFFFFFFFF, FALSE, 0, 0};
    PUSN_RECORD UsnRecord;  

    DWORD dwBytes;
    DWORD dwRetBytes;
    int I;

    hVol = CreateFile(
        TEXT("\\\\.\\c:"), 
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (hVol == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed (%d)\n", GetLastError());
        return;
    }

    if(!DeviceIoControl(
        hVol, 
        FSCTL_QUERY_USN_JOURNAL, 
        NULL,
        0,
        &JournalData,
        sizeof(JournalData),
        &dwBytes,
        NULL) 
    )
    {
        printf("Query journal failed (%d)\n", GetLastError());
        return;
    }

    ReadData.UsnJournalID = JournalData.UsnJournalID;
    ReadData.MaxMajorVersion = 3;

    printf("Journal ID: %I64x\n", JournalData.UsnJournalID);
    printf("FirstUsn: %I64x\n\n", JournalData.FirstUsn);

    for (I = 0; I <= 10; I++)
    {
        memset(Buffer, 0, BUF_LEN);

        if(!DeviceIoControl(
            hVol, 
            FSCTL_READ_USN_JOURNAL, 
            &ReadData,
            sizeof(ReadData),
            &Buffer,
            BUF_LEN,
            &dwBytes,
            NULL) 
        )
        {
            printf("Read journal failed (%d)\n", GetLastError());
            return;
        }

        dwRetBytes = dwBytes - sizeof(USN);

        // Find the first record
        UsnRecord = (PUSN_RECORD)(((PUCHAR)Buffer) + sizeof(USN));  

        printf( "****************************************\n");

        // This loop could go on for a long time, given the current buffer size.
        while (dwRetBytes > 0)
        {
            printf("USN: %I64x\n", UsnRecord->Usn);
            printf("File name: %.*S\n", 
                    UsnRecord->FileNameLength/2, 
                    UsnRecord->FileName);
            printf("Reason: %x\n", UsnRecord->Reason);
            printf("\n");

            dwRetBytes -= UsnRecord->RecordLength;

            // Find the next record
            UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + 
                    UsnRecord->RecordLength); 
        }

        // Update starting USN for next call
        ReadData.StartUsn = *(USN *)&Buffer; 
    }

    CloseHandle(hVol);

}