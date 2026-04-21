#include <arm_neon.h>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace neon {
static  __attribute__((always_inline)) inline uint64_t neon_mask4x16_to_u64(uint8x16_t m0, uint8x16_t m1,
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

inline __attribute__((always_inline)) std::pair<uint64_t, uint64_t> classify(const uint8_t* base) {
	const uint8x16_t op_table = (uint8x16_t){
		0xff, 0, ',', ':', 0, '[', ']', '{', '}', 0, 0, 0, 0, 0, 0, 0
	};
	const uint8x16_t ws_table = (uint8x16_t){
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0, 0xff, 0, 0
	};
	const uint8x16_t three = vdupq_n_u8(3);
	const uint8x16_t space = vdupq_n_u8(' ');

	const uint8x16_t d0 = vld1q_u8(base + 0);
	const uint8x16_t d1 = vld1q_u8(base + 16);
	const uint8x16_t d2 = vld1q_u8(base + 32);
	const uint8x16_t d3 = vld1q_u8(base + 48);

	const uint8x16_t match_op0 = vceqq_u8(
		vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d0, three), 4)), d0);
	const uint8x16_t match_op1 = vceqq_u8(
		vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d1, three), 4)), d1);
	const uint8x16_t match_op2 = vceqq_u8(
		vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d2, three), 4)), d2);
	const uint8x16_t match_op3 = vceqq_u8(
		vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d3, three), 4)), d3);

	const uint8x16_t match_ws0 = vqtbx1q_u8(vceqq_u8(d0, space), ws_table, d0);
	const uint8x16_t match_ws1 = vqtbx1q_u8(vceqq_u8(d1, space), ws_table, d1);
	const uint8x16_t match_ws2 = vqtbx1q_u8(vceqq_u8(d2, space), ws_table, d2);
	const uint8x16_t match_ws3 = vqtbx1q_u8(vceqq_u8(d3, space), ws_table, d3);

	const uint64_t op = neon_mask4x16_to_u64(
		match_op0, match_op1, match_op2, match_op3);
	const uint64_t whitespace = neon_mask4x16_to_u64(
		match_ws0, match_ws1, match_ws2, match_ws3);

	return {whitespace, op};
}
}