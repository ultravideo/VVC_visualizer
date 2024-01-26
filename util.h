//
// Created by Jovasa on 9.1.2024.
//

#ifndef VISUALIZER_UTIL_H
#define VISUALIZER_UTIL_H

#ifdef _MSC_VER
#include <windows.h>
typedef LARGE_INTEGER TimeStamp;
static LARGE_INTEGER Frequency;
//QueryPerformanceFrequency(&Frequency);

#define GET_TIME(ts, rs) \
    do {static TimeStamp StartingTime; \
QueryPerformanceCounter(&StartingTime); \
rs = StartingTime.QuadPart * 1000000000 / Frequency.QuadPart; } while(0)
#else
typedef struct timespec TimeStamp;
#define GET_TIME(ts, rs) \
    do {clock_gettime(CLOCK_REALTIME, &ts); \
    rs = ts.tv_sec * 1000000000 + ts.tv_nsec;} while(0)
#endif


template<typename T>
T ceil_div(T a, T b) {
    return (a + b - 1) / b;
}

template<typename T>
T floor_div(T a, T b) {
    return a / b;
}

template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

template<typename T>
T clamp(T value, T min, T max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

#endif //VISUALIZER_UTIL_H
