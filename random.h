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
    int64_t offeredQU;
    AssetWithAmount offeredAssets[4];
    int8_t offeredAssetsAmount;
    int64_t requestedQU;
    AssetWithAmount requestedAssets[4];
    int8_t requestedAssetsAmount;
};

struct EscrowGetDeals_input {
    uint8_t owner[32];
};
struct EscrowGetDeals_output {
    int64_t currentValue;
    int64_t ownedDealsAmount;
    int64_t proposedDealsAmount;
    int64_t openedDealsAmount;
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
            int64_t offeredQU;
            AssetWithAmount offeredAssets[4];
            int8_t offeredAssetsAmount;
            int64_t requestedQU;
            AssetWithAmount requestedAssets[4];
            int8_t requestedAssetsAmount;
        } deal;
    };
    DealEntity ownedDeals[8];
    DealEntity proposedDeals[8];
    DealEntity openedDeals[32];
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct EscrowOperateDeal_input {
    int64_t index;
};

void escrowCreateDeal(const char* nodeIp, int nodePort, const char* seed,
    int64_t delta,
    const char* acceptorId,
    const char* offeredAssetsCommaSeparated,
    const char* requestedAssetsCommaSeparated);
void escrowGetDeals(const char* nodeIp, int nodePort, const char* seed);
void escrowAcceptDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index);
void escrowMakeDealOpened(const char* nodeIp, int nodePort, const char* seed, const int64_t index);
void escrowCancelDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index);
void escrowOperateDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index, const int64_t fee, const unsigned short inputType);

EscrowGetDeals_output escrowGetDealsOutput(const char* nodeIp, int nodePort, const char* seed);
int64_t escrowGetRequestedQUForDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t& index);
int parseAssets(const std::string& inputStr, EscrowCreateDeal_input::AssetWithAmount* outputArray, const int& maxCount, int64_t& QUAmount);
void printDeals(int64_t dealsAmount, const EscrowGetDeals_output::DealEntity* entities, const char* dealTypeName, const char* p1, const char* p2);
