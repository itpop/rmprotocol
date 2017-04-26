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

void clearBox(const HWND *box) {
    SetWindowText(*box, "");
}

int getLines(const HWND *box) {
    return SendMessage(*box, EM_GETLINECOUNT, NULL, NULL);
}

std::vector<std::string> parseData() {
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

void updateStats(int stat, int label) {
    string temp;
    temp = to_string(stat);

    if (label == IDC_SDATA6) {
        temp += "%";
    }

    SendMessage(GetDlgItem(hDlg, label), WM_SETTEXT, NULL, (LPARAM) temp.c_str());
}

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