#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <print>
#include <random>
#include <string>
#include <vector>

#include "counters/bench.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif



struct array_container_t {
    const uint16_t *array;
    size_t cardinality;
};


/* Check whether x is present.  */
inline bool array_container_contains_old(const array_container_t *arr,
                                     uint16_t pos) {
    int32_t low = 0;
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t high = arr->cardinality - 1;
    while (high >= low + 16) {
        int32_t middleIndex = (low + high) >> 1;
        uint16_t middleValue = carr[middleIndex];
        if (middleValue < pos) {
            low = middleIndex + 1;
        } else if (middleValue > pos) {
            high = middleIndex - 1;
        } else {
            return true;
        }
    }

    for (int i = low; i <= high; i++) {
        uint16_t v = carr[i];
        if (v == pos) {
            return true;
        }
        if (v > pos) return false;
    }
    return false;
}


inline bool array_container_contains(const array_container_t *arr,
                                     uint16_t pos) {
    int32_t low = 0;
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t high = arr->cardinality - 1;
    if (high > pos) {
        // since elements are unique, x can be located only at index <= x
        high = pos;
    }
    while (high >= low + 16) {
        int32_t middleIndex = (low + high) >> 1;
        uint16_t middleValue = carr[middleIndex];
        if (middleValue < pos) {
            low = middleIndex + 1;
        } else if (middleValue > pos) {
            high = middleIndex - 1;
        } else {
            return true;
        }
    }

    while (high >= low + 3) {
        if (carr[low + 3] >= pos) {
            return (carr[low] == pos) || (carr[low + 1] == pos) ||
                   (carr[low + 2] == pos) || (carr[low + 3] == pos);
        }
        low += 4;
    }

    for (int i = low; i <= high; i++) {
        uint16_t v = carr[i];
        if (v >= pos) {
            return (v == pos);  // compiles into SETE
        }
    }
    return false;
}

template <size_t gap>
inline bool array_container_contains_naive(const array_container_t *arr,
                                          uint16_t pos) {
    int32_t low = 0;
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t high = arr->cardinality - 1;
  

    while (high >= low + gap) {
        if (carr[low + gap] > pos) {
          for(size_t r = 0; r < gap; r++) {
                if (carr[low + r] == pos) return true;
          }
          return false;
        }
        low += gap;
    }

    // Scalar check for remaining
    for (int j = low; j <= high; j++) {
        uint16_t v = carr[j];
        if (v >= pos) {
            return (v == pos);
        }
    }
    return false;
}

#ifdef __ARM_NEON
template <size_t gap>
inline bool array_container_contains_neon(const array_container_t *arr,
                                          uint16_t pos) {
    int32_t low = 0;
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t high = arr->cardinality - 1;
  

    while (high >= low + gap) {
        if (carr[low + gap] > pos) {
            for(size_t r = 0; r < gap; r += 8) {
                // SIMD check the previous 8
                uint16x8_t vec = vld1q_u16(&carr[low + r]);
                uint16x8_t cmp_eq = vceqq_u16(vec, vdupq_n_u16(pos));
                if (vmaxvq_u16(cmp_eq) != 0) return true;
            }
            return false;
        }
        low += gap;
    }

    // Scalar check for remaining
    for (int j = low; j <= high; j++) {
        uint16_t v = carr[j];
        if (v >= pos) {
            return (v == pos);
        }
    }
    return false;
}
#endif

