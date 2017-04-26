/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: OpenFile.h
--
-- DATE:        December 3, 2016
--
-- DESIGNER:    Alex Zielinski
--
-- PROGRAMMER:  Alex Zielinski
--
-- NOTES:
-- This header file includes macro definitions and function
-- declarations pertaining to file handling.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef OPENFILE_H
#define OPENFILE_H
#include "Common.h"
#include <commctrl.h>
#include <iostream>
extern HWND hSendPanel;
extern HWND hReadPanel;

// function prototypes
void initFileOpener();
void setFileOpenerFlags(int browserAction);
DWORD WINAPI createFileReader(LPVOID lpParam);
DWORD WINAPI createFileWriter(LPVOID lpParam);
void loadFile(const HWND *box, LPSTR file);
void addLine(const HWND *box, std::string line);
std::vector<std::string> parseData();
std::string getLine(const HWND *box, int line, int flag);
void setupProgressBar(const HWND *box);
void updateProgressBar(int currentPos);
int getLines(const HWND *box);
void saveFile(const HWND *box, LPSTR file);
void clearBox(const HWND *box);
void clearStats();
void updateStats(int stat, int label);
#endif
