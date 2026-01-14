#pragma once
#include <chrono>

#define TICK(x) auto x = std::chrono::high_resolution_clock::now()

#define TOCK(x) \
    do { \
        auto _end = std::chrono::high_resolution_clock::now(); \
        auto _duration = std::chrono::duration_cast<std::chrono::milliseconds>(_end - x).count(); \
        SPDLOG_INFO("{} time: {} ms", #x, _duration); \
    } while(0)

class Timer{
public:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> endTime;

    void start(){
        startTime = std::chrono::high_resolution_clock::now();
    }

    void stop(){
        endTime = std::chrono::high_resolution_clock::now();
    }

    int64_t getMsDuration(){
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    }
};