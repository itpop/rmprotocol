/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     RMProtocol.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  INT_PTR CALLBACK WndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
--                  int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
--                                     LPSTR lspszCmdParam, int nCmdShow);
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Fred Yang, Isaac Morneau, Trista Huang, Alex Zielinski
--
-- PROGRAMMER:      Fred Yang, Isaac Morneau, Trista Huang, Alex Zielinski
--
-- NOTES:
-- This program implements a simple wireless half-duplex protocal driver (fully event driven) that 
-- can be used for the transmission of Ascii Text Files and messages.
----------------------------------------------------------------------------------------------------------------------*/
#define STRICT
#include "RMProtocol.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// handle to the current window
HWND hwnd;
// global msg
MSG Msg;
// handle to the background brush
HBRUSH hbrBackground;
// file read/write thread
HANDLE	fileRWThread;
DWORD	fileRWThreadId;

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		WinMain
--
--  DATE:           December 3, 2016
--
--  DESIGNER:       Fred Yang, Isaac Morneau, Trista Huang, Alex Zielinski
--
--  PROGRAMMER:     Fred Yang, Isaac Morneau, Trista Huang, Alex Zielinski
--
--  INTERFACE:      WinMain (HINSTANCE hInst, HINSTANCE hprevInstance,
--                           LPSTR lspszCmdParam, int nCmdShow)
--                  HINSTANCE hInst: handle to an instance/module, it identifies the executable(EXE)
--                                   when it is loaded in memory.
--                  HINSTANCE hprevInstance: hPrevInstance has no meaning. It was used in 16 bit
--                                           Windows, but is now always zero.
--                  LPSTR lspszCmdParam: contains the command - line arguments as a Unicode string.
--                  int nCmdShow: a flag that says whether the main application window will be
--                                minimized, maximized, or shown normally.
--
-- RETURNS:         the return value is not used by OS, but you can use the return value to convey a
--                  status code to other apps that you write.
--
-- NOTES:
-- WINAPI is the calling convention, which defines how a function receives parameters from the caller.
-------------------------------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	               LPSTR lspszCmdParam, int nCmdShow) {
	// State - Build Window
	hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, WndProc, 0);
	ShowWindow(hDlg, nCmdShow);
	hSendPanel = GetDlgItem(hDlg, IDC_WRITERFILE);
	hReadPanel = GetDlgItem(hDlg, IDC_READERFILE);

    // initialize file opener
	initFileOpener();

    // register read/write panels
	SendMessage(GetDlgItem(hDlg, IDC_READERFILE), EM_SETLIMITTEXT, (LPARAM) FILE_LEN, NULL);
	SendMessage(GetDlgItem(hDlg, IDC_WRITERFILE), EM_SETLIMITTEXT, (LPARAM) FILE_LEN, NULL);

    // initialize comm port
	initPort();

	// State - Enter Comm param 
	configComm();

	// State - Engine Read Thread Start
	initRead();

	while (GetMessage(&Msg, NULL, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}


/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		WndProc
--
--  DATE:           December 3, 2016
--
--  DESIGNER:       Fred Yang, Isaac Morneau, Trista Huang, Alex Zielinski
--
--  PROGRAMMER:     Fred Yang, Isaac Morneau, Trista Huang, Alex Zielinski
--
--  INTERFACE:      LRESULT CALLBACK WndProc(HWND hwnd, UINT Message,
--                                           WPARAM wParam, LPARAM lParam)
--                  HWND hwnd: a handle to the window.
--                  UNIT Message: system-defined message when it communicates with an application.
--                  WPARAM wParam: additional message information. The contents of this parameter
--                                 depend on the value of the uMsg parameter.
--                  LPARAM lParam: additional message information. The contents of this parameter
--                                 depend on the value of the uMsg parameter.
--
--  RETURNS:        Result of the message processing.
--
--  NOTES:
--  Main callback function that processes messages sent to a window.
-------------------------------------------------------------------------------------------------------------------*/
INT_PTR CALLBACK WndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
    case WM_CTLCOLORSTATIC: { // static controls color settings
        DWORD ctlId = GetDlgCtrlID((HWND)lParam);
        if (ctlId != IDC_READERFILE && ctlId != IDC_WRITERFILE) {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(52,19,82));
            SetBkColor(hdcStatic, RGB(238, 238, 238));
            switch (ctlId) {
            case IDC_SLABEL0:
            case IDC_SLABEL1:
            case IDC_SLABEL2:
            case IDC_SLABEL3:
            case IDC_SLABEL4:
            case IDC_SLABEL5:
            case IDC_SLABEL6:
            case IDC_SDATA0:
            case IDC_SDATA1:
            case IDC_SDATA2:
            case IDC_SDATA3:
            case IDC_SDATA4:
            case IDC_SDATA5:
            case IDC_SDATA6:
                hbrBackground = CreateSolidBrush(RGB(238,238,238));
                break;
            default:
                hbrBackground = CreateSolidBrush(RGB(155, 200, 30));
                break;
            }
            return (INT_PTR) hbrBackground;
        }
        break;
    }
	case WM_COMMAND:  // command handling
		switch (LOWORD(wParam)) {
        case IDC_CONNECT:
            connect();
            break;
        case IDC_DISCONNECT:
            disconnect();
            break;
		case IDC_COMMSET:
			configComm();
			break;
		case IDC_OPEN:
			setFileOpenerFlags(OPEN_BROWSER);
			if (GetOpenFileName(&fileName) == TRUE) {
				fileRWThread = CreateThread(NULL, 0, createFileReader, NULL, 0, &fileRWThreadId);
			}
			loadFile(&hSendPanel, fileName.lpstrFile);
			break;
        case IDC_CLEAR_SENDER:
            write_packets.clear();
            clearBox(&hSendPanel);
            break;
		case IDC_SAVE:
			setFileOpenerFlags(SAVE_BROWSER);
			if (GetSaveFileName(&fileName) == TRUE) {
				fileRWThread = CreateThread(NULL, 0, createFileWriter, NULL, 0, &fileRWThreadId);
			}
			saveFile(&hReadPanel, fileName.lpstrFile);
			break;
		case IDC_CLEAR_RECEIVER:
            read_packets.clear();
			clearBox(&hReadPanel);
			break;
		case IDC_CLEAR_STATS:
			clearStats();
			break;
		case IDC_BUTTONSEND:
			setupProgressBar(&hSendPanel);
			errorCheck((writeThread = CreateThread(NULL, 0, loadPacketThread, NULL, 0, &writeThreadId)) == NULL ?
				ERR_WRITE_THREAD : NO_ERR);
			break;
		}
		break;

	case WM_CLOSE:	// Terminate program
		PostQuitMessage(0);
		break;
	default:
		return false;
	}
	return 0;
}