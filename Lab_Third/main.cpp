#include "header.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

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

int main() {
    std::cout << "Enter array size: ";
    std::cin >> size;
    arr = new int[size]();

    int countOfThreads = 0;
    std::cout << "Enter number of marker threads: ";
    std::cin >> countOfThreads;

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

    while (1)
    {
        bool allSignaled = false;
        while (!allSignaled) {
            allSignaled = true;
            for (int i = 0; i < countOfThreads; i++) {
                if (!threadsFinished[i] && !threadSignals[i]) {
                    allSignaled = false;
                    break;
                }
            }
            if (!allSignaled) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        {
            std::lock_guard<std::mutex> lock(arrMutex);
            std::cout << "Current array: ";
            for (int i = 0; i < size; i++) {
                std::cout << arr[i] << " ";
            }
            std::cout << "\n";
        }

        int stopIndex;
        std::cout << "Enter marker ID to terminate: ";
        std::cin >> stopIndex;

        if (stopIndex < 1 || stopIndex > countOfThreads || threadsFinished[stopIndex - 1]) {
            std::cout << "Invalid marker ID!\n";

            for (int i = 0; i < countOfThreads; i++) {
                if (!threadsFinished[i]) {
                    std::lock_guard<std::mutex> lock(*threadMutexes[i]);
                    threadCVs[i]->notify_one();
                }
            }
            continue;
        }

        int stopActualIndex = stopIndex - 1;

        {
            std::lock_guard<std::mutex> lock(*threadMutexes[stopActualIndex]);
            threadTerminate[stopActualIndex] = true;
        }
        threadCVs[stopActualIndex]->notify_one();

        threads[stopActualIndex].join();
        threadsFinished[stopActualIndex] = true;

        {
            std::lock_guard<std::mutex> lock(arrMutex);
            std::cout << "Array after termination of marker " << stopIndex << ": ";
            for (int i = 0; i < size; i++) {
                std::cout << arr[i] << " ";
            }
            std::cout << "\n";
        }

        bool allDone = true;
        for (int i = 0; i < countOfThreads; i++) {
            if (!threadsFinished[i]) {
                allDone = false;
                break;
            }
        }

        if (allDone) {
            break;
        }

        for (int i = 0; i < countOfThreads; i++) {
            if (!threadsFinished[i]) {
                std::lock_guard<std::mutex> lock(*threadMutexes[i]);
                threadSignals[i] = false;
                threadCVs[i]->notify_one();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    delete[] arr;
    std::cout << "All threads finished. Program completed." << std::endl;
    return 0;
}