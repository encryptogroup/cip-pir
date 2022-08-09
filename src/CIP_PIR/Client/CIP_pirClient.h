

#ifndef CIP_PIR_SRC_CIP_PIR_CLIENT_H_
#define CIP_PIR_SRC_CIP_PIR_CLIENT_H_

#include <iostream>
#include <cstddef>
#include <boost/asio.hpp>
#include <iomanip>
#include <boost/thread/mutex.hpp>
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/crypto/pk-crypto.h>
#include <ENCRYPTO_utils/crypto/ecc-pk-crypto.h>
#include "../../config.h"

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

class RAID_pirClient {

 public:
  RAID_pirClient (string filename);

  const static int maxLength = BLOCKSIZE;
  array<byte, BLOCKSIZE> request (uint32_t index);
  bool checkPassword (string username, string password);
  bool checkHash (byte *hashValue, uint32_t hashSize);
  void addServer (const string &addr, uint32_t port);

  array<byte, BLOCKSIZE> runPIR (uint32_t index);

  void requestServer (uint32_t id, byte *buf, uint32_t bufsize);
  void receiveSeedAndHash (uint32_t id, fe *H_ab);
  void receiveSeed (uint32_t id);
  void sendQuery (uint32_t id);
  void receiveResults (uint32_t id);
  void writeHandle (size_t byteTranferred, const byte *data, const uint32_t serverid);

  void close();


 private:
  std::vector<tcp::socket*> sockets;

  std::vector<uint32_t> chunksGroupsMap;

  uint32_t totalgroups;
  uint32_t querysize;
  array<byte, GROUPS * GROUPSIZE / 8> querybuf;
  array<byte, BLOCKSIZE> resultbuf;
  boost::mutex mutex;

  array<uint32_t, SERVERS> bytesToTransfer;
  array<uint32_t, SERVERS> transferredBytes;

  crypto cry;

  ecc_field *ef;
  fe *generator;

  byte *manifest;

  std::chrono::time_point<std::chrono::high_resolution_clock> time_receiveSeedEnd, time_receiveSeedStart, time_requestStart, time_sendQueries;


  bool rcv (uint32_t serverid, byte *dest, const uint32_t bytesNum);
  bool send (uint32_t serverid, const byte *data, const uint32_t bytesNum);
  bool sendChunks (uint32_t serverid, const byte *data, const uint32_t bytesNum);
  void handleSeed(uint32_t id, const byte *seed);
  void initRequest ();

  void readManifest(string filename);
};

void requestServers (uint32_t id, RAID_pirClient *party, byte *buf, uint32_t bufsize);
void receiveSeedsAndHash (uint32_t id, RAID_pirClient *party, fe *H_ab);
void getSeeds (uint32_t id, RAID_pirClient *party);
void transmitQuery (uint32_t id, RAID_pirClient *party);
void getResults (uint32_t id, RAID_pirClient *party);

void handleWrite (boost::system::error_code err, size_t byteTransferred, RAID_pirClient *party, const byte *data, const uint32_t serverid);
std::chrono::time_point<std::chrono::high_resolution_clock> getTime();

#endif //CIP_PIR_SRC_CIP_PIR_CLIENT_H_
