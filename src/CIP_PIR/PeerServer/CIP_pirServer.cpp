
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <fstream>
#include <chrono>
#include "CIP_pirServer.h"
#include "ClientConnection.h"
#include "../Computation/helper.h"

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::array;
using std::byte;

RAID_pirServer::RAID_pirServer (uint32_t id, const std::string &filename, uint16_t port, uint16_t secLevel, const std::string &sIP, uint16_t portS)
        : id(id), port(port), secLevel(secLevel), threadTerminated(true),
          cry (crypto (secLevel, (uint8_t*) const_seed)), maxSeeds(NUMBER_SEEDS),
          seedSize(BLOCKSIZE + secLevel / 8), availableSeeds(0),
          ef (new ecc_field(LT, const_cast<uint8_t *>(m_vSeed))) {

    uint32_t N = SERVERS;
    uint32_t G = GROUPS;
    uint32_t extraGroups = GROUPS % SERVERS; // für was?
    for (uint32_t i = 0; i < N; i++) {
        uint32_t chunkGroups = static_cast<uint32_t>(GROUPS / SERVERS);
        if (i < extraGroups) {
            ++chunkGroups;
        }
        chunksGroupsMap.push_back(chunkGroups);
    }

    readDB(filename);

    precomputedSeeds = (byte*) malloc (maxSeeds * seedSize);
    currentSeed = precomputedSeeds;
    currentPreSeed = precomputedSeeds;

#ifdef USE_PRECOMPUTATION_SERVER
    connectToCenter(sIP, portS);
#endif


    t1 = new boost::thread(precomputeSeeds, this);

#if defined(BENCHMARK) || defined(BENCHMARK_PEER)
    new boost::thread(testFunc, this);
#endif
}

void RAID_pirServer::dbgFunc(){
    uint32_t seedbytes = secLevel / 8;
    byte* seedTuple = (byte*) malloc(seedbytes + BLOCKSIZE);

    auto time1 = std::chrono::high_resolution_clock::now();

#ifdef BENCHMARK_PEER
    uint8_t* query = (uint8_t*) malloc(this->getQuerySize());
    uint8_t* dest;
    for(int i = 0; i < 10000; ++i){
        // get the seed

        // sth with the seed is shit!
        nextSeed(seedTuple);
        dest = seedTuple + secLevel;

        // here it overflows -> seed corrupts memory
        // generate query
        cry.gen_rnd_from_seed(query, this->getQuerySize(), (uint8_t *) seedTuple);

        fastxor(dest, db, query, chunksGroupsMap[id]);
    }
#else
    for(int i=0; i < 10000; ++i) {
        // std::cout << "waiting" << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // std::cout << "remove seed" << std::endl;
        nextSeed(seedTuple);
        //std::cout << "removed seed " << i << std::endl;
    }
#endif
    auto time2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>( time2 - time1 ).count();

    t1->interrupt();

#ifdef _WIN32
    std::cout << "Time taken: " << duration << " \xE6s" << std::endl;
#else

    std::cout << "Time taken: " << duration << " \xC2\xB5s" << std::endl;
#endif
    t1->interrupt();
    exit(EXIT_SUCCESS);
}


void testFunc(RAID_pirServer* party){
    party->dbgFunc();
}


RAID_pirServer::~RAID_pirServer () {}

void RAID_pirServer::connect () {
    if (!raidListen()) {
        std::cerr << "Server could not listen" << endl;
        std::exit(5);
    }
}

bool RAID_pirServer::raidListen () {
    try {
        io_service ios;
        Server server (ios, port, this);
        ios.run();
    } catch (std::exception &e) {
        std::cerr << "Listen Error: " << e.what() << endl;
        return false;
    }
    return true;
}

void RAID_pirServer::incrementAvailableSeeds() { this->availableSeeds++; }

void RAID_pirServer::decrementAvailableSeeds() { this->availableSeeds--; }

