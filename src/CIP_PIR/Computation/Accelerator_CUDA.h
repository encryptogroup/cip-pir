

#ifndef CIP_PIR_ACCELERATOR_CUDA_H
#define CIP_PIR_ACCELERATOR_CUDA_H

#include <stdint.h>
#include <cstddef>
#include "../../parameters.h"

typedef uint8_t byte;

// compute optimal block & thread counts
#if BLOCKSIZE >= (CUDA_MAX_THREADS*4)
#define CUDA_THREADS CUDA_MAX_THREADS
#define ELEMENTS_PER_THREAD BLOCKSIZE / CUDA_THREADS / 4 * 2
#else
#define CUDA_THREADS BLOCKSIZE / 4
    #define ELEMENTS_PER_THREAD 1
#endif

#define CUDA_BLOCKS (CUDA_TOTAL_THREADS) / (CUDA_THREADS) * 2
//#define CUDA_THREADS 512

constexpr std::size_t preOffsetZ = BLOCKSIZE * GROUPSIZEP2;

class Accelerator_CUDA {
public:
    Accelerator_CUDA();
    Accelerator_CUDA(byte* data, size_t size);
    ~Accelerator_CUDA();
    void compute(uint8_t* dst, uint8_t* data, const uint64_t groups, int count);
    static void xorFullBlocks(uint8_t *dest, uint8_t *data);
    static void allocateDB(uint8_t* db, uint64_t groupsForServer, uint64_t groupbytes);
    void fastXOR(uint8_t *dest, uint8_t *data, const uint8_t *query, const uint64_t groups);
private:
    byte* db;
};


#endif //CIP_PIR_ACCELERATOR_CUDA_H