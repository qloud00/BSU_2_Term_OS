#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <fstream> 
#include <string>
#include <conio.h>
#include "Employee.h"

using namespace std;

string filename;
int clientCount;

CRITICAL_SECTION csState;
HANDLE hStateChanged;

map<int, int> activeReaders;
map<int, bool> activeWriter;

void PrintFile() {
    ifstream fin(filename, ios::binary);
    if (!fin.is_open()) return;

    employee emp;
    cout << "\n--- File Content ---\n";
    while (fin.read((char*)&emp, sizeof(employee))) {
        cout << "ID: " << emp.num << ", Name: " << emp.name << ", Hours: " << emp.hours << endl;
    }
    cout << "--------------------\n";
    fin.close();
}

int FindRecord(int id, employee& outEmp) {
    ifstream fin(filename, ios::binary);
    if (!fin.is_open()) return -1;

    employee emp;
    int pos = 0;
    while (fin.read((char*)&emp, sizeof(employee))) {
        if (emp.num == id) {
            outEmp = emp;
            return pos;
        }
        pos++;
    }
    return -1;
}

void WriteRecord(int pos, employee emp) {
    fstream f(filename, ios::binary | ios::in | ios::out);
    if (f.is_open()) {
        f.seekp(pos * sizeof(employee), ios::beg);
        f.write((char*)&emp, sizeof(employee));
        f.close();
    }
}

DWORD WINAPI ClientHandler(LPVOID lpParam) {
    HANDLE hPipe = (HANDLE)lpParam;
    Request req;
    DWORD bytesRead, bytesWritten;

    while (true) {
        if (!ReadFile(hPipe, &req, sizeof(Request), &bytesRead, NULL) || bytesRead == 0) {
            break;
        }

        if (req.cmd == CMD_EXIT) {
            break;
        }

        int id = req.employeeID;
        employee empData;
        int pos = FindRecord(id, empData);
        Response resp;
        resp.status = (pos != -1);
        resp.record = empData;

        if (!resp.status) {
            WriteFile(hPipe, &resp, sizeof(Response), &bytesWritten, NULL);
            continue;
        }

        if (req.cmd == CMD_READ) {
            while (true) {
                EnterCriticalSection(&csState);
                if (!activeWriter[id]) {
                    activeReaders[id]++;
                    LeaveCriticalSection(&csState);
                    break;
                }
                LeaveCriticalSection(&csState);
                WaitForSingleObject(hStateChanged, INFINITE);
            }
        }
        else if (req.cmd == CMD_MODIFY) {
            while (true) {
                EnterCriticalSection(&csState);
                if (!activeWriter[id] && activeReaders[id] == 0) {
                    activeWriter[id] = true;
                    LeaveCriticalSection(&csState);
                    break;
                }
                LeaveCriticalSection(&csState);
                WaitForSingleObject(hStateChanged, INFINITE);
            }
        }

        WriteFile(hPipe, &resp, sizeof(Response), &bytesWritten, NULL);

        if (req.cmd == CMD_READ) {
            char buffer[10];
            ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);

            EnterCriticalSection(&csState);
            activeReaders[id]--;
            LeaveCriticalSection(&csState);

            SetEvent(hStateChanged);
        }
        else if (req.cmd == CMD_MODIFY) {
            employee newEmp;
            if (ReadFile(hPipe, &newEmp, sizeof(employee), &bytesRead, NULL)) {
                WriteRecord(pos, newEmp);
            }

            EnterCriticalSection(&csState);
            activeWriter[id] = false;
            LeaveCriticalSection(&csState);
            SetEvent(hStateChanged);
        }
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    cout << "Client disconnected.\n";
    return 0;
}

#ifndef UNIT_TEST

int main() {
    InitializeCriticalSection(&csState);
    hStateChanged = CreateEvent(NULL, FALSE, FALSE, NULL);

    cout << "Enter filename: ";
    cin >> filename;

    cout << "Enter number of employees: ";
    int empCount;
    cin >> empCount;

    ofstream fout(filename, ios::binary);
    for (int i = 0; i < empCount; ++i) {
        employee emp;
        emp.num = i + 1;
        sprintf_s(emp.name, "Emp%d", i + 1);
        emp.hours = 0.0;

        cout << "Enter ID, Name, Hours for employee " << i + 1 << ":\n";
        cin >> emp.num >> emp.name >> emp.hours;

        fout.write((char*)&emp, sizeof(employee));
    }
    fout.close();

    PrintFile();

    cout << "Enter number of clients: ";
    cin >> clientCount;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    for (int i = 0; i < clientCount; ++i) {
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        string cmdLine = "Client.exe";

        if (!CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            cout << "Could not start Client.exe.\n";
            cout << "Error: " << GetLastError() << endl;
        }
        else {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    HANDLE hPipe;
    int connectedClients = 0;

    cout << "Server started. Waiting for connections...\n";

    while (true) {
        hPipe = CreateNamedPipeA(
            "\\\\.\\pipe\\Lab5Pipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            512, 512, 0, NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            cout << "CreateNamedPipe failed.\n";
            return 1;
        }

        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            cout << "Client connected.\n";
            CreateThread(NULL, 0, ClientHandler, (LPVOID)hPipe, 0, NULL);
        }
        else {
            CloseHandle(hPipe);
        }
    }

    DeleteCriticalSection(&csState);
    CloseHandle(hStateChanged);
    return 0;
}
#endif