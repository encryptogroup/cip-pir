
#include <fstream>
#include "RAID_pirCenterWorker.h"
#include <chrono>
#include "../Computation/helper.h"
#ifdef USE_CUDA
#include "../Computation/Accelerator_CUDA.h"
#endif
#include <sys/mman.h>

RAID_pirCenterWorker::RAID_pirCenterWorker(uint32_t id, RAID_pirCenterServer* centerServer)
        : id(id),
          server(centerServer),
#ifdef BENCHMARK
        maxSeeds(1000000),
#else
          maxSeeds(NUMBER_SEEDS),
#endif
          seedSize(BLOCKSIZE + centerServer->getSeclevel() / 8),
          availableSeeds(0),
          cry (crypto (centerServer->getSeclevel(), (uint8_t*) const_seed)){

    readDB();

    precomputedSeeds = (byte*) malloc (maxSeeds * seedSize);
    currentSeed = precomputedSeeds;
    currentPreSeed = precomputedSeeds;

#ifdef BENCHMARK
    precomputeSeeds(this);
#endif

    // start precomputation
    t1 = new boost::thread(precomputeSeeds, this);
    precomputing = true;

    std::cout << "Main Thread: " << std::this_thread::get_id() << std::endl;
}

RAID_pirCenterWorker::~RAID_pirCenterWorker(){}

