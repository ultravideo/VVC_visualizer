//
// Created by Jovasa on 9.1.2024.
//

#ifndef VISUALIZER_UTIL_H
#define VISUALIZER_UTIL_H


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
