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

#define RANDOM_GET 1
#define RANDOM_SET 2


// void randomGet(const char* nodeIp, int nodePort, const char* seed)
// {
//     GetRandom_input input;

//     auto qc = make_qc(nodeIp, nodePort);
//     if (!qc) {
//         LOG("Failed to connect to node.\n");
//         return;
//     }

//     struct {
//         RequestResponseHeader header;
//         RequestContractFunction rcf;
//         GetRandom_input in;
//     } req;

//     memset(&req, 0, sizeof(req));
//     req.rcf.contractIndex = RANDOM_CONTRACT_INDEX;
//     req.rcf.inputType = RANDOM_GET;
//     req.rcf.inputSize = sizeof(input);
//     memcpy(&req.in, &input, sizeof(input));
//     req.header.setSize(sizeof(req.header) + sizeof(req.rcf) + sizeof(input));
//     req.header.randomizeDejavu();
//     req.header.setType(RequestContractFunction::type());

//     qc->sendData((uint8_t*)&req, req.header.size());

//     GetRandom_output output;
//     memset(&output, 0, sizeof(output));
//     try {
//         output = qc->receivePacketWithHeaderAs<GetRandom_output>();
//     }
//     catch (std::logic_error) {
//         LOG("Failed to get current value of escrow.\n");
//         return;
//     }

//     LOG("%lld\n", output.millisecond);
//     LOG("%lld\n", output.second);
//     LOG("%lld\n", output.minute);
// }

void randomGet(const char* nodeIp, int nodePort, const char* seed)
{
    SetRandom_input input;

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
        SetRandom_input inputData;
        uint8_t sig[64];
    } packet;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.transaction.sourcePublicKey, sourcePublicKey, 32);
    memcpy(packet.transaction.destinationPublicKey, destPublicKey, 32);
    packet.transaction.amount = 500;
    uint32_t currentTick = getTickNumberFromNode(qc);
    packet.transaction.tick = currentTick + 5;
    packet.transaction.inputType = RANDOM_SET;
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
