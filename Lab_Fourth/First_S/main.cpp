#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

const int MAX_MSG_LEN = 20;
const int RECORD_SIZE = MAX_MSG_LEN + 1;

int main() {
    std::string filename;
    int recordCount, senderCount;

    std::cout << "Enter binary file name: ";
    while (!(std::cin >> filename) || filename.empty()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input. Enter binary file name: ";
    }

    std::cout << "Enter number of records: ";
    while (!(std::cin >> recordCount) || recordCount <= 0) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input. Enter positive number: ";
    }

    std::cout << "Enter number of Sender processes: ";
    while (!(std::cin >> senderCount) || senderCount <= 0) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input. Enter positive number: ";
    }

    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    int writeIndex = 0, readIndex = 0;
    file.write(reinterpret_cast<char*>(&writeIndex), sizeof(int));
    file.write(reinterpret_cast<char*>(&readIndex), sizeof(int));
    std::vector<char> empty(RECORD_SIZE * recordCount, 0);
    file.write(empty.data(), empty.size());
    file.close();

    HANDLE emptySlots = CreateSemaphoreA(NULL, recordCount, recordCount, "emptySlots");
    HANDLE filledSlots = CreateSemaphoreA(NULL, 0, recordCount, "filledSlots");
    HANDLE fileMutex = CreateMutexA(NULL, FALSE, "fileMutex");

    for (int i = 0; i < senderCount; ++i) {
        std::string cmd = "Sender.exe " + filename;
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            std::cerr << "Failed to start Sender " << i << std::endl;
        }
        else {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    while (true) {
        int choice;
        std::cout << "\nEnter 1 to read message, 0 to exit: ";
        std::cin >> choice;

        if (choice == 0) break;
        if (choice != 1) {
            std::cout << "\nThere is no such choice!";
            continue;
        }
         
        DWORD result = WaitForSingleObject(filledSlots, 0); 
        if (result != WAIT_OBJECT_0) { 
            std::cout << "No messages available. Queue is empty.\n"; 
            continue;
        }

        WaitForSingleObject(fileMutex, INFINITE);

        std::fstream f(filename, std::ios::binary | std::ios::in | std::ios::out);
        f.seekg(sizeof(int)); 
        f.read(reinterpret_cast<char*>(&readIndex), sizeof(int));

        int offset = sizeof(int) * 2 + readIndex * RECORD_SIZE;
        f.seekg(offset);
        char buffer[RECORD_SIZE] = { 0 };
        f.read(buffer, RECORD_SIZE);

        std::cout << "Received: " << buffer;

        f.seekp(offset);
        std::vector<char> zero(RECORD_SIZE, 0);
        f.write(zero.data(), RECORD_SIZE);

        readIndex = (readIndex + 1) % recordCount;
        f.seekp(sizeof(int));
        f.write(reinterpret_cast<char*>(&readIndex), sizeof(int));

        f.close();
        ReleaseMutex(fileMutex);
        ReleaseSemaphore(emptySlots, 1, NULL);
    }

    CloseHandle(emptySlots);
    CloseHandle(filledSlots);
    CloseHandle(fileMutex);
    return 0;
}
