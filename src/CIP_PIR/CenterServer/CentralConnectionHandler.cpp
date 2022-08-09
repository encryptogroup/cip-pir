

#include "RAID_pirCenterWorker.h"
#include "RAID_pirCenterServer.h"
#include "CentralConnectionHandler.h"

CentralConnectionHandler::CentralConnectionHandler (io_service &ios, RAID_pirCenterServer *party)
    : sock (ios) {
        this->party = party;
}

tcp::socket& CentralConnectionHandler::socket () { return sock; }

void CentralConnectionHandler::start () {
    cout << "start connection" << endl;

    // Manage:    Receive 2 Byte
    //          - Server ID
    //          - Start (1) or stop (0)
    read();
}

void CentralConnectionHandler::sendHandler(const boost::system::error_code& error){
    if(error){
        auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
        std::cerr << "SendHandler: " << " @ " << ctime(&timenow) << " " << error.message() << std::endl;
    }
    #ifdef DEBUG_INFORMATION
        else
            std::cout << "Sent successfully" << std::endl;
    #endif
}

//void CentralConnectionHandler::readHandler(boost::shared_ptr<boost::array<uint8_t, 1>> buf, const boost::system::error_code& error){
void CentralConnectionHandler::readHandler(boost::shared_ptr<boost::array<uint8_t, 2>> buf, const boost::system::error_code& error){
//void CentralConnectionHandler::readHandler(const boost::system::error_code& error){
    if(error){
        auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
        std::cerr << "readHandler: " << " @ " << ctime(&timenow) << " " << error.message() << std::endl;
        return;
    }

    uint8_t id = buf->data()[0]; // maximum ID = 127
    uint8_t state = buf->data()[1];
    RAID_pirCenterWorker* worker = party->getWorker(id);


    cout << "Server: " << static_cast<unsigned>(id) << " State: " << static_cast<unsigned>(state) << endl;
    cout << "worker: " << party->getWorker(id)->getID() << endl;

    if(state == 0)
        worker->setActive(false);
    else{
        worker->setConnectionHandler(this);
        worker->setActive(true);
    }

    // look if there is some new info
    read();
}

void CentralConnectionHandler::readLoop(){
    uint32_t bufsize = 2;
    byte *buf = (byte*) malloc(bufsize);
    while(true){
        boost::system::error_code err;
        sock.read_some(buffer(buf, bufsize), err);

        if (!err) {
        #ifdef STATE
                    cout << "Server received initialization!" << endl;
        #endif
        } else {
            cout << "Problem with receiving: " << err.message() << endl;
        }

        uint8_t id = buf[0];
        uint8_t state = buf[1];
        RAID_pirCenterWorker* worker = party->getWorker(id);


        cout << "Server: " << static_cast<unsigned>(id) << " State: " << static_cast<unsigned>(state) << endl;

        if(state == 0)
            worker->setActive(false);
        else{
            worker->setConnectionHandler(this);
            worker->setActive(true);
        }
    }
}

void CentralConnectionHandler::send(byte* buff, uint32_t size){
  //  boost::system::error_code err;

    boost::asio::async_write(sock,
                             boost::asio::buffer(buff, size),
                             boost::bind(&CentralConnectionHandler::sendHandler,
                                     shared_from_this(),
                                     boost::asio::placeholders::error)
    );

  //  sock.write_some(buffer(buff, size), err);

 //   return err;
}

void CentralConnectionHandler::read(){
    boost::shared_ptr<boost::array<uint8_t, 2>> buf(new boost::array<uint8_t, 2>);
    boost::asio::buffer(*buf);

    //boost::shared_ptr<boost::array<uint8_t, 2>> buf(new boost::array<uint8_t, 2>);

    std::cout << "awaiting new info" << std::endl;
    boost::asio::async_read(
            sock
            , boost::asio::buffer(*buf)
            , boost::bind(
                    &CentralConnectionHandler::readHandler
                    , shared_from_this()
                    , buf
                    //,
                    , boost::asio::placeholders::error
            )
    );

}



CenterListener::CenterListener(io_service &ios, uint16_t port, RAID_pirCenterServer *party)
        : acceptor_(ios, tcp::endpoint(tcp::v4(), port)) {
    this->party = party;
    startAccept();
}

// start a new connection with some peer
void CenterListener::startAccept() {
    CentralConnectionHandler::pointer connection = CentralConnectionHandler::create(static_cast<io_service&>(acceptor_.get_executor().context()), party);
    acceptor_.async_accept(connection->socket(),
                           boost::bind(&CenterListener::handleAccept, this, connection, placeholders::error));
}

void CenterListener::handleAccept(CentralConnectionHandler::pointer connection, const boost::system::error_code &err){
    if (!err) {
        connection->start();
    }
    startAccept();
}