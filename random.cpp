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

#define RANDOM_CONTRACT_INDEX 3

#define ESCROW_CREATE_DEAL 1
#define ESCROW_GET_DEALS 2
#define ESCROW_ACCEPT_DEAL 3
#define ESCROW_MAKE_DEAL_OPENED 4
#define ESCROW_CANCEL_DEAL 5

constexpr uint64_t ESCROW_CREATE_DEAL_FEE = 1000000ULL;
constexpr uint64_t ESCROW_ACCEPT_DEAL_FEE = 1000000ULL;
constexpr uint64_t ESCROW_MAKE_DEAL_OPENED_FEE = 500000ULL;
constexpr uint64_t ESCROW_CANCEL_DEAL_FEE = 500000ULL;

void escrowCreateDeal(const char* nodeIp, int nodePort, const char* seed,
    int64_t delta,
    const char* acceptorId,
    const char* offeredAssetsCommaSeparated,
    const char* requestedAssetsCommaSeparated)
{
    EscrowCreateDeal_input input;
    input.delta = delta;
    memset(input.acceptorId, 0, 32);
    getPublicKeyFromIdentity(acceptorId, input.acceptorId);

    input.offeredAssetsAmount = parseAssets(offeredAssetsCommaSeparated, input.offeredAssets, 4, input.offeredQU);
    input.requestedAssetsAmount = parseAssets(requestedAssetsCommaSeparated, input.requestedAssets, 4, input.requestedQU);

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
    ((uint64_t*) destPublicKey)[0] = RANDOM_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        EscrowCreateDeal_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = ESCROW_CREATE_DEAL_FEE + input.offeredQU;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = ESCROW_CREATE_DEAL;
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

void escrowGetDeals(const char* nodeIp, int nodePort, const char* seed)
{
    EscrowGetDeals_output output = escrowGetDealsOutput(nodeIp, nodePort, seed);

    LOG("Current value of counter: %lld\n", output.currentValue);
    LOG("Current deals amount for owner: %lld\n", output.ownedDealsAmount);
    LOG("Proposed deals amount for owner: %lld\n", output.proposedDealsAmount);
    LOG("Opened deals amount: %lld\n\n", output.openedDealsAmount);

    printDeals(output.ownedDealsAmount, output.ownedDeals, "OWNED DEALS\n", " (i give)", " (i get) ");
    printDeals(output.proposedDealsAmount, output.proposedDeals, "\nPROPOSED DEALS\n", " (i get) ", " (i give)");
    printDeals(output.openedDealsAmount, output.openedDeals, "\nOPENED DEALS\n", " (i get) ", " (i give)");
}

int64_t escrowGetRequestedQUForDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t& index)
{
    EscrowGetDeals_output output = escrowGetDealsOutput(nodeIp, nodePort, seed);

    for (int i = 0; i < output.proposedDealsAmount; i++)
    {
        if (output.proposedDeals[i].index == index)
        {
            return output.proposedDeals[i].deal.requestedQU;
        }
    }

    for (int i = 0; i < output.openedDealsAmount; i++)
    {
        if (output.openedDeals[i].index == index)
        {
            return output.openedDeals[i].deal.requestedQU;
        }
    }

    return -1;
}

EscrowGetDeals_output escrowGetDealsOutput(const char* nodeIp, int nodePort, const char* seed)
{
    EscrowGetDeals_input input;
    uint8_t subseed[32] = { 0 };
    uint8_t privateKey[32] = { 0 };
    uint8_t sourcePublicKey[32] = { 0 };
    getSubseedFromSeed((uint8_t*) seed, subseed);
    getPrivateKeyFromSubSeed(subseed, privateKey);
    getPublicKeyFromPrivateKey(privateKey, sourcePublicKey);
    memset(input.owner, 0, 32);
    memcpy(input.owner, sourcePublicKey, 32);

    auto qc = make_qc(nodeIp, nodePort);
    if (!qc) {
        LOG("Failed to connect to node.\n");
        return EscrowGetDeals_output{};
    }

    struct {
        RequestResponseHeader header;
        RequestContractFunction rcf;
        EscrowGetDeals_input in;
    } req;

    memset(&req, 0, sizeof(req));
    req.rcf.contractIndex = RANDOM_CONTRACT_INDEX;
    req.rcf.inputType = ESCROW_GET_DEALS;
    req.rcf.inputSize = sizeof(input);
    memcpy(&req.in, &input, sizeof(input));
    req.header.setSize(sizeof(req.header) + sizeof(req.rcf) + sizeof(input));
    req.header.randomizeDejavu();
    req.header.setType(RequestContractFunction::type());

    qc->sendData((uint8_t*)&req, req.header.size());

    EscrowGetDeals_output output;
    memset(&output, 0, sizeof(output));
    try {
        output = qc->receivePacketWithHeaderAs<EscrowGetDeals_output>();
    }
    catch (std::logic_error) {
        LOG("Failed to get deals.\n");
        return EscrowGetDeals_output{};
    }

    return output;
}

void escrowAcceptDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index)
{
    int64_t requestedQU = escrowGetRequestedQUForDeal(nodeIp, nodePort, seed, index);
    if (requestedQU < 0)
    {
        LOG("Failed to get requestedQU for deal with index: %d", index);
        return;
    }
    escrowOperateDeal(nodeIp, nodePort, seed, index, ESCROW_ACCEPT_DEAL_FEE + requestedQU, ESCROW_ACCEPT_DEAL);
}

void escrowMakeDealOpened(const char* nodeIp, int nodePort, const char* seed, const int64_t index)
{
    escrowOperateDeal(nodeIp, nodePort, seed, index, ESCROW_MAKE_DEAL_OPENED_FEE, ESCROW_MAKE_DEAL_OPENED);
}

void escrowCancelDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index)
{
    escrowOperateDeal(nodeIp, nodePort, seed, index, ESCROW_CANCEL_DEAL_FEE, ESCROW_CANCEL_DEAL);
}

void escrowOperateDeal(const char* nodeIp, int nodePort, const char* seed, const int64_t index, const int64_t fee, const unsigned short inputType)
{
    EscrowOperateDeal_input input;
    input.index = index;

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
    ((uint64_t*) destPublicKey)[0] = RANDOM_CONTRACT_INDEX;

    struct {
        RequestResponseHeader header;
        Transaction transaction;
        EscrowOperateDeal_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = fee;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = inputType;
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



int parseAssets(const std::string& inputStr, EscrowCreateDeal_input::AssetWithAmount* outputArray, const int& maxCount, int64_t& QUAmount) 
{
    std::string QUAmountStr;
    std::string assetsStr;
    size_t pos = inputStr.find(':');
    if (pos != std::string::npos) 
    {
        QUAmountStr = inputStr.substr(0, pos);
        QUAmount = std::stoll(QUAmountStr);
        assetsStr = inputStr.substr(pos + 1);
    }
    else 
    {
        QUAmount = std::stoll(inputStr);
        return 0;
    }

    std::stringstream ss(assetsStr);
    std::string asset;
    int count = 0;

    while (std::getline(ss, asset, ':') && count < maxCount)
    {
        std::stringstream ssAsset(asset);
        std::string part;

        for (int i = 0; i < 3; i++)
        {
            if (!std::getline(ssAsset, part, ','))
            {
                LOG("BAD REQ");
                return 0;
            }

            if (i == 0)
            {
                memset(&outputArray[count].name, 0, 8);
                memcpy(&outputArray[count].name, part.c_str(), std::min<size_t>(part.size(), 8));
            }
            else if (i == 1)
            {
                uint8_t issuer[32] = {0};
                getPublicKeyFromIdentity(part.c_str(), issuer);
                memcpy(outputArray[count].issuer, issuer, 32);
            } 
            else if (i == 2)
            {
                outputArray[count].amount = std::stoll(part);
            }
        }

        count++;
    }

    return count;
}

void printDeals(int64_t dealsAmount, const EscrowGetDeals_output::DealEntity* entities, const char* dealTypeName, const char* p1, const char* p2)
{
    if (dealsAmount <= 0)
    {
        return;
    }
    LOG("%s", dealTypeName);
    LOG("%s\n", std::string().assign(222, '-').c_str());
    LOG("%-66s|%-24s%15s%-38s|%-24s%15s%s\n", "", "", "Offered assets", p1, "", "Requested assets", p2);
    LOG("%s\n", std::string().assign(222, '-').c_str());
    LOG("%-3s|%-62s|%-14s|%-62s|%-14s|%-60s\n", "#", " Acceptor ID", " QU Amount", " Asset (Issuer / Name / Amount)", " QU Amount", " Asset (Issuer / Name / Amount)");
    LOG("%s\n", std::string().assign(222, '-').c_str());
    for (int i = 0; i < dealsAmount; i++)
    {
        char iden[61];
        memset(iden, 0, 61);
        getIdentityFromPublicKey(entities[i].deal.acceptorId, iden, false);
        for (int j = 0; j < 4; j++)
        {
            bool isOffered = j < entities[i].deal.offeredAssetsAmount;
            bool isRequested = j < entities[i].deal.requestedAssetsAmount;
            if (!isOffered && !isRequested && j > 0)
            {
                continue;
            }
            char iden1[61];
            memset(iden1, 0, 61);
            getIdentityFromPublicKey(entities[i].deal.offeredAssets[j].issuer, iden1, false);
            char iden2[61];
            memset(iden2, 0, 61);
            getIdentityFromPublicKey(entities[i].deal.requestedAssets[j].issuer, iden2, false);
            LOG("%-3s%-2s%-61s| %-13s| %-61s| %-13s| %-59s\n%67s%15s %-60s |%15s %-60s\n%67s%15s %-60s |%15s %-60s\n",
                (j == 0) ? std::to_string(entities[i].index).c_str() : "",
                (j == 0) ? "| " : "  ",
                (j == 0 && strcmp(iden, "DAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANMIG") != 0) ? iden : "",
                (j == 0) ? std::to_string(entities[i].deal.offeredQU).c_str() : "",
                isOffered ? iden1 : "",
                (j == 0) ? std::to_string(entities[i].deal.requestedQU).c_str() : "",
                isRequested ? iden2 : "",
                "|",
                "|",
                isOffered ? std::string(reinterpret_cast<const char*>(&entities[i].deal.offeredAssets[j].name), 8) : "",
                "|",
                isRequested ? std::string(reinterpret_cast<const char*>(&entities[i].deal.requestedAssets[j].name), 8) : "",
                "|",
                "|",
                isOffered ? std::to_string(entities[i].deal.offeredAssets[j].amount) : "",
                "|",
                isRequested ? std::to_string(entities[i].deal.requestedAssets[j].amount) : "");

            if (j + 1 < entities[i].deal.offeredAssetsAmount || j + 1 < entities[i].deal.requestedAssetsAmount)
            {
                LOG("%67s%15s%s|%15s%s\n",
                    "|",
                    "|",
                    std::string().assign(62, '-').c_str(),
                    "|",
                    std::string().assign(62, '-').c_str());
            }
        }
        LOG("%s\n", std::string().assign(222, '-').c_str());
    }
}
