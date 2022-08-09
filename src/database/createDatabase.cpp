
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include "../config.h"
#include "manageDatabase.h"
#include "createDatabase.h"
#include "../RAID_PIR/Computation/helper.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <relic.h>
#include <ENCRYPTO_utils/crypto/ecc-pk-crypto.h>

void compress(byte *p_byte);
uint64_t getPrefix(const byte *entry, uint64_t prefix, int prefixlength);
using namespace std;

/**
 * compares two entries in a given database
 * @param db database
 * @param l index of first entry
 * @param r index of second entry
 * @return >0 if first entry is greater, <0 if second entry is greater and 0 if both are equal
 */
inline int compareDBEntries (byte *db, int32_t l, int32_t r) {
  uint32_t index1 = l * ENTRYSIZE;
  uint32_t index2 = r * ENTRYSIZE;
  int n = memcmp(&db[index1], &db[index2], ENTRYSIZE);
  return n;
}

/**
 * swaps two entries in a database
 * @param db database
 * @param l index of first entry
 * @param r index of second entry
 */
inline void DBswap (byte *db, int32_t l, int32_t r) {
  uint32_t indexl = l * ENTRYSIZE;
  uint32_t indexr = r * ENTRYSIZE;
  swap_ranges(&db[indexl], &db[indexl] + ENTRYSIZE, &db[indexr]);
}

/**
 * performs the Quicksort algorithm on a database with left index l and right index r
 * @param db database
 * @param l left index
 * @param r right index
 */
void quicksort (byte *db, int32_t l, int32_t r) {
  if (l >= r) return;
  int32_t pivot = r;
  int32_t cnt = l;
  for (int32_t i = l; i <= r; i++) {
    if (compareDBEntries(db, i, pivot) <= 0) {
      DBswap(db, cnt, i);
      cnt++;
    }
  }
  quicksort(db, l, cnt-2);
  quicksort(db, cnt, r);
}

/**
 * get the prefix of a given length of an entry
 * @param entry entry from which the prefix is extracted
 * @param prefixlength number of bits of the prefix
 * @return the prefix
 */
uint64_t getPrefix(const byte *entry, int prefixlength) {
  uint64_t j = 0;
  uint64_t prefix = 0;
  while (prefixlength > 0) {
    uint64_t val = static_cast<uint64_t>(entry[j]);
    if (prefixlength >= 8) {
      val <<= prefixlength - 8;
    } else {
      val >>= (8 - prefixlength);
      uint64_t mask = 1;
      mask <<= prefixlength;
      mask -= 1;
      val &= mask;
    }
    prefix |= val;
    j++;
    prefixlength -= 8;
  }
  return prefix;
}

/**
 * Calculates z = x - y over some bytes
 * @param x entry one
 * @param y entry two
 * @param z result
 * @param bytes number of bytes to subtract
 */
void subtractBytes(const byte *x, const byte *y, byte *z, const uint64_t bytes) {
  int borrow = 0;
  int difference = 0;
  int d1 = 0;
  int d2 = 0;
  for (int i = bytes - 1; i >= 0; i--) {
    d1 = static_cast<int> (x[i]);
    d2 = static_cast<int> (y[i]);
    difference = d1 - d2 - borrow;
    if (difference < 0) {
      difference += 16;
      borrow = 1;
    } else {
      borrow = 0;
    }
    z[i] = static_cast<byte>(difference);
  }
}

/**
 * print the number of entries for each block with the same prefix of a specifig length
 * @param db database
 * @param numberOfBitIndex number of bits of the prefix
 * @param blocks number of blocks
 */
void printPrefixBlockMapping(byte *db, int numberOfBitIndex, uint64_t blocks) {
  uint64_t maxEntries = 0;
  uint64_t minEntries = UINT64_MAX;
  vector<uint64_t> entriesBlocksMapping (blocks);
  byte *currentIndex = db;
  for (uint64_t i = 0; i < blocks; i++) {
    entriesBlocksMapping[i] = 0;
    uint64_t prefix = i;
    while (prefix == i) {
      prefix = getPrefix(currentIndex, numberOfBitIndex);
      if (prefix == i) {
        entriesBlocksMapping[i]++;
        currentIndex += ENTRYSIZE;
      }
    }
    maxEntries = max(maxEntries, entriesBlocksMapping[i]);
    minEntries = min(minEntries, entriesBlocksMapping[i]);
    cout << i << ": " << entriesBlocksMapping[i] << endl;
  }
  cout << minEntries << " - " << maxEntries << endl;
}

