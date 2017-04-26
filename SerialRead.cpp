/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     SerialRead.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  int getBER();
--                  VOID initPort();
--                  VOID initRead();
--                  DWORD WINAPI readIdle(LPVOID);
--                  VOID sendACK();
--                  CHAR readInput();
--                  BOOL evaluateInput(CHAR);
--                  VOID waitForPacket();
--                  BOOL validatePacket(const char*);
--                  BOOL validateCheckSum(std::string, std::string);
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang
--
-- PROGRAMMER:      Fred Yang
--
-- NOTES:
-- This class wraps the basic reading operations on serial comm port.
----------------------------------------------------------------------------------------------------------------------*/
#include "SerialRead.h"
using namespace std;

// handle to the comm port
HANDLE hComm;
// read lock
HANDLE hRead_Lock = CreateMutex(NULL, FALSE, READ_LOCK);
// read thread finish event
HANDLE Ev_Read_Thread_Finish = CreateEvent(NULL, TRUE, TRUE, 0);
// ENQ in wait flag
BOOL receivedENQinWait;

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       initPort
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      BOOL initPort();
--
--  RETURNS:        VOID
--
--  NOTES:
--	Initialize serial communication handle to allow overlapped communication
----------------------------------------------------------------------------------------------------------------------*/
VOID initPort()
{
    try {
	    errorCheck((hComm = CreateFile(lpszCommName, 
		    GENERIC_READ | GENERIC_WRITE, 
		    0,
		    NULL, 
		    OPEN_EXISTING, 
		    FILE_FLAG_OVERLAPPED, // File overlapped flag to allow asynchrounous operations
		    NULL)) == INVALID_HANDLE_VALUE ?
            ERR_INIT_COMM : NO_ERR);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       initRead
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      VOID initRead()
--
--  RETURNS:        VOID
--
--  NOTES:
--	Initialize the read operation and reports any error if it occurs
----------------------------------------------------------------------------------------------------------------------*/
VOID initRead()
{
    try {
	    errorCheck((readThread = CreateThread(NULL, 0, readIdle, NULL , 0, &readThreadId)) 
                    == NULL ? ERR_READ_THREAD : NO_ERR);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       readIdle
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      DWORD WINAPI readIdle(LPVOID lpvoid)
--
--  RETURNS:        0 upon successful thread finish
--
--  NOTES:
--	Represents the idle state of the wireless protocol design. It waits until an ENQ is read from the serial
--  port. It sends an ACK back to the port and then waits for a packet to come back after. Lastly the thread
--  returns to the idle state and waits for ENQ to arrive again.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI readIdle(LPVOID lpvoid)
{	
    try {
	    while (1)
	    {
		    // If we got an ENQ already from the wait state, go directly to sending 
            // an ACK to acknowledge line
		    if (receivedENQinWait) {
			    ResetEvent(Ev_Read_Thread_Finish);
			    // Clear the flag
			    receivedENQinWait = false;
			    // Send ACK
			    sendACK();
			    // Now wait for the packet to come, and validate it
			    waitForPacket();
		    } 
		    else
		    {
			    // Idle state waiting
			    ResetEvent(Ev_Read_Thread_Finish);
                if (evaluateInput(readInput()))
			    {
				    sendACK();
				    // Now we wait for the packet to come, and validate it
				    waitForPacket();
			    }
		    }
	    }
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       sendACK
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      VOID sendACK()
--
--  RETURNS:        VOID
--
--  NOTES:
--  Sends an ACK to the serial port.
----------------------------------------------------------------------------------------------------------------------*/
VOID sendACK()
{
    char c = ACK;
    sendData(&c, sizeof(c), hRead_Lock);
    updateStats(++stats.acksReceived, IDC_SDATA4);
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       readInput
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      LPSTR readInput()
--
--  RETURNS:        long pointer to the char that has been received from the port
--
--  NOTES:
--	Use event driven approach to wait for ENQ. Used only by the readIdle() thread. Also handles the mechanism
--	to exit the thread properly when performing WaitCommEvent()
----------------------------------------------------------------------------------------------------------------------*/
CHAR readInput()
{
	COMSTAT cs;
    char c = '\0';
	OVERLAPPED ovRead = { NULL };

    try {
	    // Create overlapped event
	    ovRead.hEvent = CreateEvent(NULL, FALSE, FALSE, EV_OVREAD);
	    //	Set listener to a character
	    errorCheck(!SetCommMask(hComm, EV_RXCHAR) ? ERR_COMMMASK : NO_ERR);
	    DWORD read_byte, dwEvent, dwError;
	    // Waits for EV_RXCHAR event to trigger
	    if (WaitCommEvent(hComm, &dwEvent, NULL))
	    {
		    // Clear Comm Port
		    ClearCommError(hComm, &dwError, &cs);
		    if ((dwEvent & EV_RXCHAR) && cs.cbInQue)
		    {
                if (!ReadFile(hComm, &c, sizeof(c), &read_byte, &ovRead))
				    printMsg();
		    }
	    }

	    // If a event other than EV_RXCHAR has occured, which should be triggered by the 
        // RETURN_COMM_EVENT macro.
	    if (dwEvent == 0)
		    // End the read thread
		    ExitThread(readThreadId);

	    // Discards all characters from the output and input buffer
	    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR);
	    CloseHandle(ovRead.hEvent);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }

	return c;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       evaluateInput
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      evaluateInput(CHAR c)
--					CHAR c - the character to be evaluated
--
--  RETURNS:        TRUE if c is an ENQ, false otherwise
--
--  NOTES:
--	Evaluate if we received an ENQ.
----------------------------------------------------------------------------------------------------------------------*/
BOOL evaluateInput(CHAR c)
{
    return (c == ENQ);
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       waitForPacket
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      VOID waitForPacket()
--
--  RETURNS:        VOID
--
--  NOTES:
--	Waiting for characters arrive at the serial port and attempt to parketize them into a packet
----------------------------------------------------------------------------------------------------------------------*/
VOID waitForPacket()
{
    try {
        BOOL  received = FALSE;

        while (!received)
        {
            char *str;

            // If timeout waiting for packet
            if (!waitForData(&str, PACKET_SIZE, TIME_OUT_LONG))
            {
                // If when sender has higher priority, go to wait state
                if (!receiverPriority && senderPriority) {
                    OutputDebugString("Going to wait state\n");
                    receivedENQinWait = waitForENQ();
                }

                OutputDebugString("Going back to idle\n");
                SetEvent(Ev_Read_Thread_Finish);
                return;
            }

            // fixed length packet
            if (strlen(str) == PACKET_SIZE)
            {
                // if the packet is validated, send an ack, otherwise continue to 
                // wait for a new packet
                if (validatePacket(str))
                {
                    received = true;
                }
            }
        }
        // send ACK to confirm a valid packet
        sendACK();

        if (!receiverPriority && senderPriority) {
            // WAIT STATE: wait for an enq, for a specified amount of time.
            receivedENQinWait = waitForENQ();
        }

        SetEvent(Ev_Read_Thread_Finish);
        SetEvent(Ev_Send_Thread_Finish);
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       validatePacket
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      BOOL validatePacket(const char *packet)
--				    packet - char array to validate
--
--  RETURNS:       TRUE upon valid packet, FALSE otherwise
--
--  NOTES:
--	Validate packet, make sure the packet structure is correct and that the packet is in sync
----------------------------------------------------------------------------------------------------------------------*/
BOOL validatePacket(const char *packet)
{
	string message;
    string crcs;

    try {
        // first byte is SYN
        if (packet[0] == SYN)
	    {
            for (size_t i = PACKET_DATA_INDEX; i <= PACKET_DATA_SIZE; i++)
            {
                // NUL can not be the filler as the packets that contain NUL, NULL, or '\0' are simply
                // terminated when transmitting. Ignore NUL0s in the packet.
                if (packet[i] != NUL0)
                    message += packet[i];
            }

            // the last 2 CRC bytes
            crcs.push_back(packet[PACKET_SIZE - 2]);
            crcs.push_back(packet[PACKET_SIZE - 1]);

            // check if its a duplicated packet by CRCs
            if (crcs != prev_crcs) {
                if (!validateCheckSum(message, crcs)) {
                    updateStats(++stats.packetCorrupted, IDC_SDATA3);
                }
                else {
                    message.erase(remove_if(message.begin(), message.end(), INVALID_CHAR()), message.end());
                    read_packets.push_back(message);
                    addLine(&hReadPanel, message);
                    updateStats(++stats.packetReceived, IDC_SDATA2);
                }
                updateStats(getBER(), IDC_SDATA6);
                prev_crcs = crcs;
            }
	    }
    }
    catch (exception& e) {
        OutputDebugString(e.what());
        return FALSE;
    }

	return TRUE;
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       getBER
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      int getBER()
--
--  RETURNS:        returns Bit Error Rate
--
--  NOTES:
--	Calculate the Bit Error Rate (BER)
----------------------------------------------------------------------------------------------------------------------*/
int getBER() {
    return (int)(100 * stats.packetCorrupted / (stats.packetReceived + stats.packetCorrupted));
}

/*------------------------------------------------------------------------------------------------------------------
--  FUNCTION:       validateCheckSum
--
--  DATE:			December 3, 2016
--
--	DESIGNER:		Fred Yang
--
--	PROGRAMMER:		Fred Yang
--
--  INTERFACE:      BOOL validateCheckSum(string data, string crcs)
--				    string data - data to be calculated
--                  crcs - the base CRC to be compared
--
--  RETURNS:        TRUE if validated, FALSE otherwise
--
--  NOTES:
--	Validate checksum to avoid processing duplicated packets.
----------------------------------------------------------------------------------------------------------------------*/
BOOL validateCheckSum(string data, string crcs) {
    uint16_t crcRaw = calculateCRC16(data);
    string crcCheck = CRCtoString(crcRaw);

    if (crcCheck != crcs) {
        return FALSE;
    }

    return TRUE;
}
