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
-- DESIGNER:        Isaac Morneau, Alex Zielinski
--
-- PROGRAMMER:      Isaac Morneau, Alex Zielinski
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        connect
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Isaac Morneau, Alex Zielinski
--
-- PROGRAMMER:      Isaac Morneau, Alex Zielinski
--
-- INTERFACE:       connect()
--
-- RETURNS:         void
--
-- NOTES:
-- Sets up the CONNECT mode.
----------------------------------------------------------------------------------------------------------------------*/
VOID connect() {
    connected = TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        disconnect
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Isaac Morneau, Alex Zielinski
--
-- PROGRAMMER:      Isaac Morneau, Alex Zielinski
--
-- INTERFACE:       disconnect()
--
-- RETURNS:         void
--
-- NOTES:
-- Terminates the CONNECT mode
----------------------------------------------------------------------------------------------------------------------*/
VOID disconnect() {
    connected = FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       waitForData
--
--  DATE:           December 3, 2016
--
--  DESIGNER:       Isaac Morneau, Alex Zielinski
--
--  PROGRAMMER:     Isaac Morneau, Alex Zielinski
--
--  INTERFACE:      BOOL waitForData(char **str, DWORD buffer_size, DWORD	TIMEOUT)
--			        char **str - Buffer to store the content retrieved by Read_File
--					DWORD buffer_size - The size of the buffer, has to be specified explicitly
--					DWORD TIMEOUT - Waiting time
--
--  RETURNS:        TRUE if data read successfully, FALSE if timed out
--
--  NOTES:
--	Read from the serial port. If no data has been received before timer expires the function returns
--	false indicating a failure. If data is being detected on the serial port we start reading until 
--  PACKET_SIZE characters has been successfully processed or when we have read an EOT (end of 
--  transmission). Every character read from readfile is appended onto a string and after the while 
--  loop it is being copied to str.
----------------------------------------------------------------------------------------------------------------------*/
BOOL waitForData (
	char	**str,
	DWORD	buffer_size,
	DWORD	TIMEOUT)
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

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       waitForData
--
--  DATE:           December 3, 2016
--
--  DESIGNER:       Isaac Morneau, Alex Zielinski
--
--  PROGRAMMER:     Isaac Morneau, Alex Zielinski
--
--  INTERFACE:      VOID sendData(char* msg, DWORD size, HANDLE lock)
--					char *msg - buffer to sent to the serial port
--					DWORD size - size of the buffer
--					HANDLE lock - a handle to thread locker
--
--  RETURNS:        VOID
--
--  NOTES:
--  Send data in buffer to the serial port. Make sure that no other thread is running at the same time.
--  For each character sent to the serial port, make sure that it is being successfully processed before
--  sending the next one by calling WaitForSingleObject on the overlapped object.
----------------------------------------------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       isTimeout
--
--  DATE:           December 3, 2016
--
--  DESIGNER:       Isaac Morneau, Alex Zielinski
--
--  PROGRAMMER:     Isaac Morneau, Alex Zielinski
--
--  INTERFACE:      BOOL isTimeout(DWORD msec)
--					DWORD msec : the number of milliseconds to wait
--
--  RETURNS:        TRUE if EV_RXCHAR is read before timer expires, FALSE otherwise
--
--  NOTES:
--  Waits for an EV_RXCHAR event to occur on the serial port with a given amount of time.
--  EV_RXCHAR means a character was received and placed in the input buffer.
----------------------------------------------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       waitForENQ
--
--  DATE:           December 3, 2016
--
--  DESIGNER:       Isaac Morneau, Alex Zielinski
--
--  PROGRAMMER:     Isaac Morneau, Alex Zielinski
--
--  INTERFACE:      BOOL waitForENQ()
--
--  RETURNS:        TRUE if ENQ is read before timer expires, FALSE otherwise
--
--  NOTES:
--  Waits for an ENQ to arrive from the serial port before timeout. If ENQ is received, go to 
--  acknowledge line state; otherwise, go to idle state.
----------------------------------------------------------------------------------------------------------------------*/
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