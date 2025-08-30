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
    uint64_t offeredQU;
    uint64_t offeredAssetsAmount;
    AssetWithAmount offeredAssets[4];
    uint64_t requestedQU;
    uint64_t requestedAssetsAmount;
    AssetWithAmount requestedAssets[4];
};

struct EscrowGetDeals_input {
    uint8_t owner[32];
};
struct EscrowGetDeals_output {
    int64_t currentValue;
    uint64_t ownedDealsAmount;
    uint64_t proposedDealsAmount;
    uint64_t openedDealsAmount;
    struct AssetWithAmount
    {
        uint8_t issuer[32];
        uint64_t name;
        int64_t amount;
    };
    struct Deal
    {
        int64_t index;
        uint8_t acceptorId[32];
        uint64_t offeredQU;
        uint64_t offeredAssetsAmount;
        AssetWithAmount offeredAssets[4];
        uint64_t requestedQU;
        uint64_t requestedAssetsAmount;
        AssetWithAmount requestedAssets[4];
        int16_t creationEpoch;
        uint64_t ownerFee;
        uint64_t acceptorFee;
    };
    Deal ownedDeals[8];
    Deal proposedDeals[8];
    Deal openedDeals[32];
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct EscrowOperateDeal_input {
    int64_t index;
};

struct EscrowGetFreeAsset_input {
    uint8_t owner[32];
    uint8_t issuer[32];
    uint64_t name;
};
struct EscrowGetFreeAsset_output {
    int64_t freeAmount;
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
void escrowAcceptDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index);
void escrowMakeDealOpened(const char* nodeIp, int nodePort, const char* seed, const int64_t index);
void escrowCancelDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index);
void escrowOperateDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index, const int64_t fee, const unsigned short inputType);
void escrowGetFreeAsset(const char* nodeIp, int nodePort, const char* seed, const char* asset_name, const char* issuer);

EscrowGetDeals_output escrowGetDealsOutput(const char* nodeIp, int nodePort, const char* seed);
int64_t escrowGetRequestedQUForDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t& index);
int parseAssets(const std::string& inputStr, EscrowCreateDeal_input::AssetWithAmount* outputArray, const int& maxCount, uint64_t& QUAmount);
void printDeals(int64_t dealsAmount, const EscrowGetDeals_output::Deal* deals, const char* dealTypeName, const char* p1, const char* p2);