// precompute seeds
void RAID_pirServer::precompute() {
    mutex.lock();
    threadTerminated = false;
    mutex.unlock();

#ifdef USE_PRECOMPUTATION_SERVER
    const uint64_t seedBytes = secLevel / 8;
        boost::system::error_code err;

        // send initialization request to the server
        uint8_t *initbuffer = (uint8_t*) malloc (2);
        initbuffer[0] = (uint8_t) id;
        initbuffer[1] = (uint8_t) 1;
        centerSocket->write_some(buffer(initbuffer, 2), err);

        if (!err || err == error::eof) {
            #ifdef DEBUG_INFORMATION
                cout << "Peer sent initialization!" << endl;
            #endif
        } else {
            std::cerr << "Problem with sending: " << err.message() << endl;
            return;
        }

        while (availableSeeds < maxSeeds) {
            mutex.lock();

            streambuf recBuff;
            #ifdef DEBUG_INFORMATION
                cout << "waiting for rcv!" << endl;
            #endif
            read(*centerSocket, recBuff, transfer_at_least(seedBytes+BLOCKSIZE), err);

            if (!err || err == error::eof) {
                auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
                #ifdef DEBUG_INFORMATION
                    cout << "#" << counter << " @ " <<ctime(&timenow) << " " << "Client received new seed tuple!" << endl;
                #endif
                counter++;
                memcpy(currentPreSeed, buffer_cast<const void *>(recBuff.data()), seedBytes+BLOCKSIZE);
            } else {
                auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
                std::cerr << ctime(&timenow) << "Problem with reading: " << err.message() << endl;
            }
            incrementAvailableSeeds();
            currentPreSeed += (seedBytes + BLOCKSIZE);
            // if we are at the end of the array, we go back to the beginning
            if(currentPreSeed == (precomputedSeeds + (maxSeeds * seedSize))) {
                currentPreSeed = precomputedSeeds;
            }
            mutex.unlock();
        }

        // send terminate request to the server
        initbuffer[0] = (uint8_t) id;
        initbuffer[1] = (uint8_t) 0;
        centerSocket->write_some(buffer(initbuffer, 2), err);

        if (!err || err == error::eof) {
            #ifdef DEBUGINFORMATION
                cout << "Peer sent Termination!" << endl;
            #endif
        } else {
            std::cerr << "Problem with sending: " << err.message() << endl;
            return;
        }
        free(initbuffer);

#elif defined(USE_CUDA_ON_PEER)
    uint32_t groupsToGenerate = 0;
    for (uint32_t i = 1; i < CHUNKS; i++) {
        auto currentindex = (id + i) % chunksGroupsMap.size();
        groupsToGenerate += chunksGroupsMap[currentindex];
    }
    const uint32_t bytesToGenerate = groupsToGenerate * GROUPSIZE / 8;
    const uint64_t seedBytes = getSeclevel() / 8;
    const uint64_t groupsizep2 = GROUPSIZEP2;
    const uint64_t dboffset = chunksGroupsMap[id] * groupsizep2 * BLOCKSIZE;
    auto dbStartingPoint = db + dboffset;

    uint32_t seedsToCompute = maxSeeds - availableSeeds;
    auto *query = (std::byte *) malloc(bytesToGenerate * seedsToCompute );
    auto *seed = (std::byte *) malloc(seedBytes);

    auto *dest = (std::byte *) calloc(BLOCKSIZE, seedsToCompute);

    mutex.lock();
    auto currentPreSeedPointer = currentPreSeed;
    // fill the seed+query arrays
    for(uint32_t i=0; i < seedsToCompute; ++i) {
        cry.gen_rnd((uint8_t *) seed, seedBytes);
        cry.gen_rnd_from_seed((std::byte *) query + (i * bytesToGenerate), bytesToGenerate, (uint8_t *) seed);

        #ifdef DEBUGINFORMATION
            cout << "seed: ";
            printBytes(seed, seedBytes);
            cout << "query from seed: ";
            printBytes(query, bytesToGenerate);
        #endif


        //  std::cout << i << " / " << seedsToCompute << std::endl;
        // copy into the precomputation memory
        memcpy(currentPreSeed, seed, seedBytes);

        currentPreSeed += seedSize;
        // if we are at the end of the array, we go back to the beginning
        if(currentPreSeed == (precomputedSeeds + (maxSeeds * seedSize))) {
            currentPreSeed = precomputedSeeds;
        }
    }

    #ifdef DEBUGINFORMATION
        cout << "Working on CUDA XOR.." << endl;
        cout << "CUDA Blocks: " << CUDA_BLOCKS << " CUDA Threads: " << CUDA_THREADS << endl;
    #endif

    // compute it
    accelerator_CUDA->compute(dest, query, bytesToGenerate, seedsToCompute);

    #ifdef DEBUGINFORMATION
        cout << "Finished CUDA XOR.." << endl;
    #endif

    for(int i=0; i < seedsToCompute; ++i) {
        memcpy(currentPreSeedPointer + seedBytes, dest, BLOCKSIZE);
        incrementAvailableSeeds();
        currentPreSeedPointer += (seedBytes + BLOCKSIZE);
        // if we are at the end of the array, we go back to the beginning
        if(currentPreSeedPointer == (precomputedSeeds + (maxSeeds * seedSize))) {
            currentPreSeedPointer = precomputedSeeds;
        }

        //free(seed);
    }
    mutex.unlock();
#else
    mutex.lock();
        threadTerminated = false;
        mutex.unlock();
        uint32_t groupsToGenerate = 0;
        for (uint32_t i = 1; i < CHUNKS; i++) {
            auto currentindex = (id + i) % chunksGroupsMap.size();
            groupsToGenerate += chunksGroupsMap[currentindex];
        }
        const uint32_t bytesToGenerate = groupsToGenerate * GROUPSIZE / 8;
        const uint64_t seedBytes = secLevel / 8;
        const uint64_t groupsizep2 = GROUPSIZEP2;
        const uint64_t dboffset = chunksGroupsMap[id] * groupsizep2 * BLOCKSIZE;
        auto dbStartingPoint = db + dboffset;
        std::array<std::byte, BLOCKSIZE> dest {};
        mutex.lock();
        while (availableSeeds < maxSeeds) {
            mutex.unlock();
            dest.fill(byte(0));
            auto *query = (std::byte*) malloc(bytesToGenerate);
            auto *seed = (std::byte*) malloc(seedBytes);
            cry.gen_rnd ((uint8_t*) seed, seedBytes);
            cry.gen_rnd_from_seed ((uint8_t*) query, bytesToGenerate, (uint8_t*) seed);
        #ifdef DEBUGINFORMATION
            cout << "seed: ";
            printBytes(seed, seedBytes);
            cout << "query from seed: ";
            printBytes(query, bytesToGenerate);
        #endif
        #ifdef MANUAL_THREADING
            completexor (dest.begin(), dbStartingPoint, query, bytesToGenerate);
        #else
            fastxor(dest.begin(), dbStartingPoint, query, bytesToGenerate);
        #endif
            free(query);

            mutex.lock();
            memcpy(currentPreSeed, seed, seedBytes);
            memcpy(currentPreSeed + seedBytes, dest.begin(), BLOCKSIZE);
            free(seed);
            incrementAvailableSeeds();
            currentPreSeed += (seedBytes + BLOCKSIZE);
            // if we are at the end of the array, we go back to the beginning
            if(currentPreSeed == (precomputedSeeds + (maxSeeds * seedSize))) {
                currentPreSeed = precomputedSeeds;
            }
        }
        threadTerminated = true;
        mutex.unlock();
#endif

    //mutex.lock();
    threadTerminated = true;
    mutex.unlock();
    #ifdef DEBUG_INFORMATION
        std::cout << "Seed memory full" << std::endl;
    #endif
}