#ifdef __ARM_NEON
template <size_t gap>
inline bool array_container_contains_binary_neon(const array_container_t *arr,
                                                 uint16_t pos) {
    static_assert(gap % 8 == 0, "gap must be a multiple of 8 for NEON");
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t cardinality = arr->cardinality;
    if (cardinality > pos + 1) {
        cardinality = pos + 1;  // since elements are unique, x can be located only at index <= x
    }

    int32_t num_blocks = cardinality / gap;


    // Binary search the per-block anchors carr[(i+1)*gap - 1] to find the
    // smallest block whose last element is >= pos.
    int32_t lo = 0;
    int32_t hi = num_blocks;
    while (lo < hi) {
        int32_t mid = (lo + hi) >> 1;
        if (carr[(mid + 1) * gap - 1] < pos) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < num_blocks) {
        int32_t base = lo * gap;
        for (size_t r = 0; r < gap; r += 8) {
            uint16x8_t vec = vld1q_u16(&carr[base + r]);
            uint16x8_t cmp_eq = vceqq_u16(vec, vdupq_n_u16(pos));
            if (vmaxvq_u16(cmp_eq) != 0) return true;
        }
        return false;
    }

    // Tail: scalar scan over the partial block past num_blocks * gap.
    for (int32_t j = num_blocks * gap; j < cardinality; j++) {
        uint16_t v = carr[j];
        if (v >= pos) {
            return (v == pos);
        }
    }
    return false;
}
#endif

#ifdef __ARM_NEON
template <size_t gap>
inline bool array_container_contains_shotgun_neon(const array_container_t *arr,
                                                  uint16_t pos) {
    static_assert(gap % 8 == 0, "gap must be a multiple of 8 for NEON");
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t cardinality = arr->cardinality;
    int32_t num_blocks = cardinality / gap;

    if (num_blocks == 0) {
        for (int32_t j = 0; j < cardinality; j++) {
            uint16_t v = carr[j];
            if (v >= pos) return (v == pos);
        }
        return false;
    }

    // Branchless binary search over per-block anchors carr[(i+1)*gap - 1]
    // to find the smallest block index `lo` whose anchor is >= pos.
    int32_t base = 0;
    int32_t n = num_blocks;
    while (n > 3) {
      int32_t quarter = n >> 2;
      
      // Issue all three loads in parallel - no data dependency between them
      int32_t k1 = carr[(base + quarter + 1) * gap - 1];
      int32_t k2 = carr[(base + 2 * quarter + 1) * gap - 1];
      int32_t k3 = carr[(base + 3 * quarter + 1) * gap - 1];
      
      // Now resolve which quarter pos falls into
      int32_t c1 = (k1 < pos);
      int32_t c2 = (k2 < pos);
      int32_t c3 = (k3 < pos);
      
      base += (c1 + c2 + c3) * quarter;
      n -= 3 * quarter;
    }
    while (n > 1) {
        int32_t half = n >> 1;
        base = (carr[(base + half + 1) * gap - 1] < pos) ? base + half : base;
        n -= half;
    }
    int32_t lo = (carr[(base + 1) * gap - 1] < pos) ? base + 1 : base;

    if (lo < num_blocks) {
        int32_t blk_base = lo * gap;
        for (size_t r = 0; r < gap; r += 8) {
            uint16x8_t vec = vld1q_u16(&carr[blk_base + r]);
            uint16x8_t cmp_eq = vceqq_u16(vec, vdupq_n_u16(pos));
            if (vmaxvq_u16(cmp_eq) != 0) return true;
        }
        return false;
    }

    // Tail beyond num_blocks * gap.
    for (int32_t j = num_blocks * gap; j < cardinality; j++) {
        uint16_t v = carr[j];
        if (v >= pos) return (v == pos);
    }
    return false;
}
#endif

#ifdef __ARM_NEON
inline bool array_container_contains_hybrid_neon(const array_container_t *arr,
                                                 uint16_t pos) {
    size_t card = arr->cardinality;
    if (card <= 16) {
        const uint16_t *carr = arr->array;
        for (size_t i = 0; i < card; i++) {
            if (carr[i] == pos) return true;
        }
        return false;
    }
    if (card <= 32) return array_container_contains_binary_neon<32>(arr, pos);
    return array_container_contains_neon<32>(arr, pos);
}
#endif

