#pragma once
#include "structs.h"
#include <cstdint>

#define RANDOM_CONTRACT_INDEX 3

struct EscrowCreateDeal_input {
    int64_t delta;
    uint8_t acceptorId[32];
    struct AssetWithAmount
    {
        uint8_t issuer[32];
        uint64_t name;
        int64_t amount;
    };
    AssetWithAmount offeredAssets[4];
    int8_t offeredAssetsAmount;
    AssetWithAmount requestedAssets[4];
    int8_t requestedAssetsAmount;
};
struct EscrowCreateDeal_output {
    int64_t currentValue;
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct EscrowGetDeals_input {
    uint8_t owner[32];
};
struct EscrowGetDeals_output {
    int64_t currentValue;
    int64_t ownedDealsAmount;
    int64_t proposedDealsAmount;
    struct DealEntity
    {
        struct AssetWithAmount
        {
            uint8_t issuer[32];
            uint64_t name;
            int64_t amount;
        };
        int64_t index;
        struct
        {
            uint8_t acceptorId[32];
            AssetWithAmount offeredAssets[4];
            int8_t offeredAssetsAmount;
            AssetWithAmount requestedAssets[4];
            int8_t requestedAssetsAmount;
        } deal;
    };
    DealEntity ownedDeals[8];
    DealEntity proposedDeals[8];
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct EscrowAcceptDeal_input {
    int64_t index;
};
struct EscrowAcceptDeal_output {
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

void escrowCreateDeal(const char* nodeIp, int nodePort, const char* seed,
    int64_t delta,
    const char* acceptorId,
    const char* offeredAssetsCommaSeparated,
    const char* requestedAssetsCommaSeparated);
void escrowGetDeals(const char* nodeIp, int nodePort, const char* seed);
void escrowAcceptDeal(const char* nodeIp, int nodePort, const char* seed, int64_t index);

int parseAssets(const std::string& inputStr, EscrowCreateDeal_input::AssetWithAmount* outputArray, int maxCount);
void printDeals(int64_t dealsAmount, const EscrowGetDeals_output::DealEntity* etities);
