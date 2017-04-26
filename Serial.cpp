/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     Serial.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  VOID connect();
--                  VOID disconnect();
--                  BOOL waitForData(char **str, DWORD buffer_size, DWORD TIMEOUT);
--                  VOID sendData(char* msg, DWORD size, HANDLE lock);
--                  BOOL timeout(DWORD msec);
--                  BOOL waitForENQ();
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang, Alex Zielinski
--
-- PROGRAMMER:      Fred Yang, Alex Zielinski
--
-- NOTES:
-- This class wraps the timing and flow control operations pertaining to the serial port.
----------------------------------------------------------------------------------------------------------------------*/
#include "Serial.h"
using namespace std;

// sender / receiver priorities
BOOL senderPriority = FALSE;
BOOL receiverPriority = FALSE;

BOOL connected;
FILE_STATISTICS stats;

VOID connect() {
    connected = TRUE;
}

VOID disconnect() {
    connected = FALSE;
}

BOOL waitForData (
    char    **str,
    DWORD   buffer_size,
    DWORD   TIMEOUT)
{
    try {
        string s;
        DWORD total = 0, bytes_read;
        *str = new char[buffer_size];
        OVERLAPPED ovRead = { NULL };
        ovRead.hEvent = CreateEvent(NULL, FALSE, FALSE, EV_OVREAD);

        if (!timeout(TIMEOUT))
        {
            return FALSE;
        }

        // while characters read are smaller than the buffer size
        while (total < buffer_size)
        {
            char tmp[1] = "";
            if (!ReadFile(hComm, tmp, 1, &bytes_read, &ovRead))
            {
                if (GetLastError() == ERROR_IO_PENDING)
                    if (WaitForSingleObject(ovRead.hEvent, 50) != WAIT_OBJECT_0)
                        return FALSE;
            }
            if (tmp[0] == EOT)
            {
                s += tmp[0];
                break;
            }
            s += tmp[0];
            total++;
        }

        *str = (char*) malloc(strlen(s.c_str()));
        strcpy(*str, s.c_str());
    }
    catch (exception& e) {
        OutputDebugString(e.what());
        return FALSE;
    }

    return TRUE;
}

VOID sendData(char* msg, DWORD size, HANDLE lock)
{
    try {
        COMSTAT cs;
        DWORD err, result, bytes_written;
        OVERLAPPED ovWrite = { NULL };
        ovWrite.hEvent = CreateEvent(NULL, FALSE, FALSE, EV_OVWRITE);
        // Lock this thread
        WaitForSingleObject(lock, INFINITE);

        if (!WriteFile(hComm, msg, size, &bytes_written, &ovWrite))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                if ((result = WaitForSingleObject(ovWrite.hEvent, INFINITE)) == WAIT_OBJECT_0)
                {
                    if (GetOverlappedResult(hComm, &ovWrite, &bytes_written, FALSE))
                        OutputDebugString("Write Successfully\n");
                    else
                        OutputDebugString("Write Failed\n");
                }
                else
                    OutputDebugString("Error occured in Send()::WaitForSingleObject()\n");
                }
        }
        // Release this thread
        ReleaseMutex(hWrite_Lock);
        CloseHandle(ovWrite.hEvent);
        ClearCommError(hComm, &err, &cs);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }
}

BOOL timeout(DWORD msec)
{
    try {
        COMSTAT cs;
        DWORD fdwCommMask, err;
        SetCommMask(hComm, EV_RXCHAR);
        OVERLAPPED OverLapped;
        memset((char *)&OverLapped, 0, sizeof(OVERLAPPED));
        OverLapped.hEvent = CreateEvent(NULL, TRUE, TRUE, 0);

        if (!WaitCommEvent(hComm, &fdwCommMask, &OverLapped))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                if (WaitForSingleObject(OverLapped.hEvent, (DWORD)msec) != WAIT_OBJECT_0)
                    return FALSE;
            }
        }

        CloseHandle(OverLapped.hEvent);
        ClearCommError(hComm, &err, &cs);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
        return FALSE;
    }

    return TRUE;
}

BOOL waitForENQ()
{
    try {
        char *response = "";
        if (!waitForData(&response, 1, TIME_OUT_SHORT))
        {
            SetEvent(Ev_Send_Thread_Finish);
            SetEvent(Ev_Read_Thread_Finish);
            //If time'd out, go to idle state
            OutputDebugString("Time'd out from wait state, calling initRead()\n");
            return FALSE;
        }
        else if (response[0] == ENQ)
        {
            ResetEvent(Ev_Send_Thread_Finish);
            OutputDebugString("Got an ENQ from the wait state\n");
            return TRUE;
        }
    }
    catch (exception& e) {
        OutputDebugString(e.what());
        return FALSE;
    }

    return FALSE;
}