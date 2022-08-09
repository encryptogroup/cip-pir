
#ifndef RAID_PIR_SRC_DATABASE_CREATEDATABASE_H_
#define RAID_PIR_SRC_DATABASE_CREATEDATABASE_H_

#include "../config.h"
#include <array>

using namespace std;

void generateRandomDB(const string& filename);

inline int compareDBEntries (byte *db, int32_t l, int32_t r);

inline void DBswap (byte *db, int32_t l, int32_t r);

void quicksort (byte *db, int32_t l, int32_t r);

uint64_t getPrefix(const byte *entry, int prefixlength);

void subtractBytes(const byte *x, const byte *y, byte *z, const uint64_t bytes);

void printPrefixBlockMapping(byte *db, int numberOfBitIndex, uint64_t blocks);

#endif //RAID_PIR_SRC_DATABASE_CREATEDATABASE_H_
