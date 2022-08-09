
#include "CIP_pirClient.h"

#include <iostream>
#include <cstddef>
#include <boost/asio.hpp>
#include <iomanip>
#include <boost/thread.hpp>
#include <fstream>
#include "../../config.h"
#include "../Computation/helper.h"

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

RAID_pirClient::RAID_pirClient (string filename)
        : cry (crypto (SECURITYLEVEL, (uint8_t *) const_seed)),
          ef (new ecc_field(LT, const_cast<uint8_t *>(m_vSeed))){
    auto N = SERVERS;
    auto G = GROUPS;
    auto extraGroups = GROUPS % SERVERS;
    totalgroups = 0;
    for (uint32_t i = 0; i < N; i++) {
        auto chunkGroups = GROUPS / SERVERS;
        if (i < extraGroups) {
            chunkGroups++;
        }
        chunksGroupsMap.push_back(chunkGroups);
        totalgroups += chunkGroups;
    }
    querysize = totalgroups * GROUPSIZE / 8;
    readManifest(filename);
}

void RAID_pirClient::close(){
    for(size_t i=0; i < sockets.size(); ++i){
        tcp::socket *socket = sockets.at(i);
      //  socket->shutdown(socket_base::shutdown_both);
        socket->close();
    }
}

void RAID_pirClient::initRequest () {
    querybuf.fill(byte(0));
}

bool RAID_pirClient::checkPassword (string username, string password) {
    string combined = username + password;
    uint8_t *combinedBytes = (uint8_t *) combined.c_str();
    uint8_t *hashValue = (uint8_t*) malloc(SHA256_OUT_BYTES);
    sha256_hash(hashValue, SHA256_OUT_BYTES, combinedBytes, combined.length(), hashValue);

    return this->checkHash((byte*) hashValue, combined.length());
}

std::chrono::time_point<std::chrono::high_resolution_clock> getTime(){
    return std::chrono::high_resolution_clock::now();
}

bool RAID_pirClient::checkHash (byte *hashValue, uint32_t hashSize) {
    initRequest();
    fe *clientKey = ef->get_rnd_fe();
    fe *H_a = translateToPoint(hashValue, hashSize, ef, generator);
    blindHash (H_a, clientKey);
    byte *pointBytes = (byte*) malloc (RLC_FB_BYTES + 1);
    H_a->export_to_bytes((uint8_t*) pointBytes);

    std::vector<boost::thread> initThreads;
    for(uint32_t i = 0; i < SERVERS; ++i) {
        initThreads.push_back(boost::thread (requestServers, i, this, pointBytes, hashSize));
    }
    for(uint32_t i = 0; i < SERVERS; ++i) {
        initThreads[i].join();
    }
    initThreads.clear();

    fe *H_ab = ef->get_fe();

    for(uint32_t i = 0; i < SERVERS; ++i) {
        initThreads.push_back(boost::thread (receiveSeedsAndHash, i, this, H_ab));
    }
    for(uint32_t i = 0; i < SERVERS; ++i) {
        initThreads[i].join();
    }
    initThreads.clear();

    unblindHash(H_a, clientKey);
    H_a->export_to_bytes((uint8_t *) pointBytes);
    array<uint8_t, SHA256_OUT_BYTES> hashBuffer;
    array<byte, ENTRYSIZE> resBuffer;
    sha256_hash(reinterpret_cast<uint8_t *>(resBuffer.data()),ENTRYSIZE, reinterpret_cast<uint8_t *>(pointBytes), RLC_FB_BYTES + 1 , hashBuffer.data());

    uint32_t index = 0;
    while (memcmp(pointBytes, &manifest[index * MANIFEST_BYTES], MANIFEST_BYTES) > 0) {
        ++index;
    }

#ifdef TIME
    auto pirStartTime = getTime();
#endif
    auto pirResult = runPIR(index);
#ifdef TIME
    auto pirEndTime = getTime();
    std::chrono::high_resolution_clock::duration duration = pirEndTime - pirStartTime;
    auto dR = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    cout << "PIR Time: " << dR.count() << " ms" << endl;
#endif

#ifdef DEBUGINFORMATION
    printBytes (pirResult.begin(), BLOCKSIZE);
#endif

    // is entry in the block?
    for (uint32_t i = 0; i < BLOCKSIZE / ENTRYSIZE; ++i) {
        int j = memcmp(pointBytes, &(pirResult.data()[i * ENTRYSIZE]), ENTRYSIZE);
        if (j == 0) {
            return true;
        } else if (j < 0) {
            break;
        }
    }

    return false;
}

