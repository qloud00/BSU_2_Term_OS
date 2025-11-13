#include <iostream>
#include <fstream>
#include <string>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

constexpr int MAX_MSG_LEN = 20;
constexpr int RECORD_SIZE = MAX_MSG_LEN + 1;

namespace bip = boost::interprocess;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Filename not provided.\n";
        return 1;
    }

    std::string filename = argv[1];

    try {
        bip::named_mutex file_mutex(bip::open_only, "file_mutex");
        bip::named_semaphore empty_slots(bip::open_only, "empty_slots");
        bip::named_semaphore filled_slots(bip::open_only, "filled_slots");

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
            std::cout << "Enter message (max " << MAX_MSG_LEN << " chars): ";
            std::getline(std::cin, msg);

            if (msg.empty()) {
                std::cout << "Message cannot be empty.\n";
                continue;
            }

            if (msg.length() > MAX_MSG_LEN) {
                msg = msg.substr(0, MAX_MSG_LEN);
                std::cout << "Message truncated to: " << msg << '\n';
            }

            if (!empty_slots.try_wait()) {
                std::cout << "Cannot send message now. Queue is full.\n";
                continue;
            }

            file_mutex.lock();

            std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
            if (!file) {
                file_mutex.unlock();
                std::cerr << "Failed to open file.\n";
                continue;
            }

            int writeIndex;
            file.read(reinterpret_cast<char*>(&writeIndex), sizeof(int));

            int offset = sizeof(int) * 2 + writeIndex * RECORD_SIZE;
            file.seekp(offset);

            char buffer[RECORD_SIZE] = { 0 };
            std::copy_n(msg.c_str(), msg.length(), buffer);
            buffer[msg.length()] = '\0';

            file.write(buffer, RECORD_SIZE);

            file.seekg(0, std::ios::end);
            auto fileSize = file.tellg();
            int recordCount = static_cast<int>((static_cast<size_t>(fileSize) - sizeof(int) * 2) / RECORD_SIZE);

            writeIndex = (writeIndex + 1) % recordCount;
            file.seekp(0);
            file.write(reinterpret_cast<char*>(&writeIndex), sizeof(int));

            file.close();
            file_mutex.unlock();

            filled_slots.post();

            std::cout << "Message sent: " << msg << '\n';
        }
    }
    catch (const bip::interprocess_exception& e) {
        std::cerr << "Sync error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}