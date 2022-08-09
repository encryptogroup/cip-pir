#ifndef PASSLEAK_SRC_DATABASE_MANAGEDATABASE_H_
#define PASSLEAK_SRC_DATABASE_MANAGEDATABASE_H_

#include <array>
#include <cstddef>
#include "../config.h"
using namespace std;

void readBlock(const byte *db, uint32_t index);

void readEntry (const byte *db, uint32_t index);

byte* readDBFromFile (const string &filename);

const byte* preprocess (byte *db, const string &filename);

#endif //PASSLEAK_SRC_DATABASE_MANAGEDATABASE_H_
