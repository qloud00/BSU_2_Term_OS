#include <iostream>
#include <fstream>
#include <string>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

constexpr int MAX_MSG_LEN = 20;
constexpr int RECORD_SIZE = MAX_MSG_LEN + 1;

namespace bip = boost::interprocess;

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

    try {
        bip::named_mutex::remove("file_mutex");
        bip::named_semaphore::remove("empty_slots");
        bip::named_semaphore::remove("filled_slots");

        bip::named_mutex file_mutex(bip::create_only, "file_mutex");
        bip::named_semaphore empty_slots(bip::create_only, "empty_slots", recordCount);
        bip::named_semaphore filled_slots(bip::create_only, "filled_slots", 0);

        for (int i = 0; i < senderCount; ++i) {
            std::string command = "start Sender.exe " + filename;
            if (system(command.c_str()) != 0) {
                std::cerr << "Failed to start Sender " << i << '\n';
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

            if (!filled_slots.try_wait()) {
                std::cout << "No messages available. Queue is empty.\n";
                continue;
            }

            file_mutex.lock();

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
            file_mutex.unlock();
            empty_slots.post();
        }

        bip::named_mutex::remove("file_mutex");
        bip::named_semaphore::remove("empty_slots");
        bip::named_semaphore::remove("filled_slots");

    }
    catch (const bip::interprocess_exception& e) {
        std::cerr << "Sync error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
