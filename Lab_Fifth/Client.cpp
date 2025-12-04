#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <conio.h>
#include "..\Server\Employee.h"

using namespace std;

int main() {
    cout << "Connecting to server...\n";
    if (!WaitNamedPipeA("\\\\.\\pipe\\Lab5Pipe", NMPWAIT_WAIT_FOREVER)) {
        cout << "Could not connect to server.\n";
        return 1;
    }

    HANDLE hPipe = CreateFileA(
        "\\\\.\\pipe\\Lab5Pipe",
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "Failed to open pipe. Error: " << GetLastError() << endl;
        return 1;
    }

    cout << "Connected to server.\n";

    while (true) {
        cout << "\nChoose option:\n";
        cout << "1. Modify record\n";
        cout << "2. Read record\n";
        cout << "3. Exit\n";

        int choice;
        cin >> choice;

        if (choice == 3) {
            Request req;
            req.cmd = CMD_EXIT;
            DWORD w;
            WriteFile(hPipe, &req, sizeof(req), &w, NULL);
            break;
        }

        if (choice != 1 && choice != 2) continue;

        Request req;
        req.cmd = (choice == 1) ? CMD_MODIFY : CMD_READ;

        cout << "Enter Employee ID: ";
        cin >> req.employeeID;

        DWORD bytesWritten, bytesRead;
        if (!WriteFile(hPipe, &req, sizeof(req), &bytesWritten, NULL)) {
            cout << "Server disconnected.\n";
            break;
        }

        Response resp;
        if (!ReadFile(hPipe, &resp, sizeof(resp), &bytesRead, NULL)) {
            cout << "Error reading from server.\n";
            break;
        }

        if (!resp.status) {
            cout << "Employee with ID " << req.employeeID << " not found.\n";
            continue;
        }

        cout << "ID: " << resp.record.num << "\nName: " << resp.record.name << "\nHours: " << resp.record.hours << endl;

        if (req.cmd == CMD_READ) {
            cout << "Press any key to finish reading...";
            _getch();
            char msg[] = "DONE";
            WriteFile(hPipe, msg, sizeof(msg), &bytesWritten, NULL);
        }
        else {
            cout << "Enter new Name: ";
            cin >> resp.record.name;
            cout << "Enter new Hours: ";
            cin >> resp.record.hours;

            WriteFile(hPipe, &resp.record, sizeof(employee), &bytesWritten, NULL);
            cout << "Updated record sent.\n";
            cout << "Press any key to finish...";
            _getch();
        }
    }

    CloseHandle(hPipe);
    return 0;
}