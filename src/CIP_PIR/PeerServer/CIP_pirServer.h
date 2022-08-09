
#ifndef CIP_PIR_SRC_CIP_PIR_RAID_PIRSERVER_H_
#define CIP_PIR_SRC_CIP_PIR_RAID_PIRSERVER_H_

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "CIP_pirServer.h"
#include "../../config.h"
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/crypto/ecc-pk-crypto.h>
#ifdef USE_CUDA_ON_PEER
    #include "../Computation/Accelerator_CUDA.h"
#endif

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::array;
using std::byte;



class RAID_pirServer {

 public:
  RAID_pirServer (uint32_t id, const std::string &filename, uint16_t port = 7766, uint16_t secLevel = SECURITYLEVEL, const std::string &sIP = "127.0.0.1", uint16_t portS = 7766);
  ~RAID_pirServer();

  void connect ();

  void precompute ();

  const uint32_t getSeclevel();
  const uint32_t getQuerySize();

  const void execute (byte *dest, byte *query);

  byte* nextSeed(byte *seed);

  const uint32_t getID();

  ecc_field *getEF();

  fe *getServerKey();

  void queryComplete ();
  void addBytes (uint32_t n);

    [[noreturn]] void dbgFunc();

 private:
  const uint32_t id;
  const uint16_t port;
  const uint16_t secLevel; //TODO maybe make this a global constant?

  uint64_t counter = 0;

  std::vector<uint32_t> chunksGroupsMap;

  crypto cry;

  fe *serverKey;
  ecc_field *ef;

  boost::mutex mutex;
  bool threadTerminated;

  byte *precomputedSeeds;
  uint32_t availableSeeds;
  const uint32_t maxSeeds;
  const uint32_t seedSize;
  byte *currentSeed;
  byte *currentPreSeed;

  void incrementAvailableSeeds ();
  void decrementAvailableSeeds ();

  byte *db;
  void readDB(const std::string &filename);

  bool raidListen ();

  boost::thread *t1;

  bool querySent;
  uint32_t availableBytes;

  // Centralization
  tcp::socket *centerSocket;
  void connectToCenter(const std::string &addr, uint32_t centerPort );

    #ifdef USE_CUDA_ON_PEER
        Accelerator_CUDA *accelerator_CUDA;
    #endif
};

void precomputeSeeds (RAID_pirServer *party);
void testFunc (RAID_pirServer *party);


#endif //CIP_PIR_SRC_CIP_PIR_RAID_PIRSERVER_H_