#ifdef __SSE2__
template <size_t gap>
inline bool array_container_contains_sse2(const array_container_t *arr,
                                          uint16_t pos) {
    int32_t low = 0;
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t high = arr->cardinality - 1;


    while (high >= low + gap) {
        if (carr[low + gap] > pos) {
            __m128i needle = _mm_set1_epi16((short)pos);
            for(size_t r = 0; r < gap; r += 8) {
                // SIMD check the previous 8
                __m128i vec = _mm_loadu_si128((const __m128i *)&carr[low + r]);
                __m128i cmp_eq = _mm_cmpeq_epi16(vec, needle);
                if (_mm_movemask_epi8(cmp_eq) != 0) return true;
            }
            return false;
        }
        low += gap;
    }

    // Scalar check for remaining
    for (int j = low; j <= high; j++) {
        uint16_t v = carr[j];
        if (v >= pos) {
            return (v == pos);
        }
    }
    return false;
}
#endif

#ifdef __SSE2__
template <size_t gap>
inline bool array_container_contains_binary_sse2(const array_container_t *arr,
                                                 uint16_t pos) {
    static_assert(gap % 8 == 0, "gap must be a multiple of 8 for SSE2");
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t cardinality = arr->cardinality;
    if (cardinality > pos + 1) {
        cardinality = pos + 1;  // since elements are unique, x can be located only at index <= x
    }

    int32_t num_blocks = cardinality / gap;


    // Binary search the per-block anchors carr[(i+1)*gap - 1] to find the
    // smallest block whose last element is >= pos.
    int32_t lo = 0;
    int32_t hi = num_blocks;
    while (lo < hi) {
        int32_t mid = (lo + hi) >> 1;
        if (carr[(mid + 1) * gap - 1] < pos) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < num_blocks) {
        int32_t base = lo * gap;
        __m128i needle = _mm_set1_epi16((short)pos);
        for (size_t r = 0; r < gap; r += 8) {
            __m128i vec = _mm_loadu_si128((const __m128i *)&carr[base + r]);
            __m128i cmp_eq = _mm_cmpeq_epi16(vec, needle);
            if (_mm_movemask_epi8(cmp_eq) != 0) return true;
        }
        return false;
    }

    // Tail: scalar scan over the partial block past num_blocks * gap.
    for (int32_t j = num_blocks * gap; j < cardinality; j++) {
        uint16_t v = carr[j];
        if (v >= pos) {
            return (v == pos);
        }
    }
    return false;
}
#endif

#ifdef __SSE2__
template <size_t gap>
inline bool array_container_contains_shotgun_sse2(const array_container_t *arr,
                                                  uint16_t pos) {
    static_assert(gap % 8 == 0, "gap must be a multiple of 8 for SSE2");
    const uint16_t *carr = (const uint16_t *)arr->array;
    int32_t cardinality = arr->cardinality;
    int32_t num_blocks = cardinality / gap;

    if (num_blocks == 0) {
        for (int32_t j = 0; j < cardinality; j++) {
            uint16_t v = carr[j];
            if (v >= pos) return (v == pos);
        }
        return false;
    }

    // Branchless binary search over per-block anchors carr[(i+1)*gap - 1]
    // to find the smallest block index `lo` whose anchor is >= pos.
    int32_t base = 0;
    int32_t n = num_blocks;
    while (n > 3) {
      int32_t quarter = n >> 2;

      // Issue all three loads in parallel - no data dependency between them
      int32_t k1 = carr[(base + quarter + 1) * gap - 1];
      int32_t k2 = carr[(base + 2 * quarter + 1) * gap - 1];
      int32_t k3 = carr[(base + 3 * quarter + 1) * gap - 1];

      // Now resolve which quarter pos falls into
      int32_t c1 = (k1 < pos);
      int32_t c2 = (k2 < pos);
      int32_t c3 = (k3 < pos);

      base += (c1 + c2 + c3) * quarter;
      n -= 3 * quarter;
    }
    while (n > 1) {
        int32_t half = n >> 1;
        base = (carr[(base + half + 1) * gap - 1] < pos) ? base + half : base;
        n -= half;
    }
    int32_t lo = (carr[(base + 1) * gap - 1] < pos) ? base + 1 : base;

    if (lo < num_blocks) {
        int32_t blk_base = lo * gap;
        __m128i needle = _mm_set1_epi16((short)pos);
        for (size_t r = 0; r < gap; r += 8) {
            __m128i vec = _mm_loadu_si128((const __m128i *)&carr[blk_base + r]);
            __m128i cmp_eq = _mm_cmpeq_epi16(vec, needle);
            if (_mm_movemask_epi8(cmp_eq) != 0) return true;
        }
        return false;
    }

    // Tail beyond num_blocks * gap.
    for (int32_t j = num_blocks * gap; j < cardinality; j++) {
        uint16_t v = carr[j];
        if (v >= pos) return (v == pos);
    }
    return false;
}
#endif

