#ifndef MACROS_H__
#define MACROS_H__
#include <chrono>
#include <stdio.h>
#define LOG(stage, fmt, ...) do { \
    auto ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>( \
        std::chrono::steady_clock::now().time_since_epoch()).count(); \
    fprintf(stderr, "[%lld] %-14s " fmt "\n", ms, stage, ##__VA_ARGS__); \
} while(0)


#endif