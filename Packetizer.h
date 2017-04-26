/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE:     Packetizer.h
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Trista Huang
--
-- PROGRAMMER:      Trista Huang
--
-- NOTES:
-- This header file includes common macro definitions and function
-- declarations pertaining to Packetizer.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef DATALINK_H
#define DATALINK_H
#include "Common.h"

// function prototypes
std::vector<std::string> parketize();
uint16_t calculateCRC16(const std::string& data);
std::string CRCtoString(uint16_t crc);
#endif
