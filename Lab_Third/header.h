#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>

extern int size;
extern int* arr;

extern std::mutex arrMutex;
extern std::mutex coutMutex;

extern std::condition_variable startCV;
extern std::mutex startMutex;
extern bool startSignal;

extern std::vector<std::unique_ptr<std::mutex>> threadMutexes;
extern std::vector<std::unique_ptr<std::condition_variable>> threadCVs;

extern std::vector<bool> threadSignals;
extern std::vector<bool> threadTerminate;
extern std::vector<bool> threadsFinished;

void marker(int threadIndex);