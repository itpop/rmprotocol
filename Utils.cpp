/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     Utils.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  VOID errorCheck(DWORD err);
--                  VOID printMsg();
--                  VOID configComm();
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang
--
-- PROGRAMMER:      Fred Yang
--
-- NOTES:
-- This class wraps error check, comm config, and print methods.
----------------------------------------------------------------------------------------------------------------------*/
#include "Utils.h"

VOID errorCheck(DWORD err)
{
    LPCTSTR msg = NULL;
    switch (err)
    {
        case NO_ERR:
            return;
            break;
        case ERR_INIT_COMM:
            msg = "Failed to initialize serial port handle\n";
            break;
        case ERR_READ_THREAD :
            msg = "Failed to create read thread handle\n";
            break;
        case ERR_WRITE_THREAD :
            msg = "Failed to create write thread handle\n";
            break;
        case ERR_RETRIEVE_COMM :
            msg = "Failed to retrieve communication configuration\n";
            break;
        case ERR_DISPLAY_COMM :
            msg = "Failed to load configuration box\n";
            break;
        case ERR_SET_COMM:
            msg = "Failed to set communication parameters\n";
        case ERR_COMMMASK : 
            msg = "Failed to set comm mask";
            break;
    }
    MessageBox(NULL, msg, "Error", MB_OK); 
    OutputDebugString(msg);
    
    // Display formatted error message onto console output
    printMsg();
}

VOID printMsg()
{
    // Buffer to store the formatted error string
    wchar_t buf[256];
    // Format the DWORD into buffer
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
    OutputDebugStringW(buf);
}

VOID configComm()
{
    COMMCONFIG cc;                   //Configuration state of the serial port
    cc.dwSize = sizeof(COMMCONFIG);  //Set the size of the structure to default
    cc.wVersion = 0x100;             //Set the version number 
    errorCheck(!GetCommConfig(hComm, &cc, &cc.dwSize) ? ERR_RETRIEVE_COMM : NO_ERR);
    errorCheck(!CommConfigDialog(lpszCommName, hwnd, &cc) ? NO_ERR : NO_ERR);
    errorCheck(!SetCommState(hComm, &cc.dcb) ? ERR_SET_COMM : NO_ERR);
}