/**
 * performs the compression on a database with the sequence of differences compression scheme
 * @param db database
 */
void compress(byte *db) {
  const uint64_t optimalBlocksize = static_cast<uint64_t>(sqrt(DBSIZE / SERVERS));
  uint64_t blocks = DBSIZE / optimalBlocksize;
  // adapt number of blocks to optimal blocksize

  //printPrefixBlockMapping(db, numberOfBitIndex, blocks);

  // adapt number of blocks s.t. each blocks has the same number of entries
  while (ENTRIES % blocks != 0)  {
    blocks--;
  }
  uint64_t entriesPerBlock = ENTRIES / blocks;
  uint64_t blocksize = entriesPerBlock * ENTRYSIZE;

  cout << "Blocks: " << blocks << endl;
  cout << "Entries per block " << entriesPerBlock << endl;
  cout << "Blocksize: " << blocksize << endl;
  cout << "Optimal blocksize: " << optimalBlocksize << endl;
  uint64_t maxDifferenceSize = 0;
  uint64_t minDifferenceSize = UINT64_MAX;
  byte *difference = (byte*) malloc(ENTRYSIZE);
  const uint64_t entrybits = ENTRYSIZE * 8;
  for (uint64_t i = 0; i < blocks; ++i) {
    for (uint64_t j = entriesPerBlock - 1; j > 0; --j) {
      byte *x = &db[i * blocksize + j * ENTRYSIZE];
      byte *y = &db[i * blocksize + (j-1) * ENTRYSIZE];
      subtractBytes(x, y, difference, ENTRYSIZE);
      //printBytes(difference, ENTRYSIZE);
      uint64_t temp = count_0bits(difference, ENTRYSIZE);
      uint64_t length = entrybits - temp;
      maxDifferenceSize = max(maxDifferenceSize, length);
      minDifferenceSize = min(minDifferenceSize, length);
    }
  }
  //free(difference);
  cout << "max difference (bits): " << maxDifferenceSize << endl;
  cout << "min difference (bits): " << minDifferenceSize << endl;

  uint64_t compressedBlocksize = ENTRYSIZE + std::ceil((entriesPerBlock - 1) * maxDifferenceSize / 8); // TODO ceil -> SIGILL
  uint64_t uncompressedBlocksize = entriesPerBlock * ENTRYSIZE;
  cout << "compressed blocksize: " << compressedBlocksize << endl;
  cout << "uncompressed blocksize: " << uncompressedBlocksize << endl; 
}

void blindDB (byte *unblinded_db, byte *db, ecc_field *ef, fe *generator, fe *keyB) {
  auto *point = ef->get_fe();
  uint8_t *pointBuffer = (uint8_t *) malloc(RLC_FB_BYTES + 1);
  uint8_t *hashBuffer = (uint8_t *) malloc(ENTRYSIZE);
  for (uint32_t i = 0; i < ENTRIES; i++) {
    point = translateToPoint(unblinded_db + i * UNBLINDED_ENTRYSIZE, UNBLINDED_ENTRYSIZE, ef, generator);
    blindHash(point, keyB);
    point->export_to_bytes(pointBuffer);
    sha256_hash(reinterpret_cast<uint8_t *>(db + i * ENTRYSIZE), ENTRYSIZE, pointBuffer, RLC_FB_BYTES + 1, hashBuffer);
  }
  free(pointBuffer); // MAURICE
  free(hashBuffer); // MAURICE
}

