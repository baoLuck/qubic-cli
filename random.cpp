#include "random.h"
#include "structs.h"
#include "logger.h"
#include "connection.h"
#include "walletUtils.h"
#include "nodeUtils.h"
#include "keyUtils.h"
#include "K12AndKeyUtil.h"

#include <sstream>
#include <iostream>

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdint>

#define QBOND_CONTRACT_INDEX 11

#define QBOND_STAKE 1
#define QBOND_TRANSFER 2
#define QBOND_ADD_ASK_ORDER 3
#define QBOND_REMOVE_ASK_ORDER 4
#define QBOND_ADD_BID_ORDER 5
#define QBOND_REMOVE_BID_ORDER 6

#define QBOND_GET_INFO_PER_EPOCH 1
#define QBOND_GET_ASK_ORDERS 2

constexpr uint64_t QBOND_BASE_STAKE_AMOUNT = 1000000ULL;

void qbondStake(const char* nodeIp, int nodePort, const char* seed, const int64_t millionsOfQu)
{
    Stake_input input;
    input.millions = millionsOfQu;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    uint8_t destPublicKey[32] = { 0 };
    uint8_t digest[32];
    uint8_t signature[64];
    char publicIdentity[128] = { 0 };
    char txHash[128] = { 0 };
    const bool isLowerCase = false;

    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    memset(destPublicKey, 0, 32);
    ((uint64_t*) destPublicKey)[0] = QBOND_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        Stake_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = millionsOfQu * QBOND_BASE_STAKE_AMOUNT;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = QBOND_STAKE;
    packet.transaction.inputSize = sizeof(input);
    memcpy(&packet.inputData, &input, sizeof(input));
    KangarooTwelve((uint8_t*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(input),
                   digest, 
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.sig, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t*)&packet, packet.header.size());
    KangarooTwelve((uint8_t*)&packet.transaction, 
                   sizeof(packet.transaction) + sizeof(input) + SIGNATURE_SIZE, 
                   digest,
                   32);
    getTxHashFromDigest(digest, txHash);
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("\n%u\n", currentTick);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + 5, txHash);
    LOG("to check your tx confirmation status\n");
}

void qbondTransfer(const char* nodeIp, int nodePort, const char* seed, const char* targetIdentity, const int64_t epoch, const int64_t mbondsAmount)
{
    TransferMBond_input input;
    input.epoch = epoch;
    input.numberOfMBonds = mbondsAmount;
    memset(input.newOwnerAndPossessor, 0, 32);
    getPublicKeyFromIdentity(targetIdentity, input.newOwnerAndPossessor);

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    uint8_t destPublicKey[32] = { 0 };
    uint8_t digest[32];
    uint8_t signature[64];
    char publicIdentity[128] = { 0 };
    char txHash[128] = { 0 };
    const bool isLowerCase = false;

    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    memset(destPublicKey, 0, 32);
    ((uint64_t*) destPublicKey)[0] = QBOND_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        TransferMBond_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = 1;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = QBOND_TRANSFER;
    packet.transaction.inputSize = sizeof(input);
    memcpy(&packet.inputData, &input, sizeof(input));
    KangarooTwelve((uint8_t*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(input),
                   digest, 
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.sig, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t*)&packet, packet.header.size());
    KangarooTwelve((uint8_t*)&packet.transaction, 
                   sizeof(packet.transaction) + sizeof(input) + SIGNATURE_SIZE, 
                   digest,
                   32);
    getTxHashFromDigest(digest, txHash);
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("\n%u\n", currentTick);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + 5, txHash);
    LOG("to check your tx confirmation status\n");
}

void qbondAddAskOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount)
{
    OrderOperation_input input;
    input.epoch = epoch;
    input.price = mbondPrice;
    input.numberOfMBonds = mbondsAmount;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    uint8_t destPublicKey[32] = { 0 };
    uint8_t digest[32];
    uint8_t signature[64];
    char publicIdentity[128] = { 0 };
    char txHash[128] = { 0 };
    const bool isLowerCase = false;

    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    memset(destPublicKey, 0, 32);
    ((uint64_t*) destPublicKey)[0] = QBOND_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        OrderOperation_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = 1;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = QBOND_ADD_ASK_ORDER;
    packet.transaction.inputSize = sizeof(input);
    memcpy(&packet.inputData, &input, sizeof(input));
    KangarooTwelve((uint8_t*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(input),
                   digest, 
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.sig, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t*)&packet, packet.header.size());
    KangarooTwelve((uint8_t*)&packet.transaction, 
                   sizeof(packet.transaction) + sizeof(input) + SIGNATURE_SIZE, 
                   digest,
                   32);
    getTxHashFromDigest(digest, txHash);
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("\n%u\n", currentTick);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + 5, txHash);
    LOG("to check your tx confirmation status\n");
}

void qbondRemoveAskOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount)
{
    OrderOperation_input input;
    input.epoch = epoch;
    input.price = mbondPrice;
    input.numberOfMBonds = mbondsAmount;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    uint8_t destPublicKey[32] = { 0 };
    uint8_t digest[32];
    uint8_t signature[64];
    char publicIdentity[128] = { 0 };
    char txHash[128] = { 0 };
    const bool isLowerCase = false;

    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    memset(destPublicKey, 0, 32);
    ((uint64_t*) destPublicKey)[0] = QBOND_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        OrderOperation_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = 1;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = QBOND_REMOVE_ASK_ORDER;
    packet.transaction.inputSize = sizeof(input);
    memcpy(&packet.inputData, &input, sizeof(input));
    KangarooTwelve((uint8_t*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(input),
                   digest, 
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.sig, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t*)&packet, packet.header.size());
    KangarooTwelve((uint8_t*)&packet.transaction, 
                   sizeof(packet.transaction) + sizeof(input) + SIGNATURE_SIZE, 
                   digest,
                   32);
    getTxHashFromDigest(digest, txHash);
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("\n%u\n", currentTick);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + 5, txHash);
    LOG("to check your tx confirmation status\n");
}

void qbondAddBidOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount)
{
    OrderOperation_input input;
    input.epoch = epoch;
    input.price = mbondPrice;
    input.numberOfMBonds = mbondsAmount;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    uint8_t destPublicKey[32] = { 0 };
    uint8_t digest[32];
    uint8_t signature[64];
    char publicIdentity[128] = { 0 };
    char txHash[128] = { 0 };
    const bool isLowerCase = false;

    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    memset(destPublicKey, 0, 32);
    ((uint64_t*) destPublicKey)[0] = QBOND_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        OrderOperation_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = mbondPrice * mbondsAmount;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = QBOND_ADD_BID_ORDER;
    packet.transaction.inputSize = sizeof(input);
    memcpy(&packet.inputData, &input, sizeof(input));
    KangarooTwelve((uint8_t*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(input),
                   digest, 
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.sig, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t*)&packet, packet.header.size());
    KangarooTwelve((uint8_t*)&packet.transaction, 
                   sizeof(packet.transaction) + sizeof(input) + SIGNATURE_SIZE, 
                   digest,
                   32);
    getTxHashFromDigest(digest, txHash);
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("\n%u\n", currentTick);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + 5, txHash);
    LOG("to check your tx confirmation status\n");
}

void qbondRemoveBidOrder(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t mbondPrice, const int64_t mbondsAmount)
{
    OrderOperation_input input;
    input.epoch = epoch;
    input.price = mbondPrice;
    input.numberOfMBonds = mbondsAmount;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    uint8_t destPublicKey[32] = { 0 };
    uint8_t digest[32];
    uint8_t signature[64];
    char publicIdentity[128] = { 0 };
    char txHash[128] = { 0 };
    const bool isLowerCase = false;

    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    getIdentityFromPublicKey(sourcePublicKey, publicIdentity, isLowerCase);
    memset(destPublicKey, 0, 32);
    ((uint64_t*) destPublicKey)[0] = QBOND_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        OrderOperation_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = 1;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = QBOND_REMOVE_BID_ORDER;
    packet.transaction.inputSize = sizeof(input);
    memcpy(&packet.inputData, &input, sizeof(input));
    KangarooTwelve((uint8_t*)&packet.transaction,
                   sizeof(packet.transaction) + sizeof(input),
                   digest, 
                   32);
    sign(subseed, sourcePublicKey, digest, signature);
    memcpy(packet.sig, signature, 64);
    packet.header.setSize(sizeof(packet));
    packet.header.zeroDejavu();
    packet.header.setType(BROADCAST_TRANSACTION);
    qc->sendData((uint8_t*)&packet, packet.header.size());
    KangarooTwelve((uint8_t*)&packet.transaction, 
                   sizeof(packet.transaction) + sizeof(input) + SIGNATURE_SIZE, 
                   digest,
                   32);
    getTxHashFromDigest(digest, txHash);
    printReceipt(packet.transaction, txHash, nullptr);
    LOG("\n%u\n", currentTick);
    LOG("run ./qubic-cli [...] -checktxontick %u %s\n", currentTick + 5, txHash);
    LOG("to check your tx confirmation status\n");
}

