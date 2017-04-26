/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE:     Serial.h
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Isaac Morneau
--
-- PROGRAMMER:      Isaac Morneau
--
-- NOTES:
-- This header file includes common macro definitions and function
-- declarations for Serial.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef SERIAL_H
#define SERIAL_H
#include "Common.h"

// function prototypes
VOID connect();
VOID disconnect();
BOOL waitForData(char **str, DWORD buffer_size, DWORD TIMEOUT);
VOID sendData(char* msg, DWORD size, HANDLE lock);
BOOL timeout(DWORD msec);
BOOL waitForENQ();
#endif