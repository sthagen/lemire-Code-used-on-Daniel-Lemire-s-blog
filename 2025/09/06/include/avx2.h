#ifndef AVX2_H
#define AVX2_H

#ifdef __AVX2__

#define HAVE32
#include <cstdint>
#include <cstring>
#include <x86intrin.h>

#include "scalar.h"
#include "ssse3.h"

/**
 * Insert a line feed character in the 64-byte input at index K in [0,32).
 */
__m256i insert_line_feed32(__m256i input, int K) {

  static const uint8_t low_table[16][32] = {
      {0x80, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0,    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 0x80, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1,    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 0x80, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2,    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 0x80, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3,    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 0x80, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3, 4,    5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 0x80, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5,    6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 0x80, 6, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6,    7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 0x80, 7, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7,    8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 0x80, 8, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8,    9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 0x80, 9,  10, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9,    10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x80, 10, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,   11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0x80, 11, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,   12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x80, 12, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,   13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0x80, 13, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,   14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0x80, 14,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,   15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 0x80,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
  static const uint8_t high_table[16][32] = {
      {0,    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0x80, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1,    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 0x80, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2,    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 0x80, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3,    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 0x80, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4,    5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 0x80, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5,    6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 0x80, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6,    7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 0x80, 6, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7,    8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 0x80, 7, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8,    9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 0x80, 8, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,    10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 0x80, 9,  10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,   11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x80, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,   12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0x80, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,   13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x80, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,   14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0x80, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,   15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0x80, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 0x80}};

  __m256i line_feed_vector = _mm256_set1_epi8('\n');
  if (K >= 16) {
    __m256i mask = _mm256_loadu_si256((const __m256i_u *)high_table[K - 16]);
    __m256i lf_pos =
        _mm256_cmpeq_epi8(mask, _mm256_set1_epi8(static_cast<char>(0x80)));
    __m256i shuffled = _mm256_shuffle_epi8(input, mask);
    __m256i result = _mm256_blendv_epi8(shuffled, line_feed_vector, lf_pos);
    return result;
  }
  // Shift input right by 1 byte
  __m256i shift = _mm256_alignr_epi8(
      input, _mm256_permute2x128_si256(input, input, 0x21), 15);

  input = _mm256_blend_epi32(input, shift, 0xF0);

  __m256i mask = _mm256_loadu_si256((const __m256i_u *)low_table[K]);

  __m256i lf_pos =
      _mm256_cmpeq_epi8(mask, _mm256_set1_epi8(static_cast<char>(0x80)));
  __m256i shuffled = _mm256_shuffle_epi8(input, mask);

  __m256i result = _mm256_blendv_epi8(shuffled, line_feed_vector, lf_pos);
  return result;
}


inline void insert_line_feed32(const char *buffer, size_t length, int K,
                        char *output) {
  if (K == 0) {
    memcpy(output, buffer, length);
    return;
  }
  size_t input_pos = 0;
  size_t next_line_feed = K;
  if (K >= 31) {
    while (input_pos + 32 <= length) {
      __m256i chunk = _mm256_loadu_si256((__m256i *)(buffer + input_pos));
      if (next_line_feed >= 32) {
        _mm256_storeu_si256((__m256i *)(output), chunk);
        output += 32;
        next_line_feed -= 32;
        input_pos += 32;
      } else {
        // we write next_line_feed bytes, then a line feed, then the rest (31 -
        // next_line_feed bytes)
        chunk = insert_line_feed32(chunk, next_line_feed);
        _mm256_storeu_si256((__m256i *)(output), chunk);
        output += 32;
        next_line_feed = K - (31 - next_line_feed);
        input_pos += 31;
      }
    }
  } else if (K >= 15) {
    while (input_pos + 16 <= length) {
      __m128i chunk = _mm_loadu_si128((__m128i *)(buffer + input_pos));
      if (next_line_feed >= 16) {
        _mm_storeu_si128((__m128i *)(output), chunk);
        output += 16;
        next_line_feed -= 16;
        input_pos += 16;
      } else {
        // we write next_line_feed bytes, then a line feed, then the rest (15 -
        // next_line_feed bytes)
        chunk = insert_line_feed16(chunk, next_line_feed);
        _mm_storeu_si128((__m128i *)(output), chunk);
        output += 16;
        next_line_feed = K - (15 - next_line_feed);
        input_pos += 15;
      }
    }
  }
  while (input_pos < length) {
    output[0] = buffer[input_pos];
    output++;
    input_pos++;
    next_line_feed--;
    if (next_line_feed == 0) {
      output[0] = '\n';
      output++;
      next_line_feed = K;
    }
  }
}

#endif // __AVX2__
#endif // AVX2_H
