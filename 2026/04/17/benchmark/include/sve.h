#include <arm_sve.h>
#include <arm_neon_sve_bridge.h>
static_assert(__ARM_FEATURE_SVE_BITS == 128,
              "This code requires -msve-vector-bits=128");

#include <arm_sve.h>
#include <arm_neon.h>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace sve {

// : , [ ] { }
static const uint8_t op_chars_data[16] = {
    0x3a, 0x2c, 0x5b, 0x5d, 0x7b, 0x7d, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
// \t \n \r ' '
static const uint8_t ws_chars_data[16] = {
    0x09, 0x0a, 0x0d, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Collapse four 16-lane predicates into a single uint64_t.
// Lane i of m_k lands at bit (k*16 + i).
static inline uint64_t pred4x16_to_u64(svbool_t m0, svbool_t m1,
                                       svbool_t m2, svbool_t m3) {
    // 0x00/0xFF materialization of each predicate in NEON form.
    uint8x16_t b0 = svget_neonq_u8(svdup_n_u8_z(m0, 0xFF));
    uint8x16_t b1 = svget_neonq_u8(svdup_n_u8_z(m1, 0xFF));
    uint8x16_t b2 = svget_neonq_u8(svdup_n_u8_z(m2, 0xFF));
    uint8x16_t b3 = svget_neonq_u8(svdup_n_u8_z(m3, 0xFF));

    // Per-lane bit weights, repeated for the two 8-byte halves of each 16-byte chunk.
    static const uint8_t weights_data[16] = {
        1, 2, 4, 8, 16, 32, 64, 128,
        1, 2, 4, 8, 16, 32, 64, 128
    };
    const uint8x16_t w = vld1q_u8(weights_data);

    // Each lane becomes its bit weight (0xFF & w = w) or zero.
    b0 = vandq_u8(b0, w);
    b1 = vandq_u8(b1, w);
    b2 = vandq_u8(b2, w);
    b3 = vandq_u8(b3, w);

    // Tree of pairwise adds. vpaddq_u8 adds adjacent byte pairs across two
    // 16-byte vectors, giving 16 output bytes: 8 from the first operand, 8 from the second.
    //
    // After p1: each pair of adjacent weighted lanes has been summed.
    //   p1a = [b0 pairs | b1 pairs], p1b = [b2 pairs | b3 pairs]
    uint8x16_t p1a = vpaddq_u8(b0, b1);
    uint8x16_t p1b = vpaddq_u8(b2, b3);

    // After p2: each group of 4 lanes summed.
    //   p2  = [b0 quads(4) | b1 quads(4) | b2 quads(4) | b3 quads(4)]
    uint8x16_t p2  = vpaddq_u8(p1a, p1b);

    // After p3: each group of 8 lanes summed.
    //   p3  = [b0 octs(2) | b1 octs(2) | b2 octs(2) | b3 octs(2) | (same again in high 8 bytes)]
    // vpaddq_u8(p2, p2) duplicates, which is fine — we only use the low 8 bytes.
    uint8x16_t p3  = vpaddq_u8(p2, p2);

    // Low 8 bytes of p3: [b0_lo, b0_hi, b1_lo, b1_hi, b2_lo, b2_hi, b3_lo, b3_hi]
    // where b*_lo is the movemask of lanes 0..7 and b*_hi is lanes 8..15.
    // That's exactly the uint64_t we want, little-endian.
    return vgetq_lane_u64(vreinterpretq_u64_u8(p3), 0);
}

inline std::pair<uint64_t, uint64_t> classify(const uint8_t* base) {
    const svbool_t pg_all   = svptrue_b8();
    const svbool_t pg       = svptrue_pat_b8(SV_VL16);
    const svuint8_t op_chars = svld1_u8(pg_all, op_chars_data);
    const svuint8_t ws_chars = svld1_u8(pg_all, ws_chars_data);

    svuint8_t d0 = svld1_u8(pg, base +  0);
    svuint8_t d1 = svld1_u8(pg, base + 16);
    svuint8_t d2 = svld1_u8(pg, base + 32);
    svuint8_t d3 = svld1_u8(pg, base + 48);

    svbool_t op0 = svmatch_u8(pg, d0, op_chars);
    svbool_t op1 = svmatch_u8(pg, d1, op_chars);
    svbool_t op2 = svmatch_u8(pg, d2, op_chars);
    svbool_t op3 = svmatch_u8(pg, d3, op_chars);

    svbool_t ws0 = svmatch_u8(pg, d0, ws_chars);
    svbool_t ws1 = svmatch_u8(pg, d1, ws_chars);
    svbool_t ws2 = svmatch_u8(pg, d2, ws_chars);
    svbool_t ws3 = svmatch_u8(pg, d3, ws_chars);

    return {
        pred4x16_to_u64(op0, op1, op2, op3),
        pred4x16_to_u64(ws0, ws1, ws2, ws3)
    };
}   
}