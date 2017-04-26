/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     SerialWrite.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  DWORD WINAPI loadPacketThread(LPVOID lpvoid);
--                  VOID initWrite(char* packet);
--                  DWORD WINAPI transferPacket(LPVOID packet);
--                  BOOL confirmLine();
--                  VOID sendPacket(char* str);
--                  BOOL evalResponse(char c);
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang
--
-- PROGRAMMER:      Fred Yang
--
-- NOTES:
-- This class wraps the basic writing operations on serial comm port.
----------------------------------------------------------------------------------------------------------------------*/
#include "SerialWrite.h"
#pragma warning (disable: 4996)
using namespace std;

// write lock
HANDLE hWrite_Lock = CreateMutex(NULL, FALSE, WRITE_LOCK);
// send thread finish event
HANDLE Ev_Send_Thread_Finish = CreateEvent(NULL, TRUE, FALSE, NULL);

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       loadPacketThread
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      DWORD WINAPI loadPacketThread(LPVOID lpvoid)
--
--  RETURNS:        DWORD thread exit code
--
--  NOTES:
--	Packetize file opened on the GUI and feeds each packets into the write packet pool. Wait until each packets
--  to finish tranmission before sending the next one, allocate a thread for this to prevent potential hanging
--  in the main thread.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI loadPacketThread(LPVOID lpvoid)
{
    try {
        write_packets.clear();
	    write_packets = parseData();

	    for (auto packet : write_packets)
	    {
		    char *tmp = new char[packet.length() + 1];
		    *tmp = 0;
		    strncat(tmp, packet.c_str(), packet.length() + 1);
		    initWrite(tmp);
		    WaitForSingleObject(Ev_Read_Thread_Finish, TIME_OUT_LONG);
		    OutputDebugString("[WRITING NEXT PACKET]\n");
		    ResetEvent(Ev_Read_Thread_Finish);
		    // stats sendPackets
		    updateStats(++stats.packetSent, IDC_SDATA0);
	    }
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       initWrite
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      initWrite(LPBYTE packet)
--					char* packet - the packet to be sent
--
--  RETURNS: VOID
--
--  NOTES:
--	Initialize the write operation and report any errors if it occurs
----------------------------------------------------------------------------------------------------------------------*/
VOID initWrite(char* packet)
{
    try {
	    errorCheck((writeThread = CreateThread(NULL, 0, transferPacket, (LPVOID)packet, 0, &writeThreadId)) 
            == NULL ? ERR_WRITE_THREAD : NO_ERR);
	    WaitForSingleObject(Ev_Send_Thread_Finish, INFINITE);
	    ResetEvent(Ev_Send_Thread_Finish);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        transferPacket
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      transferPacket(LPVOID packet)
--					LPVOID packet - the packet to be sent
--
--  RETURNS:        0 upon successful thread finish
--
--  NOTES:
--	Represents the sending state of the wireless protocol design.  This thread is called whenever a user chooses a
--  file to send and presses the Send button.  Firstly confirm the line to make sure the line is available.  If 
--  confirm line times out, it goes back to the Wait state to wait for an ENQ, and re-initialize the read thread. 
--  If confirm line succeeds, call sendPacket() to send packets. When sendPacket succeeds or times out, we do a check
--  on the priority states of the sender and receiver. After transmission, we may wait for an ENQ, or just go directly
--  to the read idle state based on the priority of sender/receiver.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI transferPacket(LPVOID packet)
{
    try {
	    ResetEvent(Ev_Read_Thread_Finish);
	    SetCommMask(hComm, RETURN_COMM_EVENT);
	    WaitForSingleObject(hWrite_Lock, TIME_OUT_LONG);

	    char* s = (char*) packet;

	    if (!confirmLine()) 
	    {
		    OutputDebugString("exceeded confirm line max tries\n");

		    //WAIT STATE: wait for an enq, for a specified amount of time.
		    receivedENQinWait = waitForENQ();
		    ReleaseMutex(hWrite_Lock);
		    initRead();
		    SetEvent(Ev_Send_Thread_Finish);
		    delete[] s;
		    return 0;
	    }

	    sendPacket(s);

	    // Check priorities to determine if go directly to read idle, or
        // just wait for ENQ
	    if (!(receiverPriority && !senderPriority))
        {
		    receivedENQinWait = waitForENQ();
        }
        else {
            SetEvent(Ev_Send_Thread_Finish);
        }

	    //going back to wait/idle state
	    ReleaseMutex(hWrite_Lock);
	    initRead();

        // reset send panel
        clearBox(&hSendPanel);

	    delete[] s;
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       confirmLine
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      confirmLine()
--
--  RETURNS:        TRUE if get an ACK for the ENQ that we sent, or FALSE if it times out on getting
--                  an ACK, or it exceeds the maximum number of tries.
--
--  NOTES:
--  Send an ENQ to the serial port to query if the line is ready for sending a packet. Returns true if 
--  the line is confirmed and available. Otherwise retry the query, returns false when it exceeds the 
--  maximum number of tries.
----------------------------------------------------------------------------------------------------------------------*/
BOOL confirmLine() {
	DWORD numTries_confirmLine = 0;

	// Try to send an ENQ to confirm the line until we reach the maximum attempts
	while (numTries_confirmLine < LINE_TRIES) {
		// Send an ENQ to query for the line
        char c = ENQ;
		sendData(&c, sizeof(c), hWrite_Lock);

		char *response;
		if (!waitForData(&response, 1, TIME_OUT))
		{
			// Time'd out, increment counter and restart the loop to send the ENQ again
			numTries_confirmLine++;
			continue;
		}
		if (evalResponse(response[0]))
			return TRUE;
	}
	return FALSE;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       sendPacket
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      sendPacket(char* str)
--					char* str - the packet to be sent
--
--  RETURNS: VOID
--
--  NOTES:
--  Send a packet to the serial port.  If we get an ACK back, then the packet just sent is considered
--  transmitted successfully.  If we don't get an ACK back, we assume something went wrong with the
--  transmission and resend the same packet up to the maximum tries.
----------------------------------------------------------------------------------------------------------------------*/
VOID sendPacket(char* str) { 
	DWORD numTries_sendPacket = 0;

	// Try to send the packet until we reach the maximum attempts
	while (numTries_sendPacket < SEND_TRIES) {
		// Send the packet
		sendData(str, strlen(str), hWrite_Lock);

		// Wait for a response for the packet we sent
		char *str = "";

        if (!waitForData(&str, 1, TIME_OUT_LONG))
		{
			updateStats(++stats.packetSent, IDC_SDATA0);
			numTries_sendPacket++;
			continue;
		}
		else if (evalResponse (str[0])) {
			updateProgressBar (progressSize / write_packets.size());
            updateStats(++stats.acksReceived, IDC_SDATA4);
			return;
		}
	}

    // reaches the maximum attempts
    updateStats(++stats.packetLost, IDC_SDATA1);
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       evalResponse
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      evalResponse(char c)
--
--  RETURNS:        TRUE if we got an ACK, FALSE otherwise.
--
--  NOTES:
--	Evaluate if we got an ACK
----------------------------------------------------------------------------------------------------------------------*/
BOOL evalResponse(char c)
{
	if (c == ACK)
	{
		OutputDebugString("Got ACK\n");
		return TRUE;
	}
	return  FALSE;
}