void RAID_pirCenterWorker::readDB() {
    uint64_t groups = 0;

    // get the chunks for server id
    for (uint64_t i = 0; i < CHUNKS; i++) {
        groups += server->getChunksGroupsMap()[(id + i) % SERVERS];
    }

    // open database file
    std::ifstream file;
    file.open(server->getFileName(), std::ios_base::binary | std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Could not open file " << server->getFileName() << std::endl;
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
    for (uint64_t j = id; i < CHUNKS && j < server->getChunksGroupsMap().size(); j++) {
        groupsForServerRight += server->getChunksGroupsMap()[j];
        i++;
    }
    if (i < CHUNKS) {
        for (uint64_t j = 0; i < CHUNKS; j++) {
            groupsForServerLeft += server->getChunksGroupsMap()[j];
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
        groupsToIgnore += server->getChunksGroupsMap()[id - 1 - i];
    }

    uint8_t *keyBytes = (uint8_t *) malloc(server->getEF()->fe_byte_size());

    file.ignore(1);
    file.ignore(server->getEF()->fe_byte_size());
    file.read((char *) keyBytes, server->getEF()->fe_byte_size());
    file.read ((char *) (db + groupsForServerRight * groupbytes), groupsForServerLeft * groupbytes);
    file.ignore (groupsToIgnore * groupbytes);
    file.read((char *) db, groupsForServerRight * groupbytes);
    file.close();

    // read the key
    serverKey = new ecc_fe(server->getEF());
    serverKey->import_from_bytes(keyBytes);

    accelerator_CUDA = new Accelerator_CUDA(db, groupsForServer * groupbytes);


#ifdef DEBUGINFORMATION
    cout << "Precomputed Database: "<< endl;
/*    for (uint64_t i = 0; i < groupsForServer * groupbytes; i += BLOCKSIZE) {
        printBytes(db + i, BLOCKSIZE);
    }*/
    cout << "Precomputed database end" << endl << endl;
#endif
    free(keyBytes);
}

void RAID_pirCenterWorker::precompute() {

#ifdef DEBUGINFORMATION
    std::cout << "Computation Thread [" << id << "] " << std::this_thread::get_id() << std::endl;
#endif
    uint32_t groupsToGenerate = 0;
    for (uint32_t i = 1; i < CHUNKS; i++) {
        auto currentindex = (id + i) % server->getChunksGroupsMap().size();
        groupsToGenerate += server->getChunksGroupsMap()[currentindex];
    }
    const uint32_t bytesToGenerate = groupsToGenerate * GROUPSIZE / 8;
    const uint64_t seedBytes = server->getSeclevel() / 8;
    const uint64_t groupsizep2 = GROUPSIZEP2;
    const uint64_t dboffset = server->getChunksGroupsMap()[id] * groupsizep2 * BLOCKSIZE;
    auto dbStartingPoint = db + dboffset;

#ifdef BENCHMARK
    auto time1 = std::chrono::high_resolution_clock::now();
#endif


#ifdef USE_CUDA
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
    auto *query = (std::byte *) malloc(bytesToGenerate);
    auto *seed = (std::byte *) malloc(seedBytes);
    std::array<std::byte, BLOCKSIZE> dest{};

    while (availableSeeds < maxSeeds) {
        mutex.lock();
        dest.fill(byte(0));
        cry.gen_rnd((uint8_t *) seed, seedBytes);
        cry.gen_rnd_from_seed((uint8_t *) query, bytesToGenerate, (uint8_t *) seed);

#ifdef MANUAL_THREADING
        completexor (dest.begin(), dbStartingPoint, query, bytesToGenerate);
#else
        fastxor(dest.begin(), dbStartingPoint, query, bytesToGenerate);
#endif

        // allocate the tuple
        // std::byte* result = (std::byte *) malloc(BLOCKSIZE+seedBytes);

        memcpy(currentPreSeed, seed, seedBytes);
        memcpy(currentPreSeed + seedBytes, dest.begin(), BLOCKSIZE);

        incrementAvailableSeeds();
        currentPreSeed += (seedBytes + BLOCKSIZE);
        // if we are at the end of the array, we go back to the beginning
        if(currentPreSeed == (precomputedSeeds + (maxSeeds * seedSize))) {
            currentPreSeed = precomputedSeeds;
        }

        //free(seed);
        mutex.unlock();
    }
#endif

#ifdef BENCHMARK
    auto time2 = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( time2 - time1 ).count();

        #ifdef _WIN32
            std::cout << "Time taken: " << duration << " \xE6s" << std::endl;
        #else

            std::cout << "Time taken: " << duration << " \xC2\xB5s" << std::endl;
        #endif
        exit(EXIT_SUCCESS);
#endif

    free(query);
    precomputing = false;

#ifdef DEBUGINFORMATION
    std::cout << "[" << std::this_thread::get_id() << "] " << "Thread for server " << id << " precomputed " << availableSeeds << "/" << maxSeeds << std::endl;
    std::cout << "[" << std::this_thread::get_id() << "] " << "Thread for server " << id << " terminated" << std::endl;
#endif
}

void RAID_pirCenterWorker::setActive(bool val) {
    //  mutex2.lock();
    if(val && !active){
        this->active = val;
#ifdef DEBUGINFORMATION
        std::cout << "Start sending: " << id << std::endl;
#endif
        t2 = new boost::thread(transferSeeds, this);
    }
    else
    {
        this->active = val;
#ifdef DEBUGINFORMATION
        std::cout << "Interrupt sending: " << id << std::endl;
#endif
        t2->interrupt();
    }
    // mutex2.unlock();
}

bool RAID_pirCenterWorker::getActive() {
    return active;
}

void RAID_pirCenterWorker::transfer(){
    uint32_t seedbytes = server->getSeclevel() / 8;
    byte* seed = (byte*) malloc(seedbytes + BLOCKSIZE);
    while(active){
        nextSeed(seed);
        sendTuple(seed);
        //std::cout << "sending to server " << id << std::endl;
    }

    free(seed);
}

void transferSeeds(RAID_pirCenterWorker *worker){
    worker->transfer();
}

void precomputeSeeds(RAID_pirCenterWorker *worker){
    worker->precompute();
}

const uint32_t RAID_pirCenterWorker::getID() {
    return id;
}

void RAID_pirCenterWorker::setConnectionHandler(CentralConnectionHandler* con) {
    connectionHandler = con;
}

void RAID_pirCenterWorker::sendTuple(std::byte *tuple) {
    const uint64_t seedBytes = server->getSeclevel() / 8;

    connectionHandler->send(tuple, BLOCKSIZE+seedBytes);
}

void RAID_pirCenterWorker::incrementAvailableSeeds() { this->availableSeeds++; }

void RAID_pirCenterWorker::decrementAvailableSeeds() { this->availableSeeds--; }

byte* RAID_pirCenterWorker::nextSeed(byte *seed) {
    uint32_t seedbytes = server->getSeclevel() / 8;
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
    if (!precomputing && availableSeeds <= (uint32_t)(maxSeeds / 2)) {
#ifdef DEBUGINFORMATION
        std::cout << "launching new thread:" << availableSeeds << " / " << maxSeeds << std::endl;
#endif
        t1 = new boost::thread(precomputeSeeds, this);
        precomputing = true;
    }
    mutex.unlock();

    return seed;
}