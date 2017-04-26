/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: OpenFile.cpp
--
-- PROGRAM:     RMProtocol
--
-- Functions
--              void initFileOpener();
--              void setFileOpenerFlags(int browserAction);
--              DWORD WINAPI createFileReader(LPVOID lpParam);
--              DWORD WINAPI createFileWriter(LPVOID lpParam);
--              void loadFile(const HWND *box, LPSTR file);
--              void addLine(const HWND *box, std::string line);
--              vector<std::string> parseData();
--              string getLine(const HWND *box, int line, int flag);
--              void setupProgressBar(const HWND *box);
--              void updateProgressBar(int currentPos);
--              int getLines(const HWND *box);
--              void saveFile(const HWND *box, LPSTR file);
--              void clearBox(const HWND *box);
--              void clearStats();
--              void updateStats(int stat, int label);
--
-- DATE:        December 3, 2016
--
-- DESIGNER:    Alex Zielinski
--
-- PROGRAMMER:  Alex Zielinski
--
-- NOTES:
-- This program is designed to create a file opener and handle file loading and saving.
----------------------------------------------------------------------------------------------------------------------*/
#include "OpenFile.h"
using namespace std;

// file opener dialog structure.
OPENFILENAME fileName;

// handle to the file opener.
HANDLE fileHandle;

// handle to main dialog.
HWND hDlg;

// left panel (send) handle
HWND hReadPanel;

// left panel (read) handle
HWND hSendPanel;

// send lines (left panel)
int sendLines;

// read lines (right panel)
int readLines;

// the max size of the progress bar
int progressSize;

// file path
char szFile[FILE_NAME_LEN];

// vector to contain strings from sender control
vector<std::string> v_packets;

