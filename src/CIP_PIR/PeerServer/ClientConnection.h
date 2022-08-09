

#ifndef CIP_PIR_SRC_CIP_PIR_SERVERCONNECTION_H_
#define CIP_PIR_SRC_CIP_PIR_SERVERCONNECTION_H_

#include <iostream>
#include <cstddef>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iomanip>
#include "../../config.h"
#include "CIP_pirServer.h"
#include <chrono>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;
using ttime = std::chrono::high_resolution_clock;

class conHandler : public boost::enable_shared_from_this<conHandler> {
 private:
  tcp::socket sock;
  RAID_pirServer *party;

 public:
  typedef boost::shared_ptr<conHandler> pointer;

  conHandler(io_service &ios, RAID_pirServer *party);

  static pointer create(io_service &ios, RAID_pirServer *party) {
    return pointer(new conHandler(ios, party));
  }

  tcp::socket &socket();

  void start();
};

class Server {
 private:
  tcp::acceptor acceptor_;
  void startAccept();
  RAID_pirServer *party;

 public:
  Server(io_service &ios, uint16_t port, RAID_pirServer *party);

  void handleAccept(conHandler::pointer connection, const boost::system::error_code &err);
};

void startExecuting (RAID_pirServer *party, byte *dest, byte *query);
std::chrono::time_point<ttime> getTime();

#endif //CIP_PIR_SRC_CIP_PIR_SERVERCONNECTION_H_
