#pragma once
#include "structs.h"
#include <cstdint>

#define RANDOM_CONTRACT_INDEX 11

struct Stake_input {
    int64_t millions;
};

struct TransferMBond_input {
    uint8_t newOwnerAndPossessor[32];
    int64_t epoch;
    int64_t numberOfMBonds;
};
struct TransferMBond_output {
    int64_t transferredMBonds;
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct OrderOperation_input
{
    int64_t epoch;
    int64_t price;
    int64_t numberOfMBonds;
};
struct OrderOperation_output
{
    int64_t mBondsAmount;
};

struct GetInfoPerEpoch_input {
    int64_t epoch;
};
struct GetInfoPerEpoch_output {
    uint64_t stakersAmount;
    int64_t totalStaked;
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

struct GetOrders_input
{
    int64_t epoch;
    int64_t asksOffset;
    int64_t bidsOffset;
};
struct GetOrders_output
{
    uint64_t counter;
    struct Order
    {
        uint8_t owner[32];
        int64_t epoch;
        int64_t numberOfMBonds;
        int64_t price;
    };
    Order askOrders[256];
    Order bidOrders[256];
    static constexpr unsigned char type() {
        return RespondContractFunction::type();
    }
};

void qbondStake(const char* nodeIp, int nodePort, const char* seed, const int64_t millionsOfQu);
void qbondTransfer(const char* nodeIp, int nodePort, const char* seed, const char* targetIdentity, const int64_t epoch, const int64_t mbondsAmount);
void qbondAddAskOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount);
void qbondRemoveAskOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount);
void qbondAddBidOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount);
void qbondRemoveBidOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount);
void qbondGetInfoPerEpoch(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch);
void qbondGetOrders(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t asksOffset, const int64_t bidsOffset);