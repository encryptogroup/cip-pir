#include <cstdint>
#include <iostream>

#include "../config.h"
#include "../database/createDatabase.h"
#include "../database/manageDatabase.h"
#include "../CIP_PIR/Computation/helper.h"
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/parse_options.h>
#include <iomanip>
#include <array>

using std::cout;
using std::endl;
using std::setw;
using std::setfill;



int main(int argc, char **argv) {
  bool analyze = argc > 1;
  string filename = "database/raw.db";
  string filename2 = filename + "_preprocess.db";
  generateRandomDB(filename);
  if (!analyze) {
    auto db = readDBFromFile(filename);
    cout << "Start Preprocessing" << endl;
    /*for (uint32_t i = 0; i < ENTRIES; i++) {
      readEntry(db, i);
    }*/
    auto predb = preprocess(db, filename2);
  }

  cout << endl << endl;
  /*for (uint32_t i = 0; i < PREDBSIZE / BLOCKSIZE; i++) {
    readBlock(predb, i);
  }*/
  return 0;
}