array<byte, BLOCKSIZE> RAID_pirClient::request (uint32_t index) {
#ifdef TIME
    time_requestStart = getTime();
#endif

    initRequest();

    std::vector<boost::thread> initThreads;

    // receive seeds
#ifdef TIME
    time_receiveSeedStart = getTime();
#endif
    for (uint32_t i = 0; i < SERVERS; i++) {
        initThreads.push_back(boost::thread (getSeeds, i, this));
    }
    for (uint32_t i = 0; i < SERVERS; i++) {
        initThreads[i].join();
    }
    initThreads.clear();
#ifdef TIME
    time_receiveSeedEnd = getTime();
#endif

    return runPIR (index);

}

array<byte, BLOCKSIZE> RAID_pirClient::runPIR (uint32_t index) {
    uint32_t queryindex = index / GROUPSIZE;
    byte queryvalue = byte(1 << (GROUPSIZE - 1 - index % GROUPSIZE));
    querybuf[queryindex] ^= queryvalue;
#ifdef DEBUGINFORMATION
    cout << "complete query: ";
  printBytes(querybuf.begin(), querysize);
#endif
    resultbuf.fill(byte(0));

    std::vector<boost::thread> initThreads;

    // send queries
    for (uint32_t i = 0; i < SERVERS; i++) {
        initThreads.push_back(boost::thread (transmitQuery, i, this));
    }
    for (uint32_t i = 0; i < SERVERS; i++) {
        initThreads[i].join();
    }
    initThreads.clear();
#ifdef TIME
    time_sendQueries = getTime();
#endif
    // receive results
#ifdef STATE
    mutex.lock();
  cout << endl << "Receive Results from Servers" << endl;
  mutex.unlock();
#endif
    for (uint32_t i = 0; i < SERVERS; i++) {
        initThreads.push_back(boost::thread (getResults, i, this));
    }
    for (uint32_t i = 0; i < SERVERS; i++) {
        initThreads[i].join();
    }
    initThreads.clear();

#ifdef TIME
    auto time_finish = getTime();

    std::chrono::high_resolution_clock::duration durReceiveQ = time_receiveSeedEnd - time_receiveSeedStart;
    std::chrono::high_resolution_clock::duration durSend = time_sendQueries - time_receiveSeedEnd;
    std::chrono::high_resolution_clock::duration durReceiveA = time_finish - time_sendQueries;
    std::chrono::high_resolution_clock::duration durTotal = time_finish - time_requestStart;

    auto dRQ = std::chrono::duration_cast<std::chrono::milliseconds>(durReceiveQ);
    auto dS = std::chrono::duration_cast<std::chrono::milliseconds>(durSend);
    auto dRA = std::chrono::duration_cast<std::chrono::milliseconds>(durReceiveA);
    auto dT = std::chrono::duration_cast<std::chrono::milliseconds>(durTotal);

    cout << endl;
    cout << "Times for the single steps: " << endl;
    cout << "Receiving and processing seeds from servers: " << dRQ.count() << " ms" << endl;
    cout << "Sending queries to servers: " << dS.count() << " ms" << endl;
    cout << "Receiving answers from servers and combine them to result: " << dRA.count() << " ms" << endl;
    cout << "Total runtime: " << dT.count() << " ms" << endl;
    cout << endl;
#endif

    return resultbuf;
}

void RAID_pirClient::requestServer(uint32_t id, byte *buf, uint32_t bufsize) {
    if (id == 0) {
        send(id, buf, bufsize);
    } else {
        byte *data = (byte*) calloc(1, 1);
        send(id, data, 1);
    }
}

void RAID_pirClient::receiveSeedAndHash(uint32_t id, fe *H_ab) {
    if (id == 0) {
#ifdef STATE
        mutex.lock();
    cout << "Receive hash and seed form Server 0" << endl;
    mutex.unlock();
#endif
        // TODO: Memory Leakage! allocating fe_byte_size(), but filling with fe_byte_size()+SECURITYLEVEL_BYTES!!
        //byte *buf = (byte*) malloc(ef->fe_byte_size() );
        byte *buf = (byte*) malloc(ef->fe_byte_size() + SECURITYLEVEL_BYTES);
        if (!rcv(id, buf, ef->fe_byte_size() + SECURITYLEVEL_BYTES)) {
            return;
        }
        H_ab->import_from_bytes((uint8_t*) buf);
        this->handleSeed(id, buf + ef->fe_byte_size());
    } else {
        receiveSeed(id);
    }
}

