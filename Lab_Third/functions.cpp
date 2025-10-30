#include "header.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <chrono>

void marker(int threadIndex) {
    int markedCount = 0;
    std::vector<int> markedIndices;

    {
        std::unique_lock<std::mutex> lock(startMutex);
        startCV.wait(lock, [] { return startSignal; });
    }

    srand(threadIndex + 1);

    while (true) {
        int randomNumber = rand() % size;

        {
            std::lock_guard<std::mutex> lock(arrMutex);

            if (arr[randomNumber] == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                arr[randomNumber] = threadIndex + 1;
                markedIndices.push_back(randomNumber);
                markedCount++;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                std::lock_guard<std::mutex> coutLock(coutMutex);
                std::cout << "Marker ID " << threadIndex + 1 << "; marked " << markedCount
                          << " elements; can't mark arr[" << randomNumber << "]\n";
            }
            else
            {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                std::cout << "Marker ID " << threadIndex + 1 << "; marked " << markedCount
                          << " elements; can't mark arr[" << randomNumber << "]\n";
            }
        }

        {
            std::lock_guard<std::mutex> lock(*threadMutexes[threadIndex]);
            threadSignals[threadIndex] = true;
        }

        {
            std::unique_lock<std::mutex> lock(*threadMutexes[threadIndex]);
            threadCVs[threadIndex]->wait(lock);
        }

        if (threadTerminate[threadIndex]) {
            std::lock_guard<std::mutex> lock(arrMutex);

            for (int i : markedIndices) {
                if (i >= 0 && i < size) {
                    arr[i] = 0;
                }
            }
            threadsFinished[threadIndex] = true;
            return;
        }
        else {
            threadSignals[threadIndex] = false;
        }
    }
}