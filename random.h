#pragma once
#include "structs.h"
#include <cstdint>

#define RANDOM_CONTRACT_INDEX 3

struct GetRandom_input {
};

struct GetRandom_output {
    int64_t millisecond;
    int64_t second;
    int64_t minute;
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct SetRandom_input {
};

struct SetRandom_output {
};

void randomGet(const char* nodeIp, int nodePort, const char* seed);