void RAID_pirClient::receiveSeed(uint32_t id) {
#ifdef STATE
    mutex.lock();
  cout << "Receive Seed from Server " << id << endl;
  mutex.unlock();
#endif
    const uint32_t seedbytes = SECURITYLEVEL_BYTES;
    byte *seed = (byte*) malloc(seedbytes);
    if (!rcv(id, seed, seedbytes)) {
        return;
    }
    this->handleSeed(id, seed);
}

void RAID_pirClient::handleSeed(uint32_t id, const byte *seed) {
    uint32_t groupsright = 0, groupsleft = 0;
    uint32_t i = 0;
    for (i = 1; i < CHUNKS && i + id < SERVERS; i++) {
        groupsright += this->chunksGroupsMap[i+id];
    }

    if (i < CHUNKS) {
        for (uint32_t j = 0; i < CHUNKS; j++) {
            groupsleft += this->chunksGroupsMap[j];
            i++;
        }
    }

    const uint32_t groups = groupsleft + groupsright;

    uint32_t skipgroups = 0;
    for (uint32_t i = 0; i <= id; i++) {
        skipgroups += this->chunksGroupsMap[i];
    }
    byte *query = (byte*) malloc (groups * GROUPSIZE / 8);
    this->cry.gen_rnd_from_seed ((uint8_t*) query, groups * GROUPSIZE / 8, (uint8_t*) seed);
    this->mutex.lock();
#ifdef DEBUGINFORMATION
    cout << "query buffer before including query:" << endl;
  printBytes(this->querybuf.begin(), GROUPS * GROUPSIZE / 8);
  cout << "query: " << endl;
  printBytes(query, groups * GROUPSIZE / 8);
#endif
    xorBytes (this->querybuf.begin() + skipgroups * GROUPSIZE / 8, query, groupsright * GROUPSIZE / 8);
    xorBytes (this->querybuf.begin(), query + groupsright * GROUPSIZE / 8, groupsleft * GROUPSIZE / 8);
#ifdef DEBUGINFORMATION
    cout << "query buffer after including query:" << endl;
  printBytes(this->querybuf.begin(), GROUPS * GROUPSIZE / 8);
  cout << endl;
#endif
    this->mutex.unlock();
}

void RAID_pirClient::sendQuery(uint32_t id) {
#ifdef STATE
    mutex.lock();
  cout << "send query to Server " << id << endl;
  mutex.unlock();
#endif
    uint32_t startid = 0;
    for (uint32_t i = 0; i < id; i++) {
        startid += chunksGroupsMap[i];
    }
    uint32_t numBytes = chunksGroupsMap[id];
#ifdef DEBUGINFORMATION
    mutex.lock();
  printBytes (querybuf.begin() + startid, numBytes);
  mutex.unlock();
#endif
#ifdef PIPELINE
    this->sendChunks(id, querybuf.begin() + startid, numBytes);
#else
    this->send (id, querybuf.begin() + startid, numBytes);
#endif
}

void RAID_pirClient::receiveResults(uint32_t id) {
    byte *dest = (byte*) malloc(BLOCKSIZE);
    rcv (id, dest, BLOCKSIZE);
    mutex.lock();
    xorBytes (resultbuf.begin(), dest, BLOCKSIZE);
    mutex.unlock();
}

void RAID_pirClient::addServer(const string &addr, uint32_t port) {
    if (sockets.size() >= SERVERS) {
        std::cerr << "maximum numbers of servers is reached!" << endl;
        return;
    }
    io_service ios;
    tcp::socket *socket = new tcp::socket (ios);
    socket->connect(tcp::endpoint(ip::address::from_string(addr), port));
    sockets.push_back(socket);
#ifdef STATE
    mutex.lock();
  cout << "Added server with address " << addr << " and port " << port << endl;
  mutex.unlock();
#endif
}

bool RAID_pirClient::rcv (uint32_t serverid, byte *dest, const uint32_t bytesNum) {
    if (serverid >= sockets.size()) {
        std::cerr << "receive failed! server with server id " << serverid << " is not added" << endl;
        return false;
    }

    streambuf recBuff;
    boost::system::error_code err;
    read (*sockets[serverid], recBuff, transfer_at_least(bytesNum), err);
    if (err && err!= error::eof) {
        std::cerr << "receive failed: " << err.message() << endl;
        return false;
    } else {
        memcpy(dest, buffer_cast<const void *>(recBuff.data()), bytesNum);
#ifdef STATE
        mutex.lock();
    cout << "Received message from Server " << serverid << endl;
#ifdef DEBUGINFORMATION
    printBytes(dest, bytesNum);
#endif
    mutex.unlock();
#endif
        return true;
    }
}

