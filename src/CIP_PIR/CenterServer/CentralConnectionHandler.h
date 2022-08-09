

#ifndef RAID_PIR_CENTRALCONNECTIONHANDLER_H
#define RAID_PIR_CENTRALCONNECTIONHANDLER_H

#include <iostream>
#include <cstddef>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iomanip>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

class RAID_pirCenterServer;

class CentralConnectionHandler : public boost::enable_shared_from_this<CentralConnectionHandler> {
private:
    tcp::socket sock;
    RAID_pirCenterServer *party;
   // boost::array<uint8_t, 2> buf;

public:
    typedef boost::shared_ptr<CentralConnectionHandler> pointer;

    CentralConnectionHandler(io_service &ios, RAID_pirCenterServer *party);

    static pointer create(io_service &ios, RAID_pirCenterServer *party) {
        std::cout << "new connection here yaa" << std::endl;
        return pointer(new CentralConnectionHandler(ios, party));
    }

    tcp::socket &socket();

    void send(byte* buff, uint32_t size);
    void read();

    void sendHandler(const boost::system::error_code& error);
    void readHandler(boost::shared_ptr<boost::array<uint8_t, 2>> buf, const boost::system::error_code& error);
   // void readHandler(const boost::system::error_code& error);

    void start();

    void readLoop();
};

class CenterListener {
private:
    tcp::acceptor acceptor_;
    RAID_pirCenterServer *party;

    /**
     * @brief      Start accepting connections
     */
    void startAccept();

public:
    CenterListener (io_service &ios, uint16_t port, RAID_pirCenterServer *party);

    /**
     * @brief      Handle accepted connections
     *
     * @param[in]  connection  The connection
     * @param[in]  err         The error
     */
    void handleAccept(CentralConnectionHandler::pointer connection, const boost::system::error_code &err);
};

void startExecuting (RAID_pirCenterServer *party, byte *dest, byte *query);


#endif //RAID_PIR_CENTRALCONNECTIONHANDLER_H