void qbondGetInfoPerEpoch(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch)
{
    GetInfoPerEpoch_input input;
    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    input.epoch = epoch;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        GetInfoPerEpoch_input in;
    } req;

    memset(&req, 0, sizeof(req));
    req.rcf.contractIndex = QBOND_CONTRACT_INDEX;
    req.rcf.inputType = QBOND_GET_INFO_PER_EPOCH;
    req.rcf.inputSize = sizeof(input);
    memcpy(&req.in, &input, sizeof(input));
    req.header.setSize(sizeof(req.header) + sizeof(req.rcf) + sizeof(input));
    req.header.randomizeDejavu();
    req.header.setType(RequestContractFunction::type());

    qc->sendData((uint8_t*)&req, req.header.size());

    GetInfoPerEpoch_output output;
    memset(&output, 0, sizeof(output));
    try {
        output = qc->receivePacketWithHeaderAs<GetInfoPerEpoch_output>();
    }
    catch (std::logic_error) {
        LOG("Failed to get deals.\n");
        return;
    }

    LOG("Stats for %d epoch:\n   Stakers: %llu\n   Total staked: %lld QU\n", epoch, output.stakersAmount, output.totalStaked * QBOND_BASE_STAKE_AMOUNT);
}

void qbondGetOrders(const char* nodeIp, int nodePort, const char* seed, const int64_t epoch, const int64_t asksOffset, const int64_t bidsOffset)
{
    GetOrders_input input;
    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    input.epoch = epoch;
    input.asksOffset = asksOffset;
    input.bidsOffset = bidsOffset;

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return;
    }

    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        GetOrders_input in;
    } req;

    memset(&req, 0, sizeof(req));
    req.rcf.contractIndex = QBOND_CONTRACT_INDEX;
    req.rcf.inputType = QBOND_GET_ASK_ORDERS;
    req.rcf.inputSize = sizeof(input);
    memcpy(&req.in, &input, sizeof(input));
    req.header.setSize(sizeof(req.header) + sizeof(req.rcf) + sizeof(input));
    req.header.randomizeDejavu();
    req.header.setType(RequestContractFunction::type());

    qc->sendData((uint8_t*)&req, req.header.size());

    GetOrders_output output;
    memset(&output, 0, sizeof(output));
    try {
        output = qc->receivePacketWithHeaderAs<GetOrders_output>();
    }
    catch (std::logic_error) {
        LOG("Failed to get orders.\n");
        return;
    }

    LOG("ASK Orders\n%-62s%-13s%-13s%s\n", "Owner", "MBond name", "Price", "Amount");
    for (int i = 0; i < 256; i++)
    {
        if (!isZeroPubkey(output.askOrders[i].owner))
        {
            char iden[61];
            memset(iden, 0, 61);
            getIdentityFromPublicKey(output.askOrders[i].owner, iden, false);
            LOG("%-62sMBND%-9lld%-13lld%lld\n", iden, output.askOrders[i].epoch, output.askOrders[i].price, output.askOrders[i].numberOfMBonds);
        }
        else
        {
            LOG("\n");
            break;
        }
    }

    LOG("BID Orders\n%-62s%-13s%-13s%s\n", "Owner", "MBond name", "Price", "Amount");
    for (int i = 0; i < 256; i++)
    {
        if (!isZeroPubkey(output.bidOrders[i].owner))
        {
            char iden[61];
            memset(iden, 0, 61);
            getIdentityFromPublicKey(output.bidOrders[i].owner, iden, false);
            LOG("%-62sMBND%-9lld%-13lld%lld\n", iden, output.bidOrders[i].epoch, output.bidOrders[i].price, output.bidOrders[i].numberOfMBonds);
        }
        else
        {
            LOG("\n");
            break;
        }
    }
}