bool RAID_pirClient::send (uint32_t serverid, const byte *data, const uint32_t numBytes) {
    if (serverid >= sockets.size()) {
        std::cerr << "send failed! server with server id " << serverid << " is not added" << endl;
        return false;
    }

    boost::system::error_code err;
    write(*sockets[serverid], buffer(data, numBytes), err);
    if (!err) {
#ifdef STATE
        mutex.lock();
    cout << "Client sent message to Server " << serverid << endl;
#ifdef DEBUGINFORMATION
    printBytes(data, numBytes);
#endif
    mutex.unlock();
#endif
        return true;
    } else {
        cout << "send failed: " << err.message() << endl;
        return false;
    }
}

void RAID_pirClient::writeHandle(size_t byteTransferred, const byte *data, const uint32_t serverid) {
    transferredBytes[serverid] += byteTransferred;
    if (transferredBytes < bytesToTransfer) {
        uint32_t transferbytes = PIPELINE_CHUNK_SIZE;
        if (transferredBytes[serverid] + PIPELINE_CHUNK_SIZE > bytesToTransfer[serverid]) {
            transferbytes = bytesToTransfer[serverid] - transferredBytes[serverid];
        }
        async_write(*sockets[serverid],
                    buffer(data + byteTransferred, transferbytes),
                    boost::bind(handleWrite, placeholders::error(), placeholders::bytes_transferred(), this, data + byteTransferred, serverid));
    } else {
        // sending finished
        mutex.unlock();
    }
}

bool RAID_pirClient::sendChunks(uint32_t serverid, const byte *data, const uint32_t bytesNum) {
    if (serverid >= sockets.size()) {
        std::cerr << "send failed! server with server id " << serverid << " is not added" << endl;
        return false;
    }
    mutex.lock();
    boost::system::error_code err;
    bytesToTransfer[serverid] = bytesNum;
    transferredBytes[serverid] = 0;

    uint32_t transferBytes = PIPELINE_CHUNK_SIZE;
    while (transferredBytes[serverid] < bytesToTransfer[serverid]) {
        if (transferredBytes[serverid] + transferBytes > bytesToTransfer[serverid]) {
            transferBytes = bytesToTransfer[serverid] - transferredBytes[serverid];
        }
        write(*sockets[serverid], buffer(data + transferredBytes[serverid], transferBytes), err);
        transferredBytes[serverid] += transferBytes;
    }

/*
  async_write(*sockets[serverid],
              buffer(data, PIPELINE_CHUNK_SIZE),
              boost::bind(handleWrite, placeholders::error(), placeholders::bytes_transferred(), this, data, serverid));
  mutex.lock();*/
    mutex.unlock();
}

void RAID_pirClient::readManifest(string filename) {
    std::ifstream file;
    file.open(filename, std::ios::ios_base::binary | std::ios::in);
    uint8_t *genBytes = (uint8_t *) malloc(ef->fe_byte_size());
    file.read((char *) genBytes, ef->fe_byte_size());

    generator = ef->get_rnd_generator();
    generator->import_from_bytes(genBytes);

    manifest = (byte *) malloc((BLOCKS - 1) * MANIFEST_BYTES);
    file.read((char *) manifest, (BLOCKS - 1) * MANIFEST_BYTES);
#ifdef DEBUG_INFORMATION
    for (uint32_t i = 0; i < BLOCKS - 1; ++i) {
      printBytes(manifest + i * MANIFEST_BYTES, MANIFEST_BYTES);
    }
#endif
    file.close();
}

void requestServers (uint32_t id, RAID_pirClient *party, byte *buf, uint32_t bufsize) {
    party->requestServer(id, buf, bufsize);
}

void receiveSeedsAndHash (uint32_t id, RAID_pirClient *party, fe *H_ab) {
    party->receiveSeedAndHash(id, H_ab);
}

void getSeeds (uint32_t id, RAID_pirClient *party) {
    party->receiveSeed(id);
}

void transmitQuery (uint32_t id, RAID_pirClient *party) {
    party->sendQuery(id);
}

void getResults (uint32_t id, RAID_pirClient *party) {
    party->receiveResults(id);
}

void handleWrite (boost::system::error_code err, size_t transferredBytes, RAID_pirClient *party, const byte *data, const uint32_t serverid) {
    if (!err) {
        party->writeHandle (transferredBytes, data, serverid);
    } else {
        std::cerr << "Error: " << err.message() << std::endl;

    }
}



