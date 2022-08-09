
#ifndef CIP_PIR_SRC_CONFIG_H_
#define CIP_PIR_SRC_CONFIG_H_

#include <cmath>
#include <boost/multiprecision/cpp_int.hpp>
#include <relic_fb.h>
#include <thread>
#include <ENCRYPTO_utils/crypto/crypto.h>
#include "parameters.h"

#define byte uint8_t

// number of entries of the database
constexpr uint64_t ENTRIES = 1000000; // 1M (for local testing)

// bytes per entry
constexpr uint64_t ENTRYSIZE = 8;

// --- DON'T CHANGE THE CODE BELOW! ---

constexpr uint64_t bitsToRepresent (uint64_t n) {
    if (n == 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    }
    return 1 + bitsToRepresent(n >> 1);
}

constexpr uint64_t bytesToRepresent (uint64_t n) {
    auto t = bitsToRepresent(n);
    return (t + 7) / 8;
}

constexpr uint64_t DBSIZE = ENTRIES * ENTRYSIZE;
constexpr uint64_t UNBLINDED_DBSIZE = ENTRIES * UNBLINDED_ENTRYSIZE;

// blocks
const uint64_t BLOCKS = (DBSIZE + BLOCKSIZE - 1) / BLOCKSIZE;

// total number of groups
constexpr uint64_t GROUPS = (BLOCKS + GROUPSIZE - 1) / GROUPSIZE;

// number of supported threads
const unsigned THREADS_SUPPORTED = std::thread::hardware_concurrency();

// size of the DB (don't change)
//#define DBSIZE (ENTRIES) * (ENTRYSIZE)
constexpr uint64_t PREDBSIZE = (GROUPS) * (GROUPSIZEP2) * (BLOCKSIZE);



// groups per server in online phase
#define GROUPSPERSERVER CEILING(GROUPS, SERVERS)

#define DBtype std::byte

#define CEILING(x,y) (((x) + (y) - 1) / (y))

#endif //CIP_PIR_SRC_CONFIG_H_
