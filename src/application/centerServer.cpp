#include <iostream>
#include <cstddef>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iomanip>
#include <ENCRYPTO_utils/parse_options.h>
#include "../CIP_PIR/CenterServer/RAID_pirCenterServer.h"
#include "../CIP_PIR/PeerServer/ClientConnection.h"

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

int32_t readTestOptions (int32_t *argcp, char ***argvp, uint32_t *id, uint16_t *port, string *filename, bool *centralized) {
  uint32_t int_port = 0;
  parsing_ctx options[] = { { (void*) &int_port, T_NUM, "p", "Port, default:7766", false, false},
                            { (void*) filename, T_STR, "f", "Filename of the database file, default: database/raw.db_preprocess.db", false, false},
                            { (void*) centralized, T_FLAG, "c", "Use centralized?", false, true},
                            { (void*) id, T_NUM, "i", "Peer Server ID for the precomputation", false, false}};

  if (!parse_options(argcp, argvp, options, sizeof(options) / sizeof(parsing_ctx))) {
    print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
    cout << "Exiting" << endl;
    exit(0);
  }

  if (int_port != 0) {
    assert(int_port < 1 << (sizeof(uint16_t) * 8));
    *port = (uint16_t) int_port;
  }

  return 1;
}

int main (int argc, char **argv) {
  uint16_t port = 7766;
  uint32_t id;
  bool centralized = false;
  string filename = "database/raw.db_preprocess.db";

  readTestOptions (&argc, &argv, &id, &port, &filename, &centralized);

  RAID_pirCenterServer server (filename, port, SECURITYLEVEL, centralized, id);

  server.connect();
  return 0;
}
