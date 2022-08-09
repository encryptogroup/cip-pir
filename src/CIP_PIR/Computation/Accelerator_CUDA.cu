

#include <cuda.h>
#include <stdio.h>
#include "Accelerator_CUDA.h"

static void HandleError( cudaError_t err,
                         const char *file,
                         int line ) {
    if (err != cudaSuccess) {
        printf( "%s in %s at line %d\n", cudaGetErrorString( err ),
                file, line );
        exit( EXIT_FAILURE );
    }
}
#define HANDLE_ERROR( err ) (HandleError( err, __FILE__, __LINE__ ))

__global__ void vector_xor(unsigned char* dest, unsigned char* data) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    // Handling arbitrary vector size
    if (tid < BLOCKSIZE) {
        dest[tid] ^= data[tid];
    }
}

// split it in two parts
// first xor all per block
// then xor the different dest results with eachother
__global__ void vector_xor2(unsigned char* db, unsigned char* dest, unsigned char* data, std::size_t groups) {
    int destS[ELEMENTS_PER_THREAD];

    int bid = blockIdx.x;
    unsigned char* dataPtr = data + bid * groups;

    int threadOffset = threadIdx.x * ELEMENTS_PER_THREAD * 4;

    std::size_t q = static_cast<std::size_t> (dataPtr[0]) * BLOCKSIZE;

    // for first element no xor necessary, since xor with 0 is useless
    for (int i = 0; i < ELEMENTS_PER_THREAD; ++i) {
        destS[i] = *((int*)(db + q + 4 * i + threadOffset));
    }

    // loop over rest of the query
    for (int g = 1; g < groups; ++g) {
        q = static_cast<std::size_t> (dataPtr[g]) * BLOCKSIZE + g * preOffsetZ;
        for (int i = 0; i < ELEMENTS_PER_THREAD; ++i) {
            destS[i] ^= *((int*)(db + q + 4 * i + threadOffset));
        }
    }

    int* destOutPtr = (int*)(dest + bid * BLOCKSIZE);

    // copy from shared to global
    for (int i = 0; i < ELEMENTS_PER_THREAD; ++i) {
        destOutPtr[i + threadOffset / 4] = destS[i];
    }
}
/*
size_t Accelerator_CUDA::getFreeMemory(){
    size_t free, total;
    cudaSetDevice( GPU_ID );
    cudaMemGetInfo( &free, &total );
    return free;
}*/

Accelerator_CUDA::Accelerator_CUDA(byte* data, size_t size) {
    HANDLE_ERROR(cudaMalloc((void**)&db, sizeof(byte) * size));
    HANDLE_ERROR(cudaMemcpy(db, data, sizeof(byte) * size, cudaMemcpyHostToDevice));
}

Accelerator_CUDA::~Accelerator_CUDA(){
    HANDLE_ERROR(cudaFree(db));
}

void Accelerator_CUDA::compute(uint8_t* dst, uint8_t* data_in, const uint64_t groups, int count) {
    byte* dest;
    byte* data;

    HANDLE_ERROR(cudaMalloc((byte**)&dest, BLOCKSIZE * count));
    //cudaMalloc((void**)&data, sizeof(char) * BLOCKSIZE * count);
    HANDLE_ERROR(cudaMalloc((byte**)&data, groups * count));

    HANDLE_ERROR(cudaMemset(dest, 0, BLOCKSIZE * count));
    // copy query to GPU
    HANDLE_ERROR(cudaMemcpy(data, data_in, groups * count, cudaMemcpyHostToDevice));

    auto data_ptr = data;

    // loop over all seeds
    int j;
    for (j = 0; j < count; j += CUDA_BLOCKS) {
        if(j+CUDA_BLOCKS > count) {
            vector_xor2 << < count - j, CUDA_THREADS >> > (db, (dest + j * BLOCKSIZE), data_ptr, groups);
        }
        else
            vector_xor2 << < CUDA_BLOCKS, CUDA_THREADS >> > (db, (dest + j * BLOCKSIZE), data_ptr, groups);
        data_ptr += CUDA_BLOCKS;
    }

    // handle last call; prohibit launching too many blocks
    //vector_xor2 << < count - CUDA_BLOCKS * j, CUDA_THREADS >> > (db, (dest + j * BLOCKSIZE), data_ptr, groups);

    HANDLE_ERROR(cudaMemcpy(dst, dest, BLOCKSIZE * count, cudaMemcpyDeviceToHost));

    HANDLE_ERROR(cudaFree(dest));
    HANDLE_ERROR(cudaFree(data));
}

void Accelerator_CUDA::allocateDB(uint8_t* db, uint64_t groupsForServer, uint64_t groupbytes){
    HANDLE_ERROR(cudaMallocHost( (uint8_t**) &db, sizeof(uint8_t)*groupsForServer * groupbytes) );
}

void Accelerator_CUDA::xorFullBlocks(uint8_t *dest, uint8_t *data){
    uint8_t* a;
    uint8_t* b;

    cudaMalloc((void**)&a, sizeof(char) * BLOCKSIZE);
    cudaMalloc((void**)&b, sizeof(char) * BLOCKSIZE);

    cudaMemset(dest, 0, sizeof(char) * BLOCKSIZE);
    cudaMemcpy(b, data, BLOCKSIZE, cudaMemcpyHostToDevice);

    vector_xor << < CUDA_BLOCKS, CUDA_THREADS >> > (a, b);

    cudaMemcpy(dest, a, BLOCKSIZE, cudaMemcpyDeviceToHost);

    cudaFree(a);
    cudaFree(b);
}

void Accelerator_CUDA::fastXOR(uint8_t *dest, uint8_t *data, const uint8_t *query, const uint64_t groups) {
    uint8_t *destC;
    uint8_t *queryC;

    cudaMalloc((void**)&destC, sizeof(char) * BLOCKSIZE);
    cudaMalloc((void**)&queryC, sizeof(char) * BLOCKSIZE);

    cudaMemcpy(destC, dest, BLOCKSIZE, cudaMemcpyHostToDevice);
    cudaMemcpy(queryC, query, BLOCKSIZE, cudaMemcpyHostToDevice);

    vector_xor2 << < 1, CUDA_THREADS >> > (db, destC, queryC, groups);
    cudaMemcpy(dest, destC, BLOCKSIZE, cudaMemcpyDeviceToHost);

    cudaFree(destC);
    cudaFree(queryC);
}