#ifdef __SSE2__
inline bool array_container_contains_hybrid_sse2(const array_container_t *arr,
                                                 uint16_t pos) {
    size_t card = arr->cardinality;
    if (card <= 16) {
        const uint16_t *carr = arr->array;
        for (size_t i = 0; i < card; i++) {
            if (carr[i] == pos) return true;
        }
        return false;
    }
    if (card <= 32) return array_container_contains_binary_sse2<32>(arr, pos);
    return array_container_contains_sse2<32>(arr, pos);
}
#endif


// All make_bench_* helpers walk a flat (indices, keys) sequence: query i hits
// arrays[indices[i]] with keys[i]. Cold and warm modes use the same loop body
// (so per-query instruction counts match) and differ only in `indices`:
//   cold: indices[i] = i % N           (round-robin → each revisit is cache-cold)
//   warm: indices[i] = i / warmth      (warmth consecutive queries on same array)

template <size_t gap>
auto make_bench_naive_search(const std::vector<std::vector<uint16_t>>& arrays,
                             const std::vector<size_t>& indices,
                             const std::vector<uint16_t>& keys,
                             volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_naive<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

#ifdef __ARM_NEON
template <size_t gap>
auto make_bench_neon_search(const std::vector<std::vector<uint16_t>>& arrays,
                            const std::vector<size_t>& indices,
                            const std::vector<uint16_t>& keys,
                            volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_neon<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

template <size_t gap>
auto make_bench_binary_neon_search(const std::vector<std::vector<uint16_t>>& arrays,
                                   const std::vector<size_t>& indices,
                                   const std::vector<uint16_t>& keys,
                                   volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_binary_neon<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

template <size_t gap>
auto make_bench_shotgun_neon_search(const std::vector<std::vector<uint16_t>>& arrays,
                                    const std::vector<size_t>& indices,
                                    const std::vector<uint16_t>& keys,
                                    volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_shotgun_neon<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

inline auto make_bench_hybrid_neon_search(const std::vector<std::vector<uint16_t>>& arrays,
                                          const std::vector<size_t>& indices,
                                          const std::vector<uint16_t>& keys,
                                          volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_hybrid_neon(&container, keys[i])) ++c;
    }
    counter += c;
  };
}
#endif

#ifdef __SSE2__
template <size_t gap>
auto make_bench_sse2_search(const std::vector<std::vector<uint16_t>>& arrays,
                            const std::vector<size_t>& indices,
                            const std::vector<uint16_t>& keys,
                            volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_sse2<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

template <size_t gap>
auto make_bench_binary_sse2_search(const std::vector<std::vector<uint16_t>>& arrays,
                                   const std::vector<size_t>& indices,
                                   const std::vector<uint16_t>& keys,
                                   volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_binary_sse2<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

template <size_t gap>
auto make_bench_shotgun_sse2_search(const std::vector<std::vector<uint16_t>>& arrays,
                                    const std::vector<size_t>& indices,
                                    const std::vector<uint16_t>& keys,
                                    volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_shotgun_sse2<gap>(&container, keys[i])) ++c;
    }
    counter += c;
  };
}

