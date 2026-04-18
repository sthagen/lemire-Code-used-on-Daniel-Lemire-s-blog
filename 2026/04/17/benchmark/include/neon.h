#include <arm_neon.h>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace neon {
static inline uint64_t neon_mask4x16_to_u64(uint8x16_t m0, uint8x16_t m1,
											uint8x16_t m2, uint8x16_t m3) {
	static const uint8_t weights_data[16] = {
		1, 2, 4, 8, 16, 32, 64, 128,
		1, 2, 4, 8, 16, 32, 64, 128
	};
	const uint8x16_t weights = vld1q_u8(weights_data);

	m0 = vandq_u8(m0, weights);
	m1 = vandq_u8(m1, weights);
	m2 = vandq_u8(m2, weights);
	m3 = vandq_u8(m3, weights);

	const uint8x16_t p1a = vpaddq_u8(m0, m1);
	const uint8x16_t p1b = vpaddq_u8(m2, m3);
	const uint8x16_t p2 = vpaddq_u8(p1a, p1b);
	const uint8x16_t p3 = vpaddq_u8(p2, p2);

	return vgetq_lane_u64(vreinterpretq_u64_u8(p3), 0);
}

inline std::pair<uint64_t, uint64_t> classify(const uint8_t* base) {
	static const uint8_t table1_data[16] = {
		16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0
	};
	static const uint8_t table2_data[16] = {
		8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0
	};

	const uint8x16_t table1 = vld1q_u8(table1_data);
	const uint8x16_t table2 = vld1q_u8(table2_data);
	const uint8x16_t low_nibble_mask = vdupq_n_u8(0x0f);
	const uint8x16_t op_mask = vdupq_n_u8(0x07);
	const uint8x16_t ws_mask = vdupq_n_u8(0x18);
	const uint8x16_t zero = vdupq_n_u8(0);

	const uint8x16_t d0 = vld1q_u8(base + 0);
	const uint8x16_t d1 = vld1q_u8(base + 16);
	const uint8x16_t d2 = vld1q_u8(base + 32);
	const uint8x16_t d3 = vld1q_u8(base + 48);

	const uint8x16_t v0 = vandq_u8(
		vqtbl1q_u8(table1, vandq_u8(d0, low_nibble_mask)),
		vqtbl1q_u8(table2, vshrq_n_u8(d0, 4))
	);
	const uint8x16_t v1 = vandq_u8(
		vqtbl1q_u8(table1, vandq_u8(d1, low_nibble_mask)),
		vqtbl1q_u8(table2, vshrq_n_u8(d1, 4))
	);
	const uint8x16_t v2 = vandq_u8(
		vqtbl1q_u8(table1, vandq_u8(d2, low_nibble_mask)),
		vqtbl1q_u8(table2, vshrq_n_u8(d2, 4))
	);
	const uint8x16_t v3 = vandq_u8(
		vqtbl1q_u8(table1, vandq_u8(d3, low_nibble_mask)),
		vqtbl1q_u8(table2, vshrq_n_u8(d3, 4))
	);

	const uint64_t op = neon_mask4x16_to_u64(
		vcgtq_u8(vandq_u8(v0, op_mask), zero),
		vcgtq_u8(vandq_u8(v1, op_mask), zero),
		vcgtq_u8(vandq_u8(v2, op_mask), zero),
		vcgtq_u8(vandq_u8(v3, op_mask), zero)
	);

	const uint64_t whitespace = neon_mask4x16_to_u64(
		vcgtq_u8(vandq_u8(v0, ws_mask), zero),
		vcgtq_u8(vandq_u8(v1, ws_mask), zero),
		vcgtq_u8(vandq_u8(v2, ws_mask), zero),
		vcgtq_u8(vandq_u8(v3, ws_mask), zero)
	);

	return {whitespace, op};
}
}