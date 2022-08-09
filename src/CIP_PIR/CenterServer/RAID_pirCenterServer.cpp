#include <cstddef>
#include <iostream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <fstream>
#include "RAID_pirCenterServer.h"
#include "CentralConnectionHandler.h"
#include "../Computation/helper.h"


using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::array;
using std::byte;

// peerID = -1 means, that the server computes tuples for all peer servers. When setting peerID to an ivalue >= 0, it only precomputes for the selected peer server.
RAID_pirCenterServer::RAID_pirCenterServer(const std::string &filename, uint16_t port, uint16_t secLevel, bool centralized, uint32_t peerID)
        : port(port), secLevel(secLevel), threadTerminated(true),
          cry (crypto (secLevel, (uint8_t*) const_seed)),
          seedSize(BLOCKSIZE + secLevel / 8), availableSeeds(0),
          filename(filename),
          ef (new ecc_field(LT, const_cast<uint8_t *>(m_vSeed))),
          peerId(peerID),
          centralizedArch(centralized){

    uint32_t N = SERVERS;
    uint32_t G = GROUPS;
    uint32_t extraGroups = GROUPS % SERVERS; // für was?

    for (uint32_t i = 0; i < N; i++) {
        std::cout << "i:" << i << " N:" << N << std::endl;
        uint32_t chunkGroups = static_cast<uint32_t>(GROUPS / SERVERS);
        if (i < extraGroups) {
            ++chunkGroups;
        }
        std::cout << "chunkGroups: " << chunkGroups << std::endl;
        chunksGroupsMap.push_back(chunkGroups);
    }

    // start the servers
    std::cout << "starting the workers" << std::endl;
    startServer();
}

const uint32_t RAID_pirCenterServer::getSeclevel() {
    return secLevel;
}


ecc_field *RAID_pirCenterServer::getEF() {
    return ef;
}

std::string RAID_pirCenterServer::getFileName() {
    return filename;
}

std::vector<uint32_t> RAID_pirCenterServer::getChunksGroupsMap() {
    return chunksGroupsMap;
}

void RAID_pirCenterServer::startServer() {
#ifdef BENCHMARK
    workers.push_back(new RAID_pirCenterWorker(0, this));
#else
    if(centralizedArch) {
        for (uint32_t i = 0; i < SERVERS; ++i) {
            workers.push_back(new RAID_pirCenterWorker(i, this));
            std::cout << "Worker " << i << " started!" << std::endl;
        }
    }
    else{
        std::cout << "Worker " << peerId << " started!" << std::endl;
        workers.push_back(new RAID_pirCenterWorker(peerId, this));
        std::cout << "Worker " << peerId << " started!" << std::endl;
    }

#endif
}

RAID_pirCenterServer::~RAID_pirCenterServer() {}

void RAID_pirCenterServer::connect () {
#ifndef BENCHMARK
    std::cout << "Server trying to listen" << std::endl;
    if (!raidListen()) {
        std::cerr << "Server could not listen" << endl;
        std::exit(5);
    }
    std::cout << "Server awaiting requests" << std::endl;
#endif
}

bool RAID_pirCenterServer::raidListen() {
    try {
        io_service ios;
        CenterListener listener (ios, port, this);
        std::cout << "start accept" << std::endl;
        ios.run();
        std::cout << "start accept" << std::endl;
    }
    catch(std::exception &e){
        std::cerr << "raidListen: " << e.what() << endl;
        return false;
    }
    return true;
}

RAID_pirCenterWorker* RAID_pirCenterServer::getWorker(uint32_t id){
    if(centralizedArch)
        return workers[id];
    else
        return workers[0];
}