inline auto make_bench_hybrid_sse2_search(const std::vector<std::vector<uint16_t>>& arrays,
                                          const std::vector<size_t>& indices,
                                          const std::vector<uint16_t>& keys,
                                          volatile uint64_t& counter) {
  return [&arrays, &indices, &keys, &counter]() {
    size_t c = 0;
    size_t n = indices.size();
    for (size_t i = 0; i < n; ++i) {
      const auto &arr = arrays[indices[i]];
      array_container_t container{arr.data(), arr.size()};
      if (array_container_contains_hybrid_sse2(&container, keys[i])) ++c;
    }
    counter += c;
  };
}
#endif

double pretty_print(const std::string &name, const std::string &mode,
                    size_t num_values, counters::event_aggregate agg) {
  std::print("{:<40} {:<4} : ", name, mode);
  std::print(" {:5.3f} ns ", agg.fastest_elapsed_ns() / double(num_values));
  std::print(" {:5.2f} Gv/s ", double(num_values) / agg.fastest_elapsed_ns());
  if (counters::has_performance_counters()) {
    std::print(" {:5.2f} GHz ", agg.cycles() / double(agg.elapsed_ns()));
    std::print(" {:5.2f} c ", agg.fastest_cycles() / double(num_values));
    std::print(" {:5.2f} i ", agg.fastest_instructions() / double(num_values));
    std::print(" {:5.2f} bm ", agg.branch_misses() / double(num_values));
    std::print(" {:5.2f} i/c ",
               agg.fastest_instructions() / double(agg.fastest_cycles()));
  }
  std::print("\n");
  return double(num_values) / agg.fastest_elapsed_ns();
}

