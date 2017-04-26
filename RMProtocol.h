/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE:     RMProtocol.h
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang, Isaac Morneau
--
-- PROGRAMMER:      Fred Yang, Isaac Morneau
--
-- NOTES:
-- This header file includes the macro definitions and function
-- declarations for RMProtocol.
----------------------------------------------------------------------------------------------------------------------*/
#ifndef RMPROTOCOL_H
#define RMPROTOCOL_H
#include "Common.h"

// function prototypes
INT_PTR CALLBACK WndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	               LPSTR lspszCmdParam, int nCmdShow);
#endif