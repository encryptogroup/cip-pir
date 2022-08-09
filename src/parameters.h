

#ifndef CIP_PIR_PARAMETERS_H
#define CIP_PIR_PARAMETERS_H

#include <ENCRYPTO_utils/constants.h>

//#define DEBUGINFORMATION // print debug information, outcomment when measuring performance

// Benchmarking
#define TIME // measure time
//#define STATE // print the current state, outcomment when measuring performance
//#define MEASURE_ORIGINAL_RAID_PIR // define if you want to measure times for original RAID_PIR
//#define PIPELINE
//#define BENCHMARK_PEER
//#define BENCHMARK // Benchmark the center server

// GPU Options
#define USE_CUDA
#define USE_CUDA_ON_PEER
//#define USE_PRECOMPUTATION_SERVER

// GPU Hardware Constraints
#define CUDA_MAX_THREADS 1024
#define CUDA_MAX_SM 15
#define CUDA_THREADS_PER_SM 2048
#define CUDA_TOTAL_THREADS CUDA_THREADS_PER_SM*CUDA_MAX_SM


// Parallelization Options (Choose at most one)
#define OPEN_MP
//#define MANUAL_THREADING

// Vectorization Options, comment out all vectorizations that you do not support
//#define AVX512
#define AVX2
#define SSE

// #define RUSSIANS_PRESPROCESS // define for enabling 4Russians
#define SORTING // define for sorting the database
#define COMPRESS // define for compression
#define PAUSECONDITION 0 // 0 = ALWAYS, 1 = NEVER, 2 = HALF

#define UNBLINDED_ENTRYSIZE SHA256_OUT_BYTES
// number of servers
#define SERVERS 3
// blocksize in bytes (must be a divider of DBSIZE and a multiple of 1024)
// if not multiple of 1024, the GPU accelerator needs to be adjusted!
#define BLOCKSIZE 1024


// E = 100k
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096

// E = 1M
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096

// E = 5M
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096

// E = 10M
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096

// E = 25M
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096
//#define BLOCKSIZE 4096

// number of combined bits for 4Russian
#define GROUPSIZE 8 // WARNING: if this values is != 8, you have to adapt the fastxor functions in helper.h!
#define GROUPSIZEP2 1 << GROUPSIZE
// chunks per server
#define CHUNKS SERVERS

// securitylevel
#define SECURITYLEVEL 128
#define SECURITYLEVEL_BYTES SECURITYLEVEL / 8

// max number of seeds during precomputation
#define NUMBER_SEEDS 250

#define MANIFEST_BYTES 8
#define PIPELINE_CHUNK_SIZE 32

#endif //CIP_PIR_PARAMETERS_H
