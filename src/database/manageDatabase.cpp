#include "manageDatabase.h"
#include "../RAID_PIR/Computation/helper.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstddef>
#include <limits>

/**
 * gives a database written in a given file
 * @param filename name of the file
 * @return database given in the file
 */
byte* readDBFromFile (const string &filename) {
  ifstream file (filename, ios_base::binary | ios::in);
  byte *db = static_cast<byte*>(malloc(DBSIZE + 2 * (RLC_FB_BYTES + 1)));
  if (file.is_open()) {
    string temp;
    uint32_t entries, entrysize;
    file >> temp >> entries >> temp >> entrysize;

    if (entries != ENTRIES || entrysize != ENTRYSIZE) {
      cerr << "Parameters do not fit" << endl;
      cerr << "Number of entries: " << entries << ", expected: " << ENTRIES << endl;
      cerr << "Entrysize: " << entrysize << ", expected: " << ENTRYSIZE << endl;
      free(db);
      return db;
    }
    char *buffer;
    buffer = new char [DBSIZE + 2 * (RLC_FB_BYTES + 1)];
    file.read(buffer, 1);
    file.read(buffer, DBSIZE);
    for (uint32_t i = 0; i < DBSIZE + 2 * (RLC_FB_BYTES + 1); i++) {
      db[i] = static_cast<DBtype> (buffer[i]);
    }
    free(buffer);
    free(db);
    return db;
  } else {
    cerr << "Could not open file " << filename << endl;
    free(db);
    return db;
  }
}


/**
 * prints out the database block at the given index
 * @param db database
 * @param index index of the block
 */
void readBlock(const byte *db, uint32_t index) {
  uint32_t startindex = index * BLOCKSIZE;
  uint32_t destindex = startindex + BLOCKSIZE;
  for (uint32_t i = startindex; i < destindex; ++i) {
    int temp = static_cast<int> (db[i]);
    cout << setw(2) << setfill('0') << hex << temp;
  }
  cout << endl;
}

/**
 * prints out the database entry at the given index
 * @param db database
 * @param index index of the entry
 */
void readEntry(const byte *db, uint32_t index) {
  uint32_t startindex = index * ENTRYSIZE;
  uint32_t destindex = startindex + ENTRYSIZE;
  for (uint32_t i = startindex; i < destindex; ++i) {
    int temp = static_cast<int> (db[i]);
    cout << setw(2) << setfill('0') << hex << temp;
  }
  cout << endl;
}

/**
 * precomputes the db using fourRussians and writes it into a file
 * @param db doriginal atabase
 * @param filename of the file to written in
 * @return preprocessed database
 */
 /* // not working for arbitrary length!
const byte* preprocess (byte *db, const string &filename) {
#ifdef TIME
    auto time_startpreprocess = getMilliSecs();
#endif
    byte *predb = static_cast<byte*>(calloc(PREDBSIZE, sizeof(byte)));

    const uint64_t extrarows = BLOCKS % GROUPSIZE;
    uint64_t expand = 0;

    const byte *generator = db;
    const byte *key = db + RLC_FB_BYTES + 1;
    db = db + 2 * (RLC_FB_BYTES + 1);

    if (extrarows > 0) {
        expand = GROUPSIZE - extrarows;
    }

    const size_t groupbytes = GROUPSIZEP2;
    const size_t bsize = BLOCKSIZE;

    const uint64_t destindex = BLOCKS + expand;

    uint64_t graycode, prevgraycode, diffgraycode;
    for (uint64_t i = 0; i < destindex; i += GROUPSIZE) {
        uint64_t currentGroup = static_cast<uint64_t>(i / GROUPSIZE );
        cout << "yo" << endl;
        uint64_t groupoffset = currentGroup * groupbytes * bsize;
        graycode = 0;
        cout << "yo" << endl;
        for (uint64_t groupelement = 1; groupelement < GROUPSIZEP2; groupelement++) {
            prevgraycode = graycode;
            graycode = groupelement ^ (groupelement >> 1);
            diffgraycode = graycode ^ prevgraycode;
            uint64_t offset = GROUPSIZE - 1;
            for (uint64_t j = 1; j < GROUPSIZEP2; j = j << 1) {
                if (j == diffgraycode) break;
                offset--;
            }
            auto startbytenewdb = &predb[groupoffset + graycode * BLOCKSIZE];
            auto startbyteprevdb = &predb[groupoffset + prevgraycode * BLOCKSIZE];
            auto startbytedb = &db[(i + offset) * BLOCKSIZE];
            memcpy(startbytenewdb, startbyteprevdb, BLOCKSIZE);
            xorFullBlocks(startbytenewdb, startbytedb);
        }
        cout << i <<  " / " <<  destindex << endl;
    }

    cout << "yo" << endl;
#ifdef TIME
    auto time_endPreprocess = getMilliSecs();
#endif

    cout << "yo" << endl;
    ofstream file;
    file.open (filename, ios::binary);
    if(file.good()) cout << "file good" << endl;
    file << "entries" << "\t" << ENTRIES << endl << "entrysize" << "\t" << ENTRYSIZE << endl;
    file << "blocksize" << "\t" << BLOCKSIZE << endl << "groupsize" << "\t" << GROUPSIZE << endl;
    file.write((char*) generator, RLC_FB_BYTES + 1);
    file.write((char*) key, RLC_FB_BYTES + 1);
    file.write((char*) predb, PREDBSIZE);
    file.close();
#ifdef TIME
    auto time_finishWrite = getMilliSecs();
  cout << endl;
  cout << "Times for Preprocessing: " << endl;
  cout << "Preprocessing: " << time_endPreprocess - time_startpreprocess  << " ms" << endl;
  cout << "Writing To File: " << time_finishWrite - time_endPreprocess << " ms" << endl;
  cout << "Total time: " << time_finishWrite - time_startpreprocess << " ms" << endl;
  cout << endl;
#endif

    return predb;
}*/
const byte* preprocess (byte *db, const string &filename) {

     const byte *generator = (byte *) malloc(RLC_FB_BYTES + 1);
     const byte *key = (byte *) malloc(RLC_FB_BYTES + 1);

     ifstream file2;
     file2.open("database/raw.db", std::ios::ios_base::binary | std::ios::in);

     file2.read((char*) generator, RLC_FB_BYTES + 1);
     file2.read((char*) key, RLC_FB_BYTES + 1);

     file2.close();

    cout << "yo" << endl;
  ofstream file;
  file.open (filename, ios::binary);
  if(file.good()) cout << "file good" << endl;
  file << "entries" << "\t" << ENTRIES << endl << "entrysize" << "\t" << ENTRYSIZE << endl;
  file << "blocksize" << "\t" << BLOCKSIZE << endl << "groupsize" << "\t" << GROUPSIZE << endl;
  file.write((char*) generator, RLC_FB_BYTES + 1);
  file.write((char*) key, RLC_FB_BYTES + 1);
  file.close();
#ifdef TIMExx
  auto time_finishWrite = getMilliSecs();
  cout << endl;
  cout << "Times for Preprocessing: " << endl;
  cout << "Preprocessing: " << time_endPreprocess - time_startpreprocess  << " ms" << endl;
  cout << "Writing To File: " << time_finishWrite - time_endPreprocess << " ms" << endl;
  cout << "Total time: " << time_finishWrite - time_startpreprocess << " ms" << endl;
  cout << endl;
#endif

  return nullptr;
}