void collect_benchmark_results(size_t array_size, size_t number_arrays,
                               size_t warmth, bool run_cold, bool run_warm) {
  if (array_size > 65536) {
    array_size = 65536;
  }
  std::mt19937_64 rng(42);

  // Generate `number_arrays` sorted arrays of uint16_t with no duplicates.
  std::vector<std::vector<uint16_t>> arrays;
  arrays.reserve(number_arrays);
  std::vector<uint32_t> pool(65536);
  for (uint32_t i = 0; i < 65536; ++i) {
    pool[i] = i;
  }
  for (size_t a = 0; a < number_arrays; ++a) {
    for (size_t i = 0; i < array_size; ++i) {
      std::uniform_int_distribution<size_t> dist(i, pool.size() - 1);
      size_t j = dist(rng);
      std::swap(pool[i], pool[j]);
    }
    std::vector<uint16_t> arr(array_size);
    for (size_t i = 0; i < array_size; ++i) {
      arr[i] = uint16_t(pool[i]);
    }
    std::sort(arr.begin(), arr.end());
    arrays.push_back(std::move(arr));
  }

  // Same total queries for both modes; only the array access pattern differs.
  size_t total = number_arrays * warmth;

  std::vector<size_t> indices_cold(total);
  for (size_t i = 0; i < total; ++i) {
    indices_cold[i] = i % number_arrays;       // round-robin: revisits are cache-cold
  }
  std::vector<size_t> indices_warm(total);
  for (size_t i = 0; i < total; ++i) {
    indices_warm[i] = i / warmth;              // warmth queries in a row on same array
  }

  std::vector<uint16_t> keys(total);
  std::uniform_int_distribution<uint32_t> key_dist(0, 65535);
  for (size_t i = 0; i < total; ++i) {
    keys[i] = uint16_t(key_dist(rng));
  }

  volatile uint64_t counter = 0;

  std::print("arrays: {}  size per array: {}  warmth: {}  total queries per mode: {}\n",
             number_arrays, array_size, warmth, total);

  // Verification phase: every algorithm must agree with std::binary_search on
  // a slice of the generated queries. Aborts on the first batch of mismatches.
  {
    size_t check_count = std::min<size_t>(total, size_t(10000));
    size_t mismatches = 0;
    auto check_one = [&](const char *name, bool got, bool expected,
                         const std::vector<uint16_t> &arr, uint16_t k) {
      if (got != expected) {
        if (mismatches < 5) {
          std::print("MISMATCH: {} arr_size={} key={} expected={} got={}\n",
                     name, arr.size(), k, expected, got);
        }
        ++mismatches;
      }
    };
    for (size_t i = 0; i < check_count; ++i) {
      const auto &arr = arrays[indices_cold[i]];
      uint16_t k = keys[i];
      array_container_t container{arr.data(), arr.size()};
      bool expected = std::binary_search(arr.begin(), arr.end(), k);
      check_one("custom_search", array_container_contains(&container, k), expected, arr, k);
      check_one("old_custom_search", array_container_contains_old(&container, k), expected, arr, k);
      check_one("naive_search<8>",  array_container_contains_naive<8>(&container, k),  expected, arr, k);
      check_one("naive_search<16>", array_container_contains_naive<16>(&container, k), expected, arr, k);
      check_one("naive_search<24>", array_container_contains_naive<24>(&container, k), expected, arr, k);
      check_one("naive_search<32>", array_container_contains_naive<32>(&container, k), expected, arr, k);
#ifdef __ARM_NEON
      check_one("neon_search<8>",  array_container_contains_neon<8>(&container, k),  expected, arr, k);
      check_one("neon_search<16>", array_container_contains_neon<16>(&container, k), expected, arr, k);
      check_one("neon_search<24>", array_container_contains_neon<24>(&container, k), expected, arr, k);
      check_one("neon_search<32>", array_container_contains_neon<32>(&container, k), expected, arr, k);
      check_one("binary_neon_search<8>",  array_container_contains_binary_neon<8>(&container, k),  expected, arr, k);
      check_one("binary_neon_search<16>", array_container_contains_binary_neon<16>(&container, k), expected, arr, k);
      check_one("binary_neon_search<24>", array_container_contains_binary_neon<24>(&container, k), expected, arr, k);
      check_one("binary_neon_search<32>", array_container_contains_binary_neon<32>(&container, k), expected, arr, k);
      check_one("shotgun_neon_search<8>",  array_container_contains_shotgun_neon<8>(&container, k),  expected, arr, k);
      check_one("shotgun_neon_search<16>", array_container_contains_shotgun_neon<16>(&container, k), expected, arr, k);
      check_one("shotgun_neon_search<24>", array_container_contains_shotgun_neon<24>(&container, k), expected, arr, k);
      check_one("shotgun_neon_search<32>", array_container_contains_shotgun_neon<32>(&container, k), expected, arr, k);
      check_one("hybrid_neon_search", array_container_contains_hybrid_neon(&container, k), expected, arr, k);
#endif
#ifdef __SSE2__
      check_one("sse2_search<8>",  array_container_contains_sse2<8>(&container, k),  expected, arr, k);
      check_one("sse2_search<16>", array_container_contains_sse2<16>(&container, k), expected, arr, k);
      check_one("sse2_search<24>", array_container_contains_sse2<24>(&container, k), expected, arr, k);
      check_one("sse2_search<32>", array_container_contains_sse2<32>(&container, k), expected, arr, k);
      check_one("binary_sse2_search<8>",  array_container_contains_binary_sse2<8>(&container, k),  expected, arr, k);
      check_one("binary_sse2_search<16>", array_container_contains_binary_sse2<16>(&container, k), expected, arr, k);
      check_one("binary_sse2_search<24>", array_container_contains_binary_sse2<24>(&container, k), expected, arr, k);
      check_one("binary_sse2_search<32>", array_container_contains_binary_sse2<32>(&container, k), expected, arr, k);
      check_one("shotgun_sse2_search<8>",  array_container_contains_shotgun_sse2<8>(&container, k),  expected, arr, k);
      check_one("shotgun_sse2_search<16>", array_container_contains_shotgun_sse2<16>(&container, k), expected, arr, k);
      check_one("shotgun_sse2_search<24>", array_container_contains_shotgun_sse2<24>(&container, k), expected, arr, k);
      check_one("shotgun_sse2_search<32>", array_container_contains_shotgun_sse2<32>(&container, k), expected, arr, k);
      check_one("hybrid_sse2_search", array_container_contains_hybrid_sse2(&container, k), expected, arr, k);
#endif
    }
    if (mismatches > 0) {
      std::print("FAIL: {} mismatches across {} queries — aborting\n", mismatches, check_count);
      std::exit(1);
    }
    std::print("verification passed: {} queries × all algorithms agree with std::binary_search\n",
               check_count);
  }

  auto run_pair = [&](const char *base, auto cold_make, auto warm_make) {
    if (run_cold) {
      pretty_print(base, "cold", total, counters::bench(cold_make()));
    }
    if (run_warm) {
      pretty_print(base, "warm", total, counters::bench(warm_make()));
    }
  };

#define RUN_PAIR_TEMPLATE(NAME, MAKER) \
  run_pair(NAME, \
    [&]{ return MAKER(arrays, indices_cold, keys, counter); }, \
    [&]{ return MAKER(arrays, indices_warm, keys, counter); })

  RUN_PAIR_TEMPLATE("naive_search8",  make_bench_naive_search<8>);
  RUN_PAIR_TEMPLATE("naive_search16", make_bench_naive_search<16>);
  RUN_PAIR_TEMPLATE("naive_search24", make_bench_naive_search<24>);
  RUN_PAIR_TEMPLATE("naive_search32", make_bench_naive_search<32>);
#ifdef __ARM_NEON
  RUN_PAIR_TEMPLATE("binary_neon_search8",  make_bench_binary_neon_search<8>);
  RUN_PAIR_TEMPLATE("binary_neon_search16", make_bench_binary_neon_search<16>);
  RUN_PAIR_TEMPLATE("binary_neon_search24", make_bench_binary_neon_search<24>);
  RUN_PAIR_TEMPLATE("binary_neon_search32", make_bench_binary_neon_search<32>);
  RUN_PAIR_TEMPLATE("shotgun_neon_search8",  make_bench_shotgun_neon_search<8>);
  RUN_PAIR_TEMPLATE("shotgun_neon_search16", make_bench_shotgun_neon_search<16>);
  RUN_PAIR_TEMPLATE("shotgun_neon_search24", make_bench_shotgun_neon_search<24>);
  RUN_PAIR_TEMPLATE("shotgun_neon_search32", make_bench_shotgun_neon_search<32>);
  RUN_PAIR_TEMPLATE("neon_search8",  make_bench_neon_search<8>);
  RUN_PAIR_TEMPLATE("neon_search16", make_bench_neon_search<16>);
  RUN_PAIR_TEMPLATE("neon_search24", make_bench_neon_search<24>);
  RUN_PAIR_TEMPLATE("neon_search32", make_bench_neon_search<32>);
  RUN_PAIR_TEMPLATE("hybrid_neon_search", make_bench_hybrid_neon_search);
#endif
#ifdef __SSE2__
  RUN_PAIR_TEMPLATE("binary_sse2_search8",  make_bench_binary_sse2_search<8>);
  RUN_PAIR_TEMPLATE("binary_sse2_search16", make_bench_binary_sse2_search<16>);
  RUN_PAIR_TEMPLATE("binary_sse2_search24", make_bench_binary_sse2_search<24>);
  RUN_PAIR_TEMPLATE("binary_sse2_search32", make_bench_binary_sse2_search<32>);
  RUN_PAIR_TEMPLATE("shotgun_sse2_search8",  make_bench_shotgun_sse2_search<8>);
  RUN_PAIR_TEMPLATE("shotgun_sse2_search16", make_bench_shotgun_sse2_search<16>);
  RUN_PAIR_TEMPLATE("shotgun_sse2_search24", make_bench_shotgun_sse2_search<24>);
  RUN_PAIR_TEMPLATE("shotgun_sse2_search32", make_bench_shotgun_sse2_search<32>);
  RUN_PAIR_TEMPLATE("sse2_search8",  make_bench_sse2_search<8>);
  RUN_PAIR_TEMPLATE("sse2_search16", make_bench_sse2_search<16>);
  RUN_PAIR_TEMPLATE("sse2_search24", make_bench_sse2_search<24>);
  RUN_PAIR_TEMPLATE("sse2_search32", make_bench_sse2_search<32>);
  RUN_PAIR_TEMPLATE("hybrid_sse2_search", make_bench_hybrid_sse2_search);
#endif
#undef RUN_PAIR_TEMPLATE

  // Inline lambdas use the same indices/keys traversal as the templated helpers.
  auto bench_inline = [&](const char *base, auto fn) {
    auto make = [&](const std::vector<size_t> &indices) {
      return [&arrays, &indices, &keys, &counter, fn]() {
        size_t c = 0;
        size_t n = indices.size();
        for (size_t i = 0; i < n; ++i) {
          if (fn(arrays[indices[i]], keys[i])) ++c;
        }
        counter += c;
      };
    };
    if (run_cold) {
      pretty_print(base, "cold", total, counters::bench(make(indices_cold)));
    }
    if (run_warm) {
      pretty_print(base, "warm", total, counters::bench(make(indices_warm)));
    }
  };

  bench_inline("binary_search", [](const std::vector<uint16_t> &arr, uint16_t k) {
    return std::binary_search(arr.begin(), arr.end(), k);
  });
  bench_inline("find", [](const std::vector<uint16_t> &arr, uint16_t k) {
    return std::find(arr.begin(), arr.end(), k) != arr.end();
  });
  bench_inline("custom_search", [](const std::vector<uint16_t> &arr, uint16_t k) {
    array_container_t container{arr.data(), arr.size()};
    return array_container_contains(&container, k);
  });
  bench_inline("old_custom_search", [](const std::vector<uint16_t> &arr, uint16_t k) {
    array_container_t container{arr.data(), arr.size()};
    return array_container_contains_old(&container, k);
  });
}

