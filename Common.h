/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE:     Common.h
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Alex Zielinski
--
-- PROGRAMMER:      Alex Zielinski
--
-- NOTES:
-- This header file includes common macro definitions and function
-- declarations for the app.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef COMMON_H
#define COMMON_H
#include <windows.h>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <regex>
#include "resource.h"
#include "Utils.h"
#include "OpenFile.h"
#include "Serial.h"
#include "SerialRead.h"
#include "SerialWrite.h"
#include "Packetizer.h"
#include "Session.h"
#include "RMProtocol.h"
#pragma warning (disable: 4996)

// return comm event
#define RETURN_COMM_EVENT	101
// Error checking macros
#define NO_ERR				400
#define ERR_INIT_COMM		401
#define ERR_READ_THREAD		402
#define ERR_WRITE_THREAD	403
#define ERR_RETRIEVE_COMM	404
#define ERR_DISPLAY_COMM	405
#define ERR_SET_COMM		406
#define ERR_COMMMASK		407

// Packet command macro
#define NUL     0x00
#define SOH		0x01
#define EOT		0x04
#define ENQ		0x05
#define ACK		0x06
#define SYN 	0x16
// Filled NUL
#define NUL0    0x14

// Custom Events
#define EV_SOH		TEXT("SOH")
#define EV_ACK		TEXT("ACK")
#define EV_ENQ		TEXT("ENQ")
#define EV_EOT		TEXT("EOT")
#define EV_OVREAD	TEXT("OVREAD")
#define EV_OVWRITE	TEXT("OVWRITE")
#define READ_LOCK	TEXT("READLOCK")
#define WRITE_LOCK	TEXT("WRITELOCK")

// File browser window variables
#define SAVE_BROWSER	    2530
#define OPEN_BROWSER	    2531
#define FILE_NAME_LEN       260
#define FILE_LEN            1000000

// 1(SYNC)+1024(DATA)+2(CRC)
#define PACKET_SIZE         1027
#define PACKET_DATA_SIZE    1024
#define PACKET_DATA_INDEX   1

// Timeouts in ms
#define TIME_OUT            500
#define TIME_OUT_SHORT      200
#define TIME_OUT_LONG       2000

// Timeout max tries
#define LINE_TRIES          1
#define SEND_TRIES          1

// labels
#define LABEL_COUNT         7
#define LABEL_START_ID      10022

// default port#
static	LPCSTR	lpszCommName	= "com1";
static	TCHAR	Name[]			= TEXT("Comm Shell");
// previous CRCs, used to avoid duplicated packets
static std::string prev_crcs;

// containers holds r/w packets
static	std::vector<std::string> write_packets;
static	std::vector<std::string> read_packets;

// OpenFile.h layer variables
// file handle
extern HANDLE		fileHandle;
// file name open with dialog
extern OPENFILENAME fileName;
// handle to dialog
extern HWND			hDlg;
// buffer for file name
extern char			szFile[FILE_NAME_LEN];
extern int			progressSize;

// Packetizer.h layer variables
extern MSG		    Msg;
extern size_t       fileSize;
extern std::string  rawStr;
extern HWND		    hwnd;
extern HANDLE	    fileRWThread;
extern DWORD	    fileRWThreadId;

// serial.h layer variables
extern HANDLE	hComm;
static HANDLE	readThread; 
static DWORD	readThreadId;
static HANDLE	writeThread;
static DWORD	writeThreadId;
extern BOOL     connected;

// lock r/w threads
extern HANDLE hWrite_Lock;
extern HANDLE hRead_Lock;

// r/w thread handles
extern HANDLE Ev_Send_Thread_Finish;
extern HANDLE Ev_Read_Thread_Finish;
extern BOOL receivedENQinWait;

// Priority variables
extern BOOL senderPriority;
extern BOOL receiverPriority;

// Invalid character struct
struct INVALID_CHAR
{
    bool operator()(char c) const {
        if (c == '\r' || c == '\n') return false;
        return !(c >= -1 && c <= 255);
    }
};

// File stats
struct FILE_STATISTICS {
	int packetSent;
    int packetLost;
	int packetReceived;
	int packetCorrupted;
	int acksReceived;
	int bitErrorRate;
};

extern FILE_STATISTICS stats;
#endif
