

#include "ClientConnection.h"
#include "../Computation/helper.h"

conHandler::conHandler (io_service &ios, RAID_pirServer *party) : sock (ios) { this->party = party; }

tcp::socket& conHandler::socket () { return sock; }


void conHandler::start () {
#ifdef STATE
    cout << "start connection" << endl;
#endif
#ifdef TIME
    auto time_startConnection = getTime();
#endif

    boost::system::error_code err;
    const uint32_t feSize = party->getEF()->fe_byte_size();

    bool server0 = party->getID() == 0;
    uint32_t bufsize = server0 ? feSize : 1;

    byte *initbuffer = (byte*) malloc (bufsize);
    sock.read_some(buffer(initbuffer, bufsize), err);
    if (!err) {
#ifdef STATE
        cout << "Server received initialization!" << endl;
#endif
    } else {
        cout << "Problem with receiving: " << err.message() << endl;
    }

    uint32_t seclevel = party->getSeclevel() / 8;
    bufsize = seclevel + BLOCKSIZE;
    fe *H_a = party->getEF()->get_fe();
    if (server0) {
        H_a->import_from_bytes((uint8_t*) initbuffer);
        blindHash (H_a, party->getServerKey());
        bufsize += feSize;
    }
    byte *seedBuffer = (byte*) malloc(bufsize);
    byte *seed = server0 ? seedBuffer + feSize : seedBuffer;
    party->nextSeed(seed);
#ifdef DEBUGINFORMATION
    cout << "Chose the following (seed, value) pair: " << endl;
  printBytes(seed, seclevel);
  printBytes(seed + seclevel, BLOCKSIZE);
#endif
    if (server0) {
        H_a->export_to_bytes((uint8_t *) seedBuffer);
    }
#ifdef TIME
    auto time_sendingSeed = getTime();
#endif
    sock.write_some(buffer(seedBuffer, bufsize - BLOCKSIZE), err);
#ifdef TIME
    auto time_sendingSeedFin = getTime();
#endif
    if (!err) {
#ifdef STATE
        cout << "Server sent seed to Client successfully!" << endl;
#endif
    } else {
        cout << "Problem with sending: " << err.message() << endl;
    }

    // value of the seed-value-pair
    byte *dest = seed + seclevel;

    const uint32_t querysize = party->getQuerySize();
    byte *query = (byte*) malloc(querysize);

#ifdef TIME
    auto time_recQuery = getTime();
#endif

    // TODO Was ist Pipeline?
#ifdef PIPELINE
    uint32_t bytesToReceive = querysize;
  uint32_t chunkSize = PIPELINE_CHUNK_SIZE;
  uint32_t receivedBytes = 0;
  boost::thread t1 (startExecuting, party, dest, query);
  while (receivedBytes < bytesToReceive) {
    if (receivedBytes + chunkSize > bytesToReceive) {
      chunkSize = bytesToReceive - receivedBytes;
    }
    sock.read_some(buffer(query + receivedBytes, chunkSize), err);
    receivedBytes += chunkSize;
    party->addBytes(chunkSize);
  }
  party->queryComplete();
#else
    sock.read_some(buffer(query, querysize), err);
#endif

#ifdef TIME
    auto time_recQueryFin = getTime();
#endif

    if (!err && err != error::eof) {
#ifdef STATE
        cout << "Server received query" << endl;
#ifdef DEBUGINFORMATION
    printBytes(query, querysize);
#endif
#endif
    } else {
        cout << "Problem with receiving: " << err.message() << endl;
    }

#ifdef STATE
    cout << "Compute the answer of the query" << endl;
#endif
#ifdef TIME
    auto time_computeAnswer = getTime();
#endif
#ifndef PIPELINE
    party->execute(dest, query);
#else
    t1.join();
#endif
#ifdef TIME
    auto time_computeAnswerFin = getTime();
#endif
#ifdef STATE
    cout << "Answer computed!" << endl;
#ifdef DEBUGINFORMATION
  printBytes(dest, BLOCKSIZE);
#endif
#endif

#ifdef TIME
    auto time_sendAnswer = getTime();
#endif
    sock.write_some(buffer(dest, BLOCKSIZE), err);
#ifdef TIME
    auto time_sendAnswerFin = getTime();
#endif
    if (!err) {
#ifdef STATE
        cout << "Server sent answer!" << endl;
#endif
    } else {
        cout << "Problem with sending: " << err.message() << endl;
    }
    free(initbuffer);
    free(seedBuffer);
    free(query);
#ifdef STATE
    cout << "end connection" << endl;
#endif

#ifdef TIME
    auto duration = [](auto x, auto y) { return std::chrono::duration_cast<std::chrono::nanoseconds>(x-y); };


    cout << endl;
    cout << "Times for this connection: " << endl;
    cout << "Getting next seed: " << duration(time_sendingSeed, time_startConnection).count() << " ms" << endl;
    cout << "Sending seed to client: " << duration(time_sendingSeedFin, time_sendingSeed).count() << " ms" << endl;
    cout << "Receiving query from client: " << duration(time_recQueryFin, time_recQuery).count() << " ms" << endl;
    cout << "Computing answer (XOR operations): " << duration(time_computeAnswerFin, time_computeAnswer).count() << " ms" << endl;
    cout << "Sending answer to client: " << duration(time_sendAnswerFin, time_sendAnswer).count() << " ms" << endl;
    cout << "Total runtime: " << duration(time_sendAnswerFin, time_startConnection).count() << " ms" << endl;
    cout << endl;
#endif
}


Server::Server(io_service &ios, uint16_t port, RAID_pirServer *party)
        : acceptor_(ios, tcp::endpoint(tcp::v4(), port)) {
    this->party = party;
    startAccept();
}

void Server::handleAccept(conHandler::pointer connection, const boost::system::error_code &err) {
    if (!err) {
        connection->start();
    }
    startAccept();
}

void Server::startAccept() {
    conHandler::pointer connection = conHandler::create(static_cast<io_service&>(acceptor_.get_executor().context()), party);
    acceptor_.async_accept(connection->socket(),
                           boost::bind(&Server::handleAccept, this, connection, placeholders::error));
}

void startExecuting (RAID_pirServer *party, byte *dest, byte *query) {
    party->execute(dest, query);
}

ttime::time_point getTime(){
    return ttime::now();
}

