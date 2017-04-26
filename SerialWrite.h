/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE:     SerialWrite.h
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang
--
-- PROGRAMMER:      Fred Yang
--
-- NOTES:
-- This header file includes common macro definitions and function
-- declarations for SerialWrite.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef SERIAL_WRITE_H
#define SERIAL_WRITE_H
#include "Common.h"

// function prototypes
DWORD WINAPI loadPacketThread(LPVOID lpvoid);
VOID initWrite(char* packet);
DWORD WINAPI transferPacket(LPVOID packet);
BOOL confirmLine();
VOID sendPacket(char* str);
BOOL evalResponse(char c);
#endif