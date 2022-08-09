#include <iostream>
#include <cstddef>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iomanip>
#include <ENCRYPTO_utils/parse_options.h>
#include "../RAID_PIR/PeerServer/RAID_pirServer.h"
#include "../RAID_PIR/PeerServer/ClientConnection.h"

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

int32_t readTestOptions (int32_t *argcp, char ***argvp, uint32_t *id, uint16_t *port, string *filename, string *ipS, uint16_t *portS) {
  uint32_t int_port = 0;
  uint32_t int_portS = 0;
  parsing_ctx options[] = { { (void*) id, T_NUM, "i", "ID of the server", true, false},
                            { (void*) &int_port, T_NUM, "p", "Port, default:7123", false, false},
                            { (void*) ipS, T_STR, "a", "IP of Precomputation Server, default:127.0.0.1", false, false},
                            { (void*) &int_portS, T_NUM, "s", "Port of Precomputation Server, default:7766", false, false},
                            { (void*) filename, T_STR, "f", "Filename of the database file, default: database/raw.db_preprocess.db", false, false}};

  if (!parse_options(argcp, argvp, options, sizeof(options) / sizeof(parsing_ctx))) {
    print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
    cout << "Exiting" << endl;
    exit(0);
  }

  if (int_port != 0) {
    assert(int_port < 1 << (sizeof(uint16_t) * 8));
    *port = (uint16_t) int_port;
  }
  if (int_portS != 0) {
        assert(int_portS < 1 << (sizeof(uint16_t) * 8));
        *portS = (uint16_t) int_portS;
    }

  return 1;
}

int main (int argc, char **argv) {
  string ipS = "127.0.0.1";
  uint16_t portS = 7766;
  uint16_t port = 7123;
  uint32_t id;
  string filename = "database/raw.db_preprocess.db";

  readTestOptions (&argc, &argv, &id, &port, &filename, &ipS, &portS);

  RAID_pirServer server (id, filename, port, SECURITYLEVEL, ipS, portS);
  server.connect();
  return 0;
}