byte* RAID_pirServer::nextSeed(byte *seed) {
    uint32_t seedbytes = secLevel / 8;
    while (availableSeeds == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    mutex.lock();
    memcpy(seed, currentSeed, seedbytes + BLOCKSIZE);
    decrementAvailableSeeds();
    currentSeed += seedbytes + BLOCKSIZE;
    if (currentSeed == (precomputedSeeds + (maxSeeds * seedSize))) {
        currentSeed = precomputedSeeds;
    }
    if (threadTerminated) {
        t1 = new boost::thread(precomputeSeeds, this);
    }
    mutex.unlock();
    return seed;
}

const uint32_t RAID_pirServer::getSeclevel() { return this->secLevel; }

const uint32_t RAID_pirServer::getQuerySize() {
    // TODO is this correct?
    return chunksGroupsMap[id] * GROUPSIZE / 8;
    // return std::ceil(chunksGroupsMap[id] * BLOCKSIZE / 8);
}

const void RAID_pirServer::execute(byte *dest, byte *query) {
#ifdef PIPELINE
    querySent = false;
  availableBytes = 0;
  const uint32_t groups = chunksGroupsMap[id];
  uint32_t currentGroup = 0;
  byte *current = db;
  while (!querySent) {
    if (availableBytes > currentGroup) {
      xorFullBlocks(dest, &(current)[static_cast<size_t>(query[currentGroup]) * BLOCKSIZE]);
      current += preOffset;
      ++currentGroup;
    }
  }
#endif

#if PAUSECONDITION != 1
#if PAUSECONDITION == 2
    if (availableSeeds < NUMBER_SEEDS / 2) {
#endif
    t1->interrupt();
    delete t1;
    threadTerminated = true;
#if PAUSECONDITION == 2
    }
#endif
#endif
#ifndef MEASURE_ORIGINAL_RAID_PIR
#ifdef MANUAL_THREADING
    void (*fctpointer)(byte*, const byte*, const byte*, const uint64_t){completexor};
#elif defined(USE_CUDA_ON_PEER)
    void (*fctpointer)(byte*, byte*, const byte*, const uint64_t){fastxor};
#else
    void (*fctpointer)(byte*, byte*, const byte*, const uint64_t){fastxor};
#endif
#ifdef PIPELINE
    fctpointer(dest, current, query + currentGroup, groups - currentGroup);
#elif defined(USE_CUDA_ON_PEER)
    accelerator_CUDA->fastXOR(dest, db, query, chunksGroupsMap[id]);
#else
    fastxor(dest, db, query, chunksGroupsMap[id]);
#endif
#else
    uint64_t groups = 0;
  for (uint64_t i = 0; i < CHUNKS; i++) {
    groups += chunksGroupsMap[(id + i) % SERVERS];
  }
  fastxor(dest, db, query, groups);
#endif
    if (threadTerminated) {
        t1 = new boost::thread(precomputeSeeds, this);
    }
}


void RAID_pirServer::readDB(const std::string &filename) {
    uint64_t groups = 0;

    // get the chunks for server id
    for (uint64_t i = 0; i < CHUNKS; i++) {
        groups += chunksGroupsMap[(id + i) % SERVERS];
    }

    // open database file
    std::ifstream file;
    file.open(filename, std::ios_base::binary | std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Could not open file " << filename << std::endl;
        exit(16);
    }
    std::string temp;
    uint64_t entries, entrysize, blocksize, groupsize;
    file >> temp >> entries >> temp >> entrysize >> temp >> blocksize >> temp >> groupsize;
    if (entries != ENTRIES || entrysize != ENTRYSIZE || blocksize != BLOCKSIZE || groupsize != GROUPSIZE) {
        std::cerr << "database is not compatible with you compile version. Make sure that you did the following configurations:" << std::endl;
        std::cout << "Entries: " << "\t" << entries << std::endl;
        std::cout << "Entrysize: " << "\t" << entrysize << std::endl;
        std::cout << "Blocksize: " << "\t" << blocksize << std::endl;
        std::cout << "Groupsize: " << "\t" << groupsize << std::endl;
        exit(16);
    }

    uint64_t groupsForServerRight = 0, groupsForServerLeft = 0, chunksForServerLeft = 0;
    uint64_t i = 0;
    for (uint64_t j = id; i < CHUNKS && j < chunksGroupsMap.size(); j++) {
        groupsForServerRight += chunksGroupsMap[j];
        i++;
    }
    if (i < CHUNKS) {
        for (uint64_t j = 0; i < CHUNKS; j++) {
            groupsForServerLeft += chunksGroupsMap[j];
            i++;
            chunksForServerLeft++;
        }
    }

    const uint64_t groupsForServer = groupsForServerRight + groupsForServerLeft;
    const uint64_t groupsizep2 = GROUPSIZEP2;
    const uint64_t groupbytes = groupsizep2 * blocksize;
    db = static_cast<byte *>(malloc(groupsForServer * groupbytes));
    const uint64_t chunksToIgnore = id - chunksForServerLeft;
    uint64_t groupsToIgnore = 0;
    for (uint64_t i = 0; i < chunksToIgnore; i++) {
        groupsToIgnore += chunksGroupsMap[id - 1 - i];
    }

    uint8_t *keyBytes = (uint8_t *) malloc(getEF()->fe_byte_size());

    file.ignore(1);
    file.ignore(getEF()->fe_byte_size());
    file.read((char *) keyBytes, getEF()->fe_byte_size());
    file.read ((char *) (db + groupsForServerRight * groupbytes), groupsForServerLeft * groupbytes);
    file.ignore (groupsToIgnore * groupbytes);
    file.read((char *) db, groupsForServerRight * groupbytes);
    file.close();

    // read the key
    serverKey = new ecc_fe(getEF());
    serverKey->import_from_bytes(keyBytes);

    #ifdef USE_CUDA_ON_PEER
        std::cout << "Trying to move the database to the GPU, size: " << groupsForServer * groupbytes << std::endl;
        accelerator_CUDA = new Accelerator_CUDA(db, groupsForServer * groupbytes);
    #endif

#ifdef DEBUGINFORMATION
    cout << "Precomputed Database: "<< endl;
/*    for (uint64_t i = 0; i < groupsForServer * groupbytes; i += BLOCKSIZE) {
        printBytes(db + i, BLOCKSIZE);
    }*/
    cout << "Precomputed database end" << endl << endl;
#endif
    free(keyBytes);
}

void RAID_pirServer::queryComplete() { this->querySent = true; }

void RAID_pirServer::addBytes(uint32_t n) { this->availableBytes += n; }

const uint32_t RAID_pirServer::getID() { return this->id; }

ecc_field* RAID_pirServer::getEF() { return this->ef; }

fe* RAID_pirServer::getServerKey() { return this->serverKey; }


void RAID_pirServer::connectToCenter(const std::string &addr, uint32_t centerPort ){
    io_service ios;
    centerSocket = new tcp::socket(ios);
    centerSocket->connect(tcp::endpoint(ip::address::from_string(addr), centerPort));
}

void precomputeSeeds (RAID_pirServer *party) {
    party->precompute();
}

