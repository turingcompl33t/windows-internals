// RecordAccess.cpp
// Simple application to demonstrate random access file IO.
//
// USAGE:
//  RecordAccess.exe <FILE NAME> [N RECORDS]
//
// If invoked with a number of record specified, a new record
// database file is created, identified by <FILE NAME>.
//
// If invoked without a number of record specified, it is 
// assumed that the record database file already exists, and
// the user is dropped into an interactive prompt to manipulate
// the contents of the record database file.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

// maximum number of characters in record message
constexpr auto RECORD_MSG_SIZE = 256;

typedef struct _RECORD
{
    DWORD ReferenceCount;
    SYSTEMTIME CreationTime;
    SYSTEMTIME LastReferenceTime;
    SYSTEMTIME UpdateTime;
    TCHAR Message[RECORD_MSG_SIZE];
} RECORD;

typedef struct _HEADER
{
    DWORD NumRecords;
    DWORD NumNonEmptyRecords;
} HEADER;

INT _tmain(INT argc, PTCHAR argv[])
{
    HANDLE hFile;
    LARGE_INTEGER CurrentPtr;
    DWORD OpenOption;
    DWORD NumXfer;
    DWORD RecordNo;

    RECORD record;
    HEADER header = {0, 0};

    TCHAR string[RECORD_MSG_SIZE];
    TCHAR command;
    TCHAR extra;

    OVERLAPPED ov = {0, 0, 0, 0, NULL};
    OVERLAPPED ovZero = {0, 0, 0, 0, NULL};

    SYSTEMTIME CurrentTime;
    BOOL HeaderChange;
    BOOL RecordChange;

    OpenOption = ((argc > 2 && _ttoi(argv[2]) <= 0) || argc <= 2) ?
        OPEN_EXISTING : CREATE_ALWAYS;
    
    hFile = CreateFile(
        argv[1], 
        GENERIC_READ | GENERIC_WRITE, 
        0, 
        NULL, 
        OpenOption, 
        FILE_FLAG_RANDOM_ACCESS, 
        NULL
        );

    if (argc >= 3 && _ttoi(argv[2]) > 0)
    {
        // write the header and presize the new file
        header.NumRecords = _ttoi(argv[2]);
        WriteFile(hFile,
        &header,
        sizeof(header),
        &NumXfer,
        &ovZero
        );

        // compute the total size of the file
        CurrentPtr.QuadPart = static_cast<LONGLONG>(sizeof(RECORD)) * _ttoi(argv[2]) + sizeof(HEADER);

        SetFilePointerEx(hFile, CurrentPtr, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);

        _tprintf(_T("Created new record file %s with %u record capacity\n"),
            argv[1],
            _ttoi(argv[2])
            );

        return STATUS_SUCCESS_I;
    }

    // read the file header
    ReadFile(hFile, &header, sizeof(HEADER), &NumXfer, &ovZero);

    // prompt user to read / write numbered record
    for (;;)
    {
        HeaderChange = FALSE;
        RecordChange = FALSE;

        _tprintf(_T("Enter r(ead)/w(rite)/d(elete)/q(uit) RECORD #\n"));
        _tscanf(_T("%c%u%c"), &command, &RecordNo, &extra);
        
        if (command == 'q')
        {
            break;
        }

        if (RecordNo >= header.NumRecords)
        {
            _tprintf(_T("Invalid record number"));
            continue;
        }

        // perform the random access read based on record number
        CurrentPtr.QuadPart = static_cast<LONGLONG>(RecordNo) * sizeof(RECORD) + sizeof(HEADER);
        ov.Offset = CurrentPtr.LowPart;
        ov.OffsetHigh = CurrentPtr.HighPart;
        ReadFile(hFile, &record, sizeof(RECORD), &NumXfer, &ov);

        GetSystemTime(&CurrentTime);
        record.LastReferenceTime = CurrentTime;

        if (command == 'r' || command == 'd')
        {
            if (record.ReferenceCount == 0)
            {
                _tprintf(_T("Record number %u is empty\n"), RecordNo);
                continue;
            }
            else
            {
                _tprintf(_T("Record number %u\n"), RecordNo);
                _tprintf(_T("\tReference count: %u\n"), record.ReferenceCount);
                _tprintf(_T("\tCreation Time:   %u:%u:%u\n"), 
                    record.CreationTime.wHour, 
                    record.CreationTime.wMinute, 
                    record.CreationTime.wSecond);
                _tprintf(_T("\tReference Time:  %u:%u:%u\n"), 
                    record.LastReferenceTime.wHour, 
                    record.LastReferenceTime.wMinute, 
                    record.LastReferenceTime.wSecond);
                _tprintf(_T("\tUpdate Time:     %u:%u:%u\n"), 
                    record.UpdateTime.wHour, 
                    record.UpdateTime.wMinute, 
                    record.UpdateTime.wSecond);
                _tprintf(_T("\tMessage:         %s\n"), record.Message);
            }

            if (command == 'd')
            {
                record.ReferenceCount = 0;
                header.NumNonEmptyRecords--;
                HeaderChange = TRUE;
                RecordChange = TRUE;
            }
        }
        else if (command == 'w')
        {
            _tprintf(_T("Enter new message for record\n"));
            _fgetts(string, sizeof(string), stdin);
            string[_tcslen(string) - 1] = _T('\0');

            if (record.ReferenceCount == 0)
            {
                // this is a new record
                record.CreationTime = CurrentTime;
                header.NumNonEmptyRecords++;
                HeaderChange = TRUE;
            }

            record.UpdateTime = CurrentTime;
            record.ReferenceCount++;
            _tcsncpy_s(record.Message, string, RECORD_MSG_SIZE - 1);
            RecordChange = TRUE;
        }
        else
        {
            _tprintf(_T("Invalid command\n"));
        }

        if (RecordChange)
        {
            WriteFile(hFile, &record, sizeof(RECORD), &NumXfer, &ov);
        }
        if (HeaderChange)
        {
            WriteFile(hFile, &header, sizeof(HEADER), &NumXfer, &ovZero);
        }
    }

    _tprintf(_T("Computed number of non-empty records is: %u\n"), header.NumNonEmptyRecords);
    ReadFile(hFile, &header, sizeof(HEADER), &NumXfer, &ovZero);
    _tprintf(_T("File has %u non-empty records; total capacity: %u\n"), 
        header.NumNonEmptyRecords, 
        header.NumRecords
        );

    CloseHandle(hFile);

    return STATUS_SUCCESS_I;
}