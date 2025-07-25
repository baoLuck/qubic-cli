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

constexpr uint64_t ESCROW_CREATE_DEAL_FEE = 1000000ULL;
constexpr uint64_t ESCROW_ACCEPT_DEAL_FEE = 1000000ULL;

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

    input.offeredAssetsAmount = parseAssets(offeredAssetsCommaSeparated, input.offeredAssets, 4);
    input.requestedAssetsAmount = parseAssets(requestedAssetsCommaSeparated, input.requestedAssets, 4);

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
    packet.transaction.amount = ESCROW_CREATE_DEAL_FEE;
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
        return;
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
        LOG("Failed to get deals for owner.\n");
        return;
    }

    // LOG("%lld\n", sizeof(output));

    // uint8_t* rawPtr1 = reinterpret_cast<uint8_t*>(output.currentValue);
    // for (size_t i = 0; i < 8; i++) {
    //     printf("%02X ", rawPtr1[i]);
    //     if ((i + 1) % 40 == 0) printf("\n");
    // }


    LOG("Current value of counter: %lld\n", (long long) output.currentValue);
    LOG("Current deals amount for owner: %lld\n", (long long) output.ownedDealsAmount);
    LOG("Proposed deals amount for owner: %lld\n\n", (long long) output.proposedDealsAmount);

    // LOG("\n\n%lld   %lld\n\n", output.ownedDeals[0].deal.offeredAssets[0].amount, output.ownedDeals[0].deal.requestedAssets[2].amount);
    // LOG("\n\n%lld   %lld\n\n", output.proposedDeals[0].deal.offeredAssets[0].amount, output.proposedDeals[0].deal.offeredAssets[3].amount);


    // for (int i = 0; i < 8; i++)
    // {
    //     LOG("%d. %lld\n", i + 1, output.ownedDeals[0].deal.testAssets[i].amount);
    //     LOG("%d. %lld\n", i + 1, output.proposedDeals[0].deal.testAssets[i].amount);
    // }


    // uint8_t* outputBytes = reinterpret_cast<uint8_t*>(&output);
    // printf("\n%llu\n\n", sizeof(outputBytes));

    // uint8_t* rawPtr = reinterpret_cast<uint8_t*>(output.proposedDeals);
    // size_t rawSize = sizeof(output.proposedDeals);

    // printf("\n%llu\n\n", rawSize);

    // for (size_t i = 0; i < 40; i++) {
    //     printf("%02X ", rawPtr[i]);
    //     if ((i + 1) % 40 == 0) printf("\n");
    // }
    // printf("\n\n");

    // for (size_t i = 40; i < 424; i++) {
    //     printf("%02X ", rawPtr[i]);
    //     if ((i - 40 + 1) % 48 == 0) printf("\n");
    // }

    // printf("\n\n");

    // for (size_t i = 424; i < 464; i++) {
    //     printf("%02X ", rawPtr[i]);
    //     if ((i + 1) % 40 == 0) printf("\n");
    // }


    // printf("\nsizeof(Deal) = %zu\n", sizeof(CounterGetCurrentValue_output::Deal));

    LOG("OWNED DEALS\n%-2s | %-60s | %-60s |%-8s|%-10s| %-60s |%-8s|%-10s\n", "#", "Acceptor ID", "Offered asset issuer", "A Name", "A amount", "Requested asset issuer", "A Name", "A amount");
    printDeals(output.ownedDealsAmount, output.ownedDeals);

    LOG("\nPROPOSED DEALS\n%-2s | %-60s | %-60s |%-8s|%-10s| %-60s |%-8s|%-10s\n", "#", "Acceptor ID", "Offered asset issuer", "A Name", "A amount", "Requested asset issuer", "A Name", "A amount");
    printDeals(output.proposedDealsAmount, output.proposedDeals);
}

void escrowAcceptDeal(const char* nodeIp, int nodePort, const char* seed, int64_t index)
{
    EscrowAcceptDeal_input input;
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
        EscrowAcceptDeal_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = ESCROW_ACCEPT_DEAL_FEE;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = ESCROW_ACCEPT_DEAL;
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

int parseAssets(const std::string& inputStr, EscrowCreateDeal_input::AssetWithAmount* outputArray, int maxCount) 
{
    std::stringstream ss(inputStr);
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

void printDeals(int64_t dealsAmount, const EscrowGetDeals_output::DealEntity* etities)
{
    for (int i = 0; i < dealsAmount; i++)
    {
        char iden[61];
        memset(iden, 0, 61);
        getIdentityFromPublicKey(etities[i].deal.acceptorId, iden, false);
        for (int j = 0; j < etities[i].deal.offeredAssetsAmount; j++)
        {
            char iden1[61];
            memset(iden1, 0, 61);
            getIdentityFromPublicKey(etities[i].deal.offeredAssets[j].issuer, iden1, false);
            char iden2[61];
            memset(iden2, 0, 61);
            getIdentityFromPublicKey(etities[i].deal.requestedAssets[j].issuer, iden2, false);
            LOG("%-2s %s%-60s | %-60s |%-8s|%-10lld| %-60s |%-8s|%-10lld\n",
                (j == 0) ? std::to_string(etities[i].index).c_str() : "",
                (j == 0) ? "| " : "  ",
                (j == 0) ? iden : "",
                iden1,
                &etities[i].deal.offeredAssets[j].name,
                etities[i].deal.offeredAssets[j].amount,
                iden2,
                &etities[i].deal.requestedAssets[j].name,
                etities[i].deal.requestedAssets[j].amount);
        }
        LOG("%s\n", std::string().assign(230, '-').c_str());
    }
}
