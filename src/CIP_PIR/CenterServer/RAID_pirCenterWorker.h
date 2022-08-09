

#ifndef RAID_PIR_RAID_PIRCENTERWORKER_H
#define RAID_PIR_RAID_PIRCENTERWORKER_H

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
#include "RAID_pirCenterServer.h"
#include "CentralConnectionHandler.h"

#include "../Computation/Accelerator_CUDA.h"

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;
using std::array;
using std::byte;

// forward declaration -> sufficient since just using pointers
class RAID_pirCenterServer;

/**
 * @brief      This class describes a thread run by the central server. Here the
 *             precomputation takes place.
 */
class RAID_pirCenterWorker {
public:
    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  id            The identifier of the server
     * @param      centerServer  The center server
     */
    RAID_pirCenterWorker(uint32_t id, RAID_pirCenterServer* centerServer);

 //   RAID_pirCenterWorker(RAID_pirCenterWorker& w);
 //   RAID_pirCenterWorker(const RAID_pirCenterWorker& w);

    /**
     * @brief      Destroys the object.
     */
    ~RAID_pirCenterWorker();

    void precompute();


    /**
     * @brief      Gets the id.
     *
     * @return     The id.
     */
    const uint32_t getID();

    /**
     * @brief      Sets the active variable.
     *
     * @param[in]  active  The desired state of the thread
     */
    void setActive(bool active);
    /**
     * @brief      Gets if the thread is active
     *
     * @return     State of the thread.
     */
    bool getActive();

    void setConnectionHandler(CentralConnectionHandler* con);
    void transfer ();

private:
    uint32_t id;
    bool active = false;
    bool precomputing = false;
    crypto cry;

    uint64_t counter = 0;

    RAID_pirCenterServer* server;
    boost::thread *t1, *t2;
    CentralConnectionHandler *connectionHandler;

    fe *serverKey;
    byte *db;
    /**
     * @brief      Reads the chunks from the database according to the server's
     *             id.
     */
    void readDB();

    /**
     * @brief      Sends a tuple to the peer server.
     *
     * @param      tuple  The tuple
     */
    void sendTuple(std::byte* tuple);


    // seed management
    byte *precomputedSeeds;
    uint32_t availableSeeds;
    const uint32_t maxSeeds;
    const uint32_t seedSize;
    byte *currentSeed;
    byte *currentPreSeed;

    boost::mutex mutex;
    boost::mutex mutex2;
    void incrementAvailableSeeds ();
    void decrementAvailableSeeds ();
    byte* nextSeed(byte *seed);

    Accelerator_CUDA *accelerator_CUDA;

};


void precomputeSeeds (RAID_pirCenterWorker *worker);
void transferSeeds (RAID_pirCenterWorker *worker);


#endif //RAID_PIR_RAID_PIRCENTERWORKER_H
