#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

namespace bip = boost::interprocess;

const int MAX_MSG_LEN = 20;
const int RECORD_SIZE = MAX_MSG_LEN + 1;
const int HEADER_SIZE = sizeof(int) * 2;
const std::string TEST_FILE = "test_fifo.bin";
const std::string MUTEX_NAME = "test_mutex";
const std::string EMPTY_SEM_NAME = "test_empty";
const std::string FILLED_SEM_NAME = "test_filled";

void initTestFile(int recordCount) {
    std::ofstream file(TEST_FILE, std::ios::binary | std::ios::trunc);
    int writeIndex = 0, readIndex = 0;
    file.write(reinterpret_cast<char*>(&writeIndex), sizeof(int));
    file.write(reinterpret_cast<char*>(&readIndex), sizeof(int));
    std::vector<char> empty(RECORD_SIZE * recordCount, 0);
    file.write(empty.data(), empty.size());
    file.close();
}

void writeMessage(const std::string& msg, int recordCount) {
    bip::named_mutex file_mutex(bip::open_only, MUTEX_NAME.c_str());
    bip::named_semaphore empty_slots(bip::open_only, EMPTY_SEM_NAME.c_str());
    bip::named_semaphore filled_slots(bip::open_only, FILLED_SEM_NAME.c_str());

    empty_slots.wait();
    file_mutex.lock();

    std::fstream f(TEST_FILE, std::ios::binary | std::ios::in | std::ios::out);
    int writeIndex;
    f.read(reinterpret_cast<char*>(&writeIndex), sizeof(int));

    std::string trimmed = msg.substr(0, MAX_MSG_LEN);
    char buffer[RECORD_SIZE] = { 0 };
    strncpy_s(buffer, sizeof(buffer), trimmed.c_str(), MAX_MSG_LEN);
    buffer[MAX_MSG_LEN] = '\0';

    int offset = HEADER_SIZE + writeIndex * RECORD_SIZE;
    f.seekp(offset);
    f.write(buffer, RECORD_SIZE);

    writeIndex = (writeIndex + 1) % recordCount;
    f.seekp(0);
    f.write(reinterpret_cast<char*>(&writeIndex), sizeof(int));
    f.close();

    file_mutex.unlock();
    filled_slots.post();
}

std::string readMessage(int recordCount) {
    bip::named_mutex file_mutex(bip::open_only, MUTEX_NAME.c_str());
    bip::named_semaphore empty_slots(bip::open_only, EMPTY_SEM_NAME.c_str());
    bip::named_semaphore filled_slots(bip::open_only, FILLED_SEM_NAME.c_str());

    if (!filled_slots.try_wait()) {
        return "";
    }

    file_mutex.lock();

    std::fstream f(TEST_FILE, std::ios::binary | std::ios::in | std::ios::out);
    f.seekg(sizeof(int));
    int readIndex;
    f.read(reinterpret_cast<char*>(&readIndex), sizeof(int));

    int offset = HEADER_SIZE + readIndex * RECORD_SIZE;
    f.seekg(offset);
    char buffer[RECORD_SIZE] = { 0 };
    f.read(buffer, RECORD_SIZE);

    readIndex = (readIndex + 1) % recordCount;
    f.seekp(sizeof(int));
    f.write(reinterpret_cast<char*>(&readIndex), sizeof(int));
    f.close();

    file_mutex.unlock();
    empty_slots.post();

    return std::string(buffer);
}

class BoostRingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        bip::named_mutex::remove(MUTEX_NAME.c_str());
        bip::named_semaphore::remove(EMPTY_SEM_NAME.c_str());
        bip::named_semaphore::remove(FILLED_SEM_NAME.c_str());

        initTestFile(recordCount);

        bip::named_mutex file_mutex(bip::create_only, MUTEX_NAME.c_str());
        bip::named_semaphore empty_slots(bip::create_only, EMPTY_SEM_NAME.c_str(), recordCount);
        bip::named_semaphore filled_slots(bip::create_only, FILLED_SEM_NAME.c_str(), 0);
    }

    void TearDown() override {
        bip::named_mutex::remove(MUTEX_NAME.c_str());
        bip::named_semaphore::remove(EMPTY_SEM_NAME.c_str());
        bip::named_semaphore::remove(FILLED_SEM_NAME.c_str());
        std::remove(TEST_FILE.c_str());
    }

    int recordCount = 3;
};

TEST_F(BoostRingBufferTest, FIFOOrder) {
    writeMessage("first", recordCount);
    writeMessage("second", recordCount);
    writeMessage("third", recordCount);

    EXPECT_EQ(readMessage(recordCount), "first");
    EXPECT_EQ(readMessage(recordCount), "second");
    EXPECT_EQ(readMessage(recordCount), "third");
}

TEST_F(BoostRingBufferTest, Truncation) {
    int recordCount = 1;
    initTestFile(recordCount);

    writeMessage("this message is too long and will be cut", recordCount);
    EXPECT_EQ(readMessage(recordCount), "this message is too ");
}

TEST_F(BoostRingBufferTest, WrapAround) {
    int recordCount = 2;
    initTestFile(recordCount);

    writeMessage("A", recordCount);
    writeMessage("B", recordCount);
    EXPECT_EQ(readMessage(recordCount), "A");

    writeMessage("C", recordCount);
    EXPECT_EQ(readMessage(recordCount), "B");
    EXPECT_EQ(readMessage(recordCount), "C");
}

TEST_F(BoostRingBufferTest, EmptyReadReturnsZeros) {
    int recordCount = 2;
    initTestFile(recordCount);

    std::string msg = readMessage(recordCount);
    EXPECT_EQ(msg, "");
}

TEST_F(BoostRingBufferTest, MultipleWrapAroundCycles) {
    int recordCount = 2;
    initTestFile(recordCount);

    for (int i = 0; i < 6; ++i) {
        writeMessage("msg" + std::to_string(i), recordCount);
        std::string expected = "msg" + std::to_string(i);
        EXPECT_EQ(readMessage(recordCount), expected);
    }
}

TEST_F(BoostRingBufferTest, MessageExactly20Chars) {
    int recordCount = 1;
    initTestFile(recordCount);

    std::string msg = "12345678901234567890";
    writeMessage(msg, recordCount);
    EXPECT_EQ(readMessage(recordCount), msg);
}

TEST_F(BoostRingBufferTest, MessageWithSpecialChars) {
    int recordCount = 1;
    initTestFile(recordCount);

    std::string msg = "!@#$%^&*()_+=-{}[]";
    writeMessage(msg, recordCount);
    EXPECT_EQ(readMessage(recordCount), msg);
}

TEST_F(BoostRingBufferTest, ReadAfterWriteIndexWrap) {
    int recordCount = 2;
    initTestFile(recordCount);

    writeMessage("first", recordCount);
    writeMessage("second", recordCount);
    EXPECT_EQ(readMessage(recordCount), "first");

    writeMessage("third", recordCount);
    EXPECT_EQ(readMessage(recordCount), "second");
    EXPECT_EQ(readMessage(recordCount), "third");
}