#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

HANDLE hSlot;
HANDLE hFile;
LPCTSTR SlotName = TEXT("\\\\.\\mailslot\\blah");

BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName)
{
    //LPSECURITY_ATTRIBUTES atr{};
    //atr->bInheritHandle = TRUE;
    hSlot = CreateMailslot(lpszSlotName,
        0,                             // no maximum message size 
        MAILSLOT_WAIT_FOREVER,         // no time-out for operations 
        (LPSECURITY_ATTRIBUTES)NULL);
        //atr); // default security

    if (hSlot == INVALID_HANDLE_VALUE)
    {
        printf("CreateMailslot failed with %d\n", GetLastError());
        return FALSE;
    }
    else printf("Mailslot created successfully.\n");
    return TRUE;
}

BOOL WriteSlot(LPCTSTR lpszMessage)
{
    BOOL fResult;
    DWORD cbWritten;

    fResult = WriteFile(hFile,
        lpszMessage,
        (DWORD)(lstrlen(lpszMessage) + 1) * sizeof(TCHAR),
        &cbWritten,
        (LPOVERLAPPED)NULL);

    if (!fResult)
    {
        printf("WriteFile failed with %d.\n", GetLastError());
        return FALSE;
    }

    printf("Slot written to successfully.\n");

    return TRUE;
}

BOOL ReadSlot()
{
    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;
    LPTSTR lpszBuffer;
    TCHAR achID[80];
    DWORD cAllMessages;
    HANDLE hEvent;
    OVERLAPPED ov;

    cbMessage = cMessage = cbRead = 0;

    hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("ExampleSlot"));
    if (NULL == hEvent)
        return FALSE;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;

    fResult = GetMailslotInfo(hSlot, // mailslot handle 
        (LPDWORD)NULL,               // no maximum message size 
        &cbMessage,                   // size of next message 
        &cMessage,                    // number of messages 
        (LPDWORD)NULL);              // no read time-out 

    if (!fResult)
    {
        printf("GetMailslotInfo failed with %d.\n", GetLastError());
        return FALSE;
    }

    if (cbMessage == MAILSLOT_NO_MESSAGE)
    {
        printf("Waiting for a message...\n");
        return TRUE;
    }

    cAllMessages = cMessage;

    while (cMessage != 0)  // retrieve all messages
    {
        // Create a message-number string. 

        StringCchPrintf((LPTSTR)achID,
            80,
            TEXT("\nMessage #%d of %d\n"),
            cAllMessages - cMessage + 1,
            cAllMessages);

        // Allocate memory for the message. 

        lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,
            lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage);
        if (NULL == lpszBuffer)
            return FALSE;
        lpszBuffer[0] = '\0';

        fResult = ReadFile(hSlot,
            lpszBuffer,
            cbMessage,
            &cbRead,
            &ov);

        if (!fResult)
        {
            printf("ReadFile failed with %d.\n", GetLastError());
            GlobalFree((HGLOBAL)lpszBuffer);
            return FALSE;
        }

        // Concatenate the message and the message-number string. 

        StringCbCat(lpszBuffer,
            lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage,
            (LPTSTR)achID);

        // Display the message. 

        _tprintf(TEXT("Contents of the mailslot: %s\n"), lpszBuffer);

        GlobalFree((HGLOBAL)lpszBuffer);

        fResult = GetMailslotInfo(hSlot,  // mailslot handle 
            (LPDWORD)NULL,               // no maximum message size 
            &cbMessage,                   // size of next message 
            &cMessage,                    // number of messages 
            (LPDWORD)NULL);              // no read time-out 

        if (!fResult)
        {
            printf("GetMailslotInfo failed (%d)\n", GetLastError());
            return FALSE;
        }
    }
    CloseHandle(hEvent);
    return TRUE;
}

DWORD WINAPI WriteMsgs(LPVOID lpParam)
{
    hFile = CreateFile(SlotName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES)NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            (HANDLE)NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed with %d.\n", GetLastError());
        return FALSE;
    }
    WriteSlot(TEXT("Message 1 for mailslot."));
    WriteSlot(TEXT("Message 2 for mailslot."));
    Sleep(4000);
    WriteSlot(TEXT("Message 3 for mailslot."));
    Sleep(8000);
    CloseHandle(hFile);
    CloseHandle(hSlot);
    return 0;
}

DWORD WINAPI ReadMsgs(LPVOID lpParam)
{
    while (ReadSlot())
    {
        Sleep(3000);
    }
    return 0;
}
int _tmain()
{
    MakeSlot(SlotName);
    HANDLE  hThreadArray[2];

    /*WriteMsgs(NULL);
    ReadSlot(NULL);*/
    // Create thread for reading data from mailslot
    hThreadArray[0] = CreateThread(
        NULL,        // security 
        0,           // stack size
        ReadMsgs,    // thread func
        NULL,        // func args
        0,           // 
        NULL);       // return thread id 
    // Create thread for reading data from mailslot
    hThreadArray[1] = CreateThread(
        NULL,        // security 
        0,           // stack size
        WriteMsgs,    // thread func
        NULL,        // func args
        0,           // 
        NULL);       // return thread id 

    WaitForMultipleObjects(2, hThreadArray,TRUE, INFINITE);
}