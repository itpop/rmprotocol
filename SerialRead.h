/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE:     SerialRead.h
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang
--
-- PROGRAMMER:      Fred Yang
--
-- NOTES:
-- This header file includes common macro definitions and function
-- declarations for SerialRead.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef SERIAL_Read_H
#define SERIAL_Read_H
#include "Common.h"

// function prototypes
int getBER();
VOID initPort();
VOID initRead();
DWORD WINAPI readIdle(LPVOID);
VOID sendACK();
CHAR readInput();
BOOL evaluateInput(CHAR);
VOID waitForPacket();
BOOL validatePacket(const char*);
BOOL validateCheckSum(std::string, std::string);
#endif