int main(int argc, char **argv) {
  size_t number_arrays = 100000;
  size_t warmth = 100;
  enum class Mode { Both, ColdOnly, WarmOnly };
  Mode mode = Mode::Both;
  std::vector<size_t> array_sizes = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--number" && i + 1 < argc) {
      number_arrays = std::stoul(argv[++i]);
    } else if (arg == "--warmth" && i + 1 < argc) {
      warmth = std::stoul(argv[++i]);
    } else if (arg == "--cold") {
      mode = Mode::ColdOnly;
    } else if (arg == "--warm") {
      mode = Mode::WarmOnly;
    } else if (arg == "--lengths" && i + 1 < argc) {
      std::string lengths_str = argv[++i];
      array_sizes.clear();
      size_t pos = 0;
      while ((pos = lengths_str.find(',')) != std::string::npos) {
        array_sizes.push_back(std::stoul(lengths_str.substr(0, pos)));
        lengths_str.erase(0, pos + 1);
      }
      array_sizes.push_back(std::stoul(lengths_str));
    }
  }
  bool run_cold = (mode != Mode::WarmOnly);
  bool run_warm = (mode != Mode::ColdOnly);
  for (size_t array_size : array_sizes) {
    collect_benchmark_results(array_size, number_arrays, warmth, run_cold, run_warm);
  }
}
