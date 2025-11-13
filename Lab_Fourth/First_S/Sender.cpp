#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

const int MAX_MSG_LEN = 20;
const int RECORD_SIZE = MAX_MSG_LEN + 1;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Filename not provided.\n";
        return 1;
    }

    std::string filename = argv[1];

    HANDLE emptySlots = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "emptySlots");
    HANDLE filledSlots = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "filledSlots");
    HANDLE fileMutex = OpenMutexA(SYNCHRONIZE, FALSE, "fileMutex");

    if (!emptySlots || !filledSlots || !fileMutex) {
        std::cerr << "Failed to open synchronization objects.\n";
        return 1;
    }

    while (true) {
        int choice;
        std::cout << "\nEnter 1 to send message, 0 to exit: ";
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 0) break;
        if (choice != 1) {
            std::cout << "\nThere is no such choice!";
            continue;
        }

        std::string msg;
        std::cout << "Enter message (max 20 chars): ";
        std::getline(std::cin, msg);

        if (msg.empty()) {
            std::cout << "Message cannot be empty.\n";
            continue;
        }

        if (msg.length() > MAX_MSG_LEN) {
            msg = msg.substr(0, MAX_MSG_LEN);
            std::cout << "Message truncated to: " << msg << std::endl;
        }

        DWORD result = WaitForSingleObject(emptySlots, 0); 
        if (result != WAIT_OBJECT_0) {
            std::cout << "Cannot send message now. Queue is full.\n";
            continue;
        }

        WaitForSingleObject(fileMutex, INFINITE);

        std::fstream f(filename, std::ios::binary | std::ios::in | std::ios::out);
        int writeIndex;
        f.read(reinterpret_cast<char*>(&writeIndex), sizeof(int));

        int offset = sizeof(int) * 2 + writeIndex * RECORD_SIZE;
        f.seekp(offset);
        char buffer[RECORD_SIZE] = { 0 };
        strncpy_s(buffer, sizeof(buffer), msg.c_str(), MAX_MSG_LEN);
        buffer[MAX_MSG_LEN] = '\0'; 
        f.write(buffer, RECORD_SIZE);

        f.seekg(0, std::ios::end);
        std::streamoff fileSize = f.tellg();
        int recordCount = static_cast<int>((fileSize - sizeof(int) * 2) / RECORD_SIZE);

        writeIndex = (writeIndex + 1) % recordCount;
        f.seekp(0);
        f.write(reinterpret_cast<char*>(&writeIndex), sizeof(int));

        f.close();
        ReleaseMutex(fileMutex);
        ReleaseSemaphore(filledSlots, 1, NULL);

        std::cout << "Message sent: " << msg << std::endl;
    }

    CloseHandle(emptySlots);
    CloseHandle(filledSlots);
    CloseHandle(fileMutex);
    return 0;
}
