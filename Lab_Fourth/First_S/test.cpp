#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>

const int MAX_MSG_LEN = 20;
const int RECORD_SIZE = MAX_MSG_LEN + 1;
const int HEADER_SIZE = sizeof(int) * 2;
const std::string TEST_FILE = "test_fifo.bin";

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
}

std::string readMessage(int recordCount) {
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

    return std::string(buffer);
    
}

TEST(RingBufferTest, FIFOOrder) {
    int recordCount = 3;
    initTestFile(recordCount);

    writeMessage("first", recordCount);
    writeMessage("second", recordCount);
    writeMessage("third", recordCount);

    EXPECT_EQ(readMessage(recordCount), "first");
    EXPECT_EQ(readMessage(recordCount), "second");
    EXPECT_EQ(readMessage(recordCount), "third");
}

TEST(RingBufferTest, Truncation) {
    int recordCount = 1;
    initTestFile(recordCount);

    writeMessage("this message is too long and will be cut", recordCount);
    EXPECT_EQ(readMessage(recordCount), "this message is too ");
}

TEST(RingBufferTest, WrapAround) {
    int recordCount = 2;
    initTestFile(recordCount);

    writeMessage("A", recordCount);
    writeMessage("B", recordCount);
    EXPECT_EQ(readMessage(recordCount), "A");

    writeMessage("C", recordCount);
    EXPECT_EQ(readMessage(recordCount), "B");
    EXPECT_EQ(readMessage(recordCount), "C");
}

TEST(RingBufferTest, EmptyReadReturnsZeros) {
    int recordCount = 2;
    initTestFile(recordCount);

    std::string msg = readMessage(recordCount);
    EXPECT_EQ(msg, "");
}

TEST(RingBufferTest, MultipleWrapAroundCycles) {
    int recordCount = 2;
    initTestFile(recordCount);

    for (int i = 0; i < 6; ++i) {
        writeMessage("msg" + std::to_string(i), recordCount);
        std::string expected = "msg" + std::to_string(i);
        EXPECT_EQ(readMessage(recordCount), expected);
    }
}

TEST(RingBufferTest, MessageExactly20Chars) {
    int recordCount = 1;
    initTestFile(recordCount);

    std::string msg = "12345678901234567890"; 
    writeMessage(msg, recordCount);
    EXPECT_EQ(readMessage(recordCount), msg);
}

TEST(RingBufferTest, MessageWithSpecialChars) {
    int recordCount = 1;
    initTestFile(recordCount);

    std::string msg = "!@#$%^&*()_+=-{}[]";
    writeMessage(msg, recordCount);
    EXPECT_EQ(readMessage(recordCount), msg);
}

TEST(RingBufferTest, ReadAfterWriteIndexWrap) {
    int recordCount = 2;
    initTestFile(recordCount);

    writeMessage("first", recordCount);
    writeMessage("second", recordCount);
    EXPECT_EQ(readMessage(recordCount), "first");

    writeMessage("third", recordCount); 
    EXPECT_EQ(readMessage(recordCount), "second");
    EXPECT_EQ(readMessage(recordCount), "third");
}
