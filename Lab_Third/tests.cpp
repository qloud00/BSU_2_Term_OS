#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>
#include "../header.h"

int size = 0;
int* arr = nullptr;

std::mutex arrMutex;
std::mutex coutMutex;

std::condition_variable startCV;
std::mutex startMutex;
bool startSignal = false;

std::vector<std::unique_ptr<std::mutex>> threadMutexes;
std::vector<std::unique_ptr<std::condition_variable>> threadCVs;

std::vector<bool> threadSignals;
std::vector<bool> threadTerminate;
std::vector<bool> threadsFinished;

TEST(MarkerThreadTest, ThreadTerminationAndCleanup) {
    size = 10;
    arr = new int[size]();

    int countOfThreads = 1;

    threadSignals.resize(countOfThreads, false);
    threadTerminate.resize(countOfThreads, false);
    threadsFinished.resize(countOfThreads, false);

    threadCVs.resize(countOfThreads);
    threadMutexes.resize(countOfThreads);
    for (int i = 0; i < countOfThreads; i++) {
        threadCVs[i] = std::make_unique<std::condition_variable>();
        threadMutexes[i] = std::make_unique<std::mutex>();
    }

    std::thread testThread(marker, 0);

    {
        std::lock_guard<std::mutex> lock(startMutex);
        startSignal = true;
    }
    startCV.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> lock(*threadMutexes[0]);
        threadTerminate[0] = true;
    }
    threadCVs[0]->notify_one();

    testThread.join();

    for (int i = 0; i < size; i++) {
        EXPECT_EQ(arr[i], 0);
    }

    delete[] arr;
}

TEST(MarkerThreadTest, MultipleThreadsCleanup) {
    size = 5;
    arr = new int[size]();

    int countOfThreads = 2;

    threadSignals.resize(countOfThreads, false);
    threadTerminate.resize(countOfThreads, false);
    threadsFinished.resize(countOfThreads, false);

    threadCVs.resize(countOfThreads);
    threadMutexes.resize(countOfThreads);
    for (int i = 0; i < countOfThreads; i++) {
        threadCVs[i] = std::make_unique<std::condition_variable>();
        threadMutexes[i] = std::make_unique<std::mutex>();
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < countOfThreads; i++) {
        threads.emplace_back(marker, i);
    }

    {
        std::lock_guard<std::mutex> lock(startMutex);
        startSignal = true;
    }
    startCV.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (int i = 0; i < countOfThreads; i++) {
        {
            std::lock_guard<std::mutex> lock(*threadMutexes[i]);
            threadTerminate[i] = true;
        }
        threadCVs[i]->notify_one();
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int i = 0; i < size; i++) {
        EXPECT_EQ(arr[i], 0);
    }

    delete[] arr;
}