
#ifndef RAID_PIR_SRC_RAID_PIR_RAID_PIRCENTERSERVER_H_
#define RAID_PIR_SRC_RAID_PIR_RAID_PIRCENTERSERVER_H_

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/crypto/ecc-pk-crypto.h>
#include "../../config.h"
#include "RAID_pirCenterWorker.h"



using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

class RAID_pirCenterWorker;

/**
 * @brief      This class describes a raid pir center server.
 */
class RAID_pirCenterServer {

public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  filename  The filename
     * @param[in]  port      The port
     * @param[in]  secLevel  The security level
     */
    RAID_pirCenterServer( const std::string &filename, uint16_t port = 7766, uint16_t secLevel = SECURITYLEVEL, bool centralized = true, uint32_t peerID = 0);

    /**
     * @brief      Destroys the object.
     */
    ~RAID_pirCenterServer();
    /**
     * @brief      Connect to the port of the server.
     */
    void connect ();

    /**
     * @brief      Start the precomputation
     *
     * @param[in]  server  The peer server's id
     */
    void precompute (uint32_t server);

    /**
     * @brief      Gets the seclevel.
     *
     * @return     The seclevel.
     */
    const uint32_t getSeclevel();

    /**
     * @brief      Gets the ef.
     *
     * @return     The ef.
     */
    ecc_field *getEF();

    /**
     * @brief      Gets the file name.
     *
     * @return     The file name.
     */
    std::string getFileName();

    /**
     * @brief      Gets the chunks groups map.
     *
     * @return     The chunks groups map.
     */
    std::vector<uint32_t> getChunksGroupsMap();

    /**
     * @brief      Starts the tcp server.
     */
    void startServer();

    RAID_pirCenterWorker* getWorker(uint32_t id);

private:
    const uint16_t port;
    const uint16_t secLevel; //TODO maybe make this a global constant?
    const std::string &filename;
    const uint32_t peerId;
    const bool centralizedArch;

    std::vector<uint32_t> chunksGroupsMap;

    ecc_field *ef;
    crypto cry;

    bool threadTerminated;

    uint32_t availableSeeds;
    const uint32_t seedSize;

    /**
     * @brief      Start listening with the tcp server
     *
     * @return     { description_of_the_return_value }
     */
    bool raidListen ();

    std::vector<RAID_pirCenterWorker *> workers;

    bool querySent;
    uint32_t availableBytes;
};

#endif //RAID_PIR_SRC_RAID_PIR_RAID_PIRCENTERSERVER_H_