regex addNewLine("(?!\r)\n");

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		createFileReader
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		DWORD WINAPI createFileReader(LPVOID lpParam);
--					LPVOID lpParam - a void pointer that points to parameters passed in by CreateThread.
--
--	RETURNS:		DWORD WINAPI
--
--	NOTES:
--  This function creates a file on the handle for the file opener. The file opener is a window which allows
--  the user to browse through their computer. This function is specific to opening the file browser to open
--	files, rather than save them.
-------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI createFileReader(LPVOID lpParam) {
	fileHandle = CreateFile(fileName.lpstrFile,
		GENERIC_READ,
		0,
		(LPSECURITY_ATTRIBUTES) NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE) hDlg);

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		createFileWriter
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		DWORD WINAPI createFileWriter(LPVOID lpParam);
--					LPVOID lpParam - a void pointer that points to parameters passed in by CreateThread.
--
--	RETURNS:		DWORD WINAPI
--
--	NOTES:
--  This function creates a file on the handle for the file opener. The file opener is a window which
--  allows the user to browse through their computer. This function is specific to opening the file
--	browser to save files, rather than open them.
-------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI createFileWriter(LPVOID lpParam) {
	fileHandle = CreateFile(fileName.lpstrFile,
		GENERIC_WRITE,
		0,
		(LPSECURITY_ATTRIBUTES) NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE) hDlg);

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		loadFile
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void loadFile(const HWND *box, LPSTR file);
--					const HWND *box - the specific HWND this file is being loaded to.
--					LPSTR file - the file path the file will be read from.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function uses the constructed file from the createFileReader to load a plain text file
--  into the specified handle, (*box). The function parses the file line by line, and sends each
--  parsed line as a line into the specified handle (normally an Edit Box).
-------------------------------------------------------------------------------------------------------------------*/
void loadFile(const HWND *box, LPSTR file) {
	int idx;
	string tmp;
	ifstream input(file);
	clearBox(box);
	SendMessage(*box, EM_SETREADONLY, (LPARAM) FALSE, NULL);

	while (getline(input, tmp)) {
		tmp += "\n";
		idx = GetWindowTextLength(*box);
		SendMessage(*box, EM_SETSEL, (LPARAM) idx, (LPARAM) idx);
		SendMessage(*box, EM_REPLACESEL, 0, (LPARAM) (tmp.c_str()));
	}
	sendLines = SendMessage(*box, EM_GETLINECOUNT, NULL, NULL);

	setupProgressBar(&hSendPanel);
	input.close();
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		saveFile
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void saveFile(const HWND *box, LPSTR file);
--					const HWND *box - the HWND to the edit control where we get the data.
--					LPSTR file - The file path to save a new file to.
--
--	RETURNS:		VOID
--
--	NOTES:
--  When this function is called, the file to write to is opened and the data from the specified
--  edit control will be saved into the file.
-------------------------------------------------------------------------------------------------------------------*/
void saveFile(const HWND *box, LPSTR file) {
    size_t onLine = 0;
    size_t totalLines = getLines(box);
	ofstream output(file);

	for (; onLine < totalLines; onLine++) {
        //output << getLine(box, onLine, 1) << endl;
        string s = getLine(box, onLine, 1);
        s.erase(remove_if(s.begin(), s.end(), INVALID_CHAR()), s.end());
        for (auto& c : s) {
            if (c == '\r' || c == '\n') {
                output << endl;
            }
            else {
                output << c;
            }
        }
        output << endl;
	}

	output.close();
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		clearBox
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void clearBox(const HWND *box);
--					const HWND *box - the specific HWND that the text is being cleared.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function takes in a handle to a UI element, and sets it's text to be an empty string. It
--  is inteded to be called when the user clicks on one of the clear buttons for the Edit Boxes.
-------------------------------------------------------------------------------------------------------------------*/
void clearBox(const HWND *box) {
    SetWindowText(*box, "");
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		getLines
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		int getLines(const HWND *box)
--					const HWND *box - the HWND to the edit control where we get the data.
--
--	RETURNS:		Returns the amount of lines of text in the edit control.
--
--	NOTES:
--  This function targets the specified Edit Control, and returns the amount of text lines
--  in the control.
-------------------------------------------------------------------------------------------------------------------*/
int getLines(const HWND *box) {
	return SendMessage(*box, EM_GETLINECOUNT, NULL, NULL);
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		parseData
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		std::vector<std::string> parseData()
--
--	RETURNS:		std::vector<std::string> - a vector that contains frame string.
--
--	NOTES:
--  This function parses all the data in the Send Edit Box, and puts the strings into a
--  vector, which will be processed for transmission.
-------------------------------------------------------------------------------------------------------------------*/
std::vector<std::string> parseData() {
	//SendMessage(GetDlgItem(hDlg, IDC_WRITERFILE), EM_SETREADONLY, (LPARAM) TRUE, NULL);

    if (sendLines < 1) {
        sendLines = getLines(&hSendPanel);
    }

    rawStr = "";
	for (int i = 0; i < sendLines; i++) {
		rawStr += getLine(&hSendPanel, i, 0);
		updateProgressBar(i);
	}

	fileSize = rawStr.length();

    v_packets.clear();
	v_packets = parketize();
	return v_packets;
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		setupProgressBar
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void setupProgressBar(const HWND *box)
--					const HWND *box - the HWND to the edit control.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function sets up a progress bar showing the progress of transmission.
-------------------------------------------------------------------------------------------------------------------*/
void setupProgressBar(const HWND *box) {
	progressSize = GetWindowTextLength(*box);
	SendMessage(GetDlgItem(hDlg, IDC_FILEPROGRESS), PBM_SETRANGE32, 0, progressSize);
	SendMessage(GetDlgItem(hDlg, IDC_FILEPROGRESS), PBM_SETPOS, 0, 0);
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		updateProgressBar
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void updateProgressBar(int currentPos);
--					int currentPos - the position that donating the increment the progress bar proceeds.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function updates the file progress bar by the given position.
-------------------------------------------------------------------------------------------------------------------*/
void updateProgressBar(int currentPos) {
	SendMessage(GetDlgItem(hDlg, IDC_FILEPROGRESS), PBM_SETPOS, 
        SendMessage(GetDlgItem(hDlg, IDC_FILEPROGRESS), PBM_GETPOS, 0, 0) + currentPos, 0);
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		addLine
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void addLine(const HWND *box, string line)
--					const HWND *box - the HWND to the edit control.
--					string line - the line to be appended to the control.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function appends a single string to the end of the display area in the specified edit control.
--  This line will be ended with a new line character.
-------------------------------------------------------------------------------------------------------------------*/
void addLine(const HWND *box, string line) {
	int idx;
	idx = GetWindowTextLength(*box);
	std::regex e("(?!\r)\n");
	std::string tmp = std::regex_replace(line, e, "\r\n");
	SendMessage(*box, EM_SETSEL, (LPARAM)idx, (LPARAM)idx);
	SendMessage(*box, EM_REPLACESEL, 0, (LPARAM)tmp.c_str());
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		getLine
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		string getLine(const HWND *box, int line, int flag)
--					const HWND *box - the HWND to which Edit Box the line is being retrieved from.
--					int line - the line to be retrieved from the specified edit box.
--                  int flag - 0: send panel, 1: read panel
--
--	RETURNS:		The string retrieved from the specified line.
--
--	NOTES:
--  This function gets the string for the specified single line .
-------------------------------------------------------------------------------------------------------------------*/
string getLine(const HWND *box, int line, int flag) {
    string content;
	TCHAR *tmp = 0;
	size_t len = SendMessage(*box, EM_LINELENGTH, SendMessage(*box, EM_LINEINDEX, line, 0), NULL);
	tmp = new TCHAR[len];

    if (flag) {
        SendMessage(GetDlgItem(hDlg, IDC_READERFILE), EM_GETLINE, line, (LPARAM)tmp);
    }
    else {
        SendMessage(GetDlgItem(hDlg, IDC_WRITERFILE), EM_GETLINE, line, (LPARAM)tmp);
    }

	tmp[len] = '\0';
    content = tmp;
	return content;
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		initFileOpener
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void initFileOpener();
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function is called to setup the default flags for the file opener.
-------------------------------------------------------------------------------------------------------------------*/
void initFileOpener() {
	ZeroMemory(&fileName, sizeof(fileName));
	fileName.lStructSize = sizeof(fileName);
	fileName.hwndOwner = hDlg;
	fileName.lpstrFile = szFile;
	fileName.lpstrFile[0] = '\0';
	fileName.nMaxFile = sizeof(szFile);
	fileName.lpstrFilter = "Text Files\0*.txt\0";
	fileName.lpstrDefExt = "txt";
	fileName.nFilterIndex = 1;
	fileName.lpstrFileTitle = NULL;
	fileName.nMaxFileTitle = 0;
	fileName.lpstrInitialDir = NULL;
	setFileOpenerFlags(OPEN_BROWSER);
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		setFileOpenerFlags
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void setFileOpenerFlags(int browserAction)
--					int browserAction - a flag sets for file reading or writing.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function sets the correct flags for what type of file processing, ie. reading or
--  saving a file.
-------------------------------------------------------------------------------------------------------------------*/
void setFileOpenerFlags(int browserAction) {
	fileName.Flags = 0;
	switch (browserAction) {
	case SAVE_BROWSER:
        fileName.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING;
        break;
    case OPEN_BROWSER:
        fileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_HIDEREADONLY;
        break;
	}
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		updateStats
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void updateStats(int stat, int label)
--					int stat - the value to be displayed.
--					int label - the label to display the new value.
--
--	RETURNS:		VOID
--
--	NOTES:
--  This function updates the value for the specified the label.
-------------------------------------------------------------------------------------------------------------------*/
void updateStats(int stat, int label) {
	string temp;
	temp = to_string(stat);

    if (label == IDC_SDATA6) {
        temp += "%";
    }

	SendMessage(GetDlgItem(hDlg, label), WM_SETTEXT, NULL, (LPARAM) temp.c_str());
}

/*------------------------------------------------------------------------------------------------------------------
--	FUNCTION:		clearStats
--
--	DATE:			December 3, 2016
--
--	DESIGNER:		Alex Zielinski
--
--	PROGRAMMER:		Alex Zielinski
--
--	INTERFACE:		void clearStats()
--
--	RETURNS:		VOID
--
--	NOTES:
--  This functions clears all the stats on the GUI in the Stats section.
-------------------------------------------------------------------------------------------------------------------*/
void clearStats() {
	size_t label = LABEL_START_ID;
    for (; label < LABEL_START_ID + LABEL_COUNT; label++) {
        if (label == IDC_SDATA6) {
            SendMessage(GetDlgItem(hDlg, label), WM_SETTEXT, NULL, (LPARAM) "0%");
        }
        else {
            SendMessage(GetDlgItem(hDlg, label), WM_SETTEXT, NULL, (LPARAM) "0");
        }
    }
}