void createManifest (const string& filename, const byte *db, fe *generator) {
  ofstream file;
  file.open (filename, ios::binary);
  byte *genbuf = (byte *) malloc(RLC_FB_BYTES + 1);
  generator->export_to_bytes((uint8_t *) genbuf);
  file.write((char *) genbuf, RLC_FB_BYTES + 1);
  //free(genbuf);

  byte *manifestEntries = (byte *) malloc((BLOCKS - 1) * MANIFEST_BYTES);
  uint64_t indexManifest, indexDB;
  for (uint32_t i = 1; i < BLOCKS; ++i) {
    indexManifest = (i - 1) * MANIFEST_BYTES;
    indexDB = i * (ENTRIES / BLOCKS) * ENTRYSIZE;
    memcpy(manifestEntries + indexManifest, db + indexDB, MANIFEST_BYTES);

#ifdef DEBUGINFORMATION
    printBytes(manifestEntries + indexManifest, MANIFEST_BYTES);
#endif
  }

  file.write((char *) manifestEntries, (BLOCKS - 1) * MANIFEST_BYTES);

  file.close();
  file.flush();

  free(genbuf); // MAURICE
  free(manifestEntries); // MAURICE
}

/**
 * generates a random database and stores it in a file with the given filename
 * @param filename name of the file
 */
void generateRandomDB(const string& filename) {
#ifdef TIME
  auto time_startGenerateDatabase = getMilliSecs();
#endif
  ofstream file;
  file.open (filename, ios::binary);
  file << "entries" << "\t" << ENTRIES << endl << "entrysize" << "\t" << ENTRYSIZE << endl;

  random_device engine;
  byte *unblinded_db = static_cast<byte*>(malloc(UNBLINDED_DBSIZE));
  uniform_int_distribution<int> dist(0, 255);
  for (uint64_t i = 0; i < UNBLINDED_DBSIZE; i++) {
    auto temp = dist(engine);
    unblinded_db[i] = static_cast<DBtype> (temp);
  }
#ifdef TIME
  auto time_generatedDatabase = getMilliSecs();
#endif

  byte *db = static_cast<byte*>(malloc(DBSIZE));
  ecc_field *ef = new ecc_field(LT, const_cast<uint8_t *>(m_vSeed));
  fe *generator = ef->get_rnd_generator();
  fe *serverKey = ef->get_rnd_fe();
  blindDB(unblinded_db, db, ef, generator, serverKey);
  free(unblinded_db);

  uint8_t *feStorage = (uint8_t*) malloc(RLC_FB_BYTES + 1);
  generator->export_to_bytes(feStorage);
  file.write((char*) feStorage, RLC_FB_BYTES + 1);
  serverKey->export_to_bytes(feStorage);
  file.write((char*) feStorage, RLC_FB_BYTES + 1);
  free(feStorage);


#if defined(SORTING) || defined(COMPRESS)
  cout << "Start Quicksort" << endl;
  quicksort(db, 0, ENTRIES - 1);
  cout << "End Quicksort" << endl;
#endif

  for (uint64_t i = 1; i < ENTRIES; ++i) {
    if (memcmp (db + (i-1) * ENTRYSIZE, db + i * ENTRYSIZE, ENTRYSIZE) == 0) {
      cout << "Collission found!" << endl;
    }
  }

  createManifest(filename + ".manifest", db, generator);



#ifdef COMPRESS
  cout << "Start Compression" << endl;
//  compress(db);
  cout << "End Compression" << endl;
#endif
#ifdef TIME
  auto time_endQuicksort = getMilliSecs();
#endif
#ifdef DEBUGINFORMATION
  cout << "Created following database:" << endl;
  for (int i = 0; i < ENTRIES; i++) {
    printBytes(&db[i * ENTRYSIZE], ENTRYSIZE);
  }
#endif

  cout << endl;
  printBytes(&db[ENTRYSIZE], ENTRYSIZE);
  printBytes(&db[0], ENTRYSIZE);
  byte *z = (byte*) malloc(ENTRYSIZE);
  subtractBytes(&db[ENTRYSIZE], &db[0], z, ENTRYSIZE);
  printBytes(z, ENTRYSIZE);
  //free(z);
  file.write((char*) db, DBSIZE);
  file.close();
  //free(db);
#ifdef TIME
  auto time_finishWriting = getMilliSecs();
  cout << endl;
  cout << "Times for generating Database: " << endl;
  cout << "Generating Random Bits: " << time_generatedDatabase - time_startGenerateDatabase  << " ms" << endl;
  cout << "Quicksort: " << time_endQuicksort << " ms" << endl;
  cout << "Writing To File: " << time_finishWriting - time_endQuicksort << " ms" << endl;
  cout << "Total time: " << time_finishWriting - time_startGenerateDatabase << " ms" << endl;
  cout << endl;
#endif
}
