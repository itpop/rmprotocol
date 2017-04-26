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
            WaitForSingleObject(Ev_Read_Thread_Finish, TIME_OUT_LONG);
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
            SetEvent(Ev_Send_Thread_Finish);
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

