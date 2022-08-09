

#ifndef CIP_PIR_SRC_CIP_PIR_HELPER_H_
#define CIP_PIR_SRC_CIP_PIR_HELPER_H_

#include <cstddef>
#include <cstdint>
#include "../../config.h"
#include <iostream>
#include <boost/thread.hpp>
#include <boost/make_unique.hpp>
#include "omp.h"
#include <ENCRYPTO_utils/crypto/ecc-pk-crypto.h>
#include <immintrin.h>

using std::cout;
using std::endl;

constexpr std::size_t preOffset = BLOCKSIZE * GROUPSIZEP2;


void printBytes (const std::byte *msg, uint32_t bytes);

/**
 * translate a hash value to an ECC point
 * @param buf hash value
 * @param bufsize size of the hash
 * @param ef elliptic curve field
 * @param generator field generator
 * @return point on the elliptic curve
 */
inline fe *translateToPoint(const std::byte *buf, uint32_t bufsize, ecc_field *ef, fe *generator) {
  num *p1 = ef->get_num();
  fe *point = ef->get_fe();
  point->set(generator);
  p1->import_from_bytes((uint8_t*) buf, bufsize);
  point->set_pow(point, p1);
  return point;
}

/**
 * blind a point on an elliptic curve that represents a hash
 * @param point the point to blind
 * @param key blinding key
 */
inline void blindHash (fe *point, fe *key) {
  point->set_mul(point, key);
}

/**
 * unblinds a point on an elliptic curve that reprsents a hash
 * @param point the point to unblind
 * @param key blinding key
 */
inline void unblindHash (fe *point, fe *key) {
  point->set_div(point, key);
}

/**
 * gives the current millisecond
 * @return milliseconds
 */
inline clock_t getMilliSecs() {
  return clock() / (CLOCKS_PER_SEC / 1000);
}

/**
 * computes dest xor data for BLOCKSIZE bytes and stores the result in dest
 * @param dest input1 and output
 * @param data data to xor to dest
 */
inline void xorFullBlocks (std::byte *dest, std::byte *data) {

#ifdef AVX512
  auto *a = reinterpret_cast<__m512i *>(dest);
  auto *b = reinterpret_cast<__m512i *>(data);
  __m512i X, Y;
  for (std::size_t i = 0; i < BLOCKSIZE / 64; ++i) {
    X = _mm512_loadu_si512 (&a[i]);
    Y = _mm512_loadu_si512 (&b[i]);
    X = _mm512_xor_si512(X, Y);
    _mm512_storeu_si512(&a[i], X);
  }
  for (std::size_t i = std::floor(BLOCKSIZE / 64) * 64; i < BLOCKSIZE; ++i) {
    dest[i] ^= data[i];
  }

#elif defined(AVX2)
  __m256i *a = reinterpret_cast<__m256i *>(dest);
  __m256i *b = reinterpret_cast<__m256i *>(data);
  __m256i X, Y;
  for (std::size_t i = 0; i < BLOCKSIZE / 32; ++i) {
    X = _mm256_loadu_si256 (&a[i]);
    Y = _mm256_loadu_si256 (&b[i]);
    X = _mm256_xor_si256(X, Y);
    _mm256_storeu_si256(&a[i], X);
  }
  for (std::size_t i = static_cast<std::size_t>(BLOCKSIZE / 32) * 32; i < BLOCKSIZE; ++i) {
    dest[i] ^= data[i];
  }

#elif defined (SSE)
  auto *a = reinterpret_cast<__m128i *>(dest);
  auto *b = reinterpret_cast<__m128i *>(data);
  __m128i X, Y;
  for (std::size_t i = 0; i < BLOCKSIZE / 16; ++i) {
    X = _mm_loadu_si128 (&a[i]);
    Y = _mm_loadu_si128 (&b[i]);
    X = _mm_xor_si128(X, Y);
    _mm_storeu_si128(&a[i], X);
  }
  for (std::size_t i = std::floor(BLOCKSIZE / 16) * 16; i < BLOCKSIZE; ++i) {
    dest[i] ^= data[i];
  }
#else
#pragma omp simd
  for (std::size_t i = 0; i < BLOCKSIZE; ++i) {
    dest[i] = data[i] ^ dest[i];
  }
#endif
}

/**
 * computes xor on all groups depending on the given query and stores the result in dest
 * @param dest output
 * @param data data to xor
 * @param query query which specifies the data to xor
 * @param groups number of groups to xor
 */
inline void fastxor (std::byte *dest, std::byte *data, const std::byte *query, const uint64_t groups) {
  std::size_t offset = 0;
#ifdef OPEN_MP
#pragma omp parallel for shared (offset)
#endif
  for (std::size_t i = 0; i < groups; ++i) {
    std::size_t temp = offset + (static_cast<std::size_t> (query[i]) * BLOCKSIZE);
    offset += preOffset;
#ifdef OPEN_MP
#pragma omp critical
#endif
    xorFullBlocks (dest, &(data)[temp]);
  }
}

#ifdef MANUAL_THREADING
inline void combineDest (std::byte *dest, const size_t entries) {
  if (entries == 1) {
    return;
  } else if (entries - 4 < 0) {
    xorFullBlocks (dest, dest + BLOCKSIZE);
    if (entries == 3) {
      xorFullBlocks (dest, dest + 2 * BLOCKSIZE);
    }
    return;
  }

  size_t left = entries / 2;
  size_t right = entries - left;

  combineDest(dest, left);
  std::byte *rightData = dest + left * BLOCKSIZE;
  combineDest(rightData, right);

  xorFullBlocks(dest, rightData);
}

inline void completexor (std::byte *dest, const std::byte *data, const std::byte *query, const uint64_t groups) {
  const uint32_t numThreads = THREADS_SUPPORTED - 2;
  std::byte *results = (std::byte *) calloc(numThreads * BLOCKSIZE, 1);
  std::vector<std::unique_ptr<boost::thread>> threads (numThreads);
  size_t groupsPerThreads = std::floor(groups / numThreads);
  size_t remainingGroups = groups % numThreads;
  size_t currentGroup = 0;
  for (size_t i = 0; i < numThreads; ++i) {
    size_t totalGroups = groupsPerThreads + (remainingGroups > i);
    threads[i] = boost::make_unique<boost::thread>(fastxor,
        results + i * BLOCKSIZE,
        data + currentGroup * preOffset,
        query + currentGroup,
        totalGroups);
    currentGroup += totalGroups;
  }
  for (size_t i = 0; i < numThreads; ++i) {
    threads[i]->join();
  }

  combineDest(results, numThreads);
  memcpy(dest, results, BLOCKSIZE);
}

#endif

/**
 * computes dest xor data for a given number of bytes
 * @param dest input1 and output
 * @param data input2
 * @param bytes number of bytes to xor
 */
inline void xorBytes (std::byte *dest, const std::byte *data, const uint64_t bytes) {
#pragma omp parallel for simd
  for (std::size_t i = 0; i < bytes; ++i) {
    dest[i] ^= data[i];
  }
}

/**
 * count the number of 0 front bits in an entry
 * @param db entry
 * @param maxLength length of entry
 * @return number of 0 front bits
 */
inline uint64_t count_0bits (std::byte *db, uint64_t maxLength) {
  uint64_t length = 0;
  uint8_t val = 0;
  for (uint64_t i = 0; i < maxLength; ++i) {
    length += 8;
    val = (uint8_t) db[i];
    if (val != 0) {
      while (val != 0) {
        val >>= 1u;
        --length;
      }
      break;
    }
  }
  return length;
}

#endif //CIP_PIR_SRC_CIP_PIR_HELPER_H_
