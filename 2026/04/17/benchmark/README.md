
Under Linux and macOS, you may run:

```
cmake -B build
cmake --build build
./build/benchmark
```


# Instructions (GCC 16)


```
════════════════════════════════════════════════════════════════
  neon::classify  (neon_classify_asm)
════════════════════════════════════════════════════════════════
0000000000000000 <neon_classify_asm>:
   0:   ldp     q31, q30, [x0]
   4:   adrp    x4, 0 <neon_classify_asm>
   8:   ldp     q29, q28, [x0, #32]
   c:   add     x3, x4, #0x0
  10:   mov     v5.16b, v31.16b
  14:   mov     v3.16b, v30.16b
  18:   mov     v2.16b, v29.16b
  1c:   mov     v1.16b, v28.16b
  20:   ldp     q4, q24, [x3, #16]
  24:   ldr     q27, [x4]
  28:   and     z5.b, z5.b, #0xf
  2c:   ushr    v31.16b, v31.16b, #4
  30:   and     z3.b, z3.b, #0xf
  34:   ushr    v30.16b, v30.16b, #4
  38:   and     z2.b, z2.b, #0xf
  3c:   ushr    v29.16b, v29.16b, #4
  40:   ushr    v28.16b, v28.16b, #4
  44:   and     z1.b, z1.b, #0xf
  48:   tbl     v5.16b, {v27.16b}, v5.16b
  4c:   tbl     v31.16b, {v4.16b}, v31.16b
  50:   tbl     v3.16b, {v27.16b}, v3.16b
  54:   tbl     v30.16b, {v4.16b}, v30.16b
  58:   tbl     v2.16b, {v27.16b}, v2.16b
  5c:   tbl     v29.16b, {v4.16b}, v29.16b
  60:   tbl     v28.16b, {v4.16b}, v28.16b
  64:   tbl     v1.16b, {v27.16b}, v1.16b
  68:   and     v31.16b, v5.16b, v31.16b
  6c:   and     v30.16b, v3.16b, v30.16b
  70:   and     v29.16b, v2.16b, v29.16b
  74:   and     v28.16b, v1.16b, v28.16b
  78:   mov     v0.16b, v31.16b
  7c:   mov     v23.16b, v30.16b
  80:   mov     v26.16b, v29.16b
  84:   mov     v25.16b, v28.16b
  88:   and     z31.b, z31.b, #0x18
  8c:   and     z30.b, z30.b, #0x18
  90:   and     z29.b, z29.b, #0x18
  94:   and     z28.b, z28.b, #0x18
  98:   and     z0.b, z0.b, #0x7
  9c:   and     z23.b, z23.b, #0x7
  a0:   and     z26.b, z26.b, #0x7
  a4:   and     z25.b, z25.b, #0x7
  a8:   cmtst   v31.16b, v31.16b, v31.16b
  ac:   cmtst   v30.16b, v30.16b, v30.16b
  b0:   cmtst   v29.16b, v29.16b, v29.16b
  b4:   cmtst   v28.16b, v28.16b, v28.16b
  b8:   cmtst   v0.16b, v0.16b, v0.16b
  bc:   cmtst   v23.16b, v23.16b, v23.16b
  c0:   cmtst   v26.16b, v26.16b, v26.16b
  c4:   cmtst   v25.16b, v25.16b, v25.16b
  c8:   and     v31.16b, v31.16b, v24.16b
  cc:   and     v30.16b, v30.16b, v24.16b
  d0:   and     v29.16b, v29.16b, v24.16b
  d4:   and     v28.16b, v28.16b, v24.16b
  d8:   and     v0.16b, v0.16b, v24.16b
  dc:   and     v23.16b, v23.16b, v24.16b
  e0:   and     v26.16b, v26.16b, v24.16b
  e4:   and     v25.16b, v25.16b, v24.16b
  e8:   addp    v30.16b, v31.16b, v30.16b
  ec:   addp    v28.16b, v29.16b, v28.16b
  f0:   addp    v23.16b, v0.16b, v23.16b
  f4:   addp    v25.16b, v26.16b, v25.16b
  f8:   addp    v28.16b, v30.16b, v28.16b
  fc:   addp    v25.16b, v23.16b, v25.16b
 100:   addp    v28.16b, v28.16b, v28.16b
 104:   addp    v25.16b, v25.16b, v25.16b
 108:   str     d28, [x1]
 10c:   str     d25, [x2]
 110:   ret
  → 69 instructions
```

```
════════════════════════════════════════════════════════════════
  sve::classify   (sve_classify_asm)
════════════════════════════════════════════════════════════════
0000000000000000 <sve_classify_asm>:
   0:   adrp    x4, 0 <sve_classify_asm>
   4:   add     x3, x4, #0x0
   8:   ptrue   p7.b, vl16
   c:   ldp     q26, q25, [x0]
  10:   ldp     q22, q30, [x3, #16]
  14:   match   p14.b, p7/z, z26.b, z22.b
  18:   mov     z0.b, p14/z, #-1
  1c:   ldr     q4, [x4]
  20:   match   p15.b, p7/z, z26.b, z4.b
  24:   mov     z3.b, p15/z, #-1
  28:   match   p15.b, p7/z, z25.b, z4.b
  2c:   mov     z2.b, p15/z, #-1
  30:   ldp     q24, q23, [x0, #32]
  34:   match   p15.b, p7/z, z24.b, z4.b
  38:   mov     z1.b, p15/z, #-1
  3c:   match   p15.b, p7/z, z23.b, z4.b
  40:   mov     z27.b, p15/z, #-1
  44:   and     v3.16b, v3.16b, v30.16b
  48:   match   p15.b, p7/z, z25.b, z22.b
  4c:   mov     z29.b, p15/z, #-1
  50:   and     v2.16b, v2.16b, v30.16b
  54:   match   p15.b, p7/z, z24.b, z22.b
  58:   match   p7.b, p7/z, z23.b, z22.b
  5c:   and     v1.16b, v1.16b, v30.16b
  60:   mov     z28.b, p15/z, #-1
  64:   mov     z31.b, p7/z, #-1
  68:   and     v27.16b, v27.16b, v30.16b
  6c:   and     v0.16b, v0.16b, v30.16b
  70:   and     v29.16b, v29.16b, v30.16b
  74:   and     v28.16b, v28.16b, v30.16b
  78:   and     v31.16b, v31.16b, v30.16b
  7c:   addp    v2.16b, v3.16b, v2.16b
  80:   addp    v27.16b, v1.16b, v27.16b
  84:   addp    v29.16b, v0.16b, v29.16b
  88:   addp    v31.16b, v28.16b, v31.16b
  8c:   addp    v27.16b, v2.16b, v27.16b
  90:   addp    v31.16b, v29.16b, v31.16b
  94:   addp    v27.16b, v27.16b, v27.16b
  98:   addp    v31.16b, v31.16b, v31.16b
  9c:   str     d27, [x1]
  a0:   str     d31, [x2]
  a4:   ret
  → 42 instructions
  ```

-----------


# Instructions (clang 20)


$ bash dump_asm.sh
Compiling neon::classify ...
Compiling sve::classify ...

════════════════════════════════════════════════════════════════
  neon::classify  (neon_classify_asm)
════════════════════════════════════════════════════════════════
0000000000000000 <neon_classify_asm>:
   0:	movi	v0.16b, #0xf
   4:	ldp	q1, q2, [x0]
   8:	ldp	q3, q4, [x0, #32]
   c:	adrp	x8, 0 <neon_classify_asm>
  10:	ldr	q5, [x8]
  14:	adrp	x8, 0 <neon_classify_asm>
  18:	ushr	v6.16b, v1.16b, #4
  1c:	ushr	v7.16b, v2.16b, #4
  20:	ushr	v16.16b, v3.16b, #4
  24:	and	v1.16b, v1.16b, v0.16b
  28:	and	v2.16b, v2.16b, v0.16b
  2c:	ushr	v17.16b, v4.16b, #4
  30:	and	v3.16b, v3.16b, v0.16b
  34:	and	v0.16b, v4.16b, v0.16b
  38:	ldr	q4, [x8]
  3c:	tbl	v6.16b, {v5.16b}, v6.16b
  40:	tbl	v7.16b, {v5.16b}, v7.16b
  44:	tbl	v16.16b, {v5.16b}, v16.16b
  48:	adrp	x8, 0 <neon_classify_asm>
  4c:	tbl	v5.16b, {v5.16b}, v17.16b
  50:	tbl	v1.16b, {v4.16b}, v1.16b
  54:	tbl	v2.16b, {v4.16b}, v2.16b
  58:	tbl	v3.16b, {v4.16b}, v3.16b
  5c:	tbl	v0.16b, {v4.16b}, v0.16b
  60:	movi	v4.16b, #0x7
  64:	movi	v17.16b, #0x18
  68:	and	v1.16b, v6.16b, v1.16b
  6c:	and	v2.16b, v7.16b, v2.16b
  70:	and	v3.16b, v16.16b, v3.16b
  74:	and	v0.16b, v5.16b, v0.16b
  78:	and	v5.16b, v1.16b, v4.16b
  7c:	and	v1.16b, v1.16b, v17.16b
  80:	and	v6.16b, v2.16b, v17.16b
  84:	and	v7.16b, v3.16b, v17.16b
  88:	and	v16.16b, v0.16b, v17.16b
  8c:	and	v2.16b, v2.16b, v4.16b
  90:	and	v3.16b, v3.16b, v4.16b
  94:	and	v0.16b, v0.16b, v4.16b
  98:	ldr	q4, [x8]
  9c:	cmeq	v5.16b, v5.16b, #0
  a0:	cmeq	v1.16b, v1.16b, #0
  a4:	cmeq	v6.16b, v6.16b, #0
  a8:	cmeq	v7.16b, v7.16b, #0
  ac:	cmeq	v16.16b, v16.16b, #0
  b0:	cmeq	v2.16b, v2.16b, #0
  b4:	cmeq	v3.16b, v3.16b, #0
  b8:	cmeq	v0.16b, v0.16b, #0
  bc:	bic	v5.16b, v4.16b, v5.16b
  c0:	bic	v1.16b, v4.16b, v1.16b
  c4:	bic	v6.16b, v4.16b, v6.16b
  c8:	bic	v7.16b, v4.16b, v7.16b
  cc:	bic	v16.16b, v4.16b, v16.16b
  d0:	bic	v2.16b, v4.16b, v2.16b
  d4:	bic	v3.16b, v4.16b, v3.16b
  d8:	bic	v0.16b, v4.16b, v0.16b
  dc:	addp	v1.16b, v1.16b, v6.16b
  e0:	addp	v4.16b, v7.16b, v16.16b
  e4:	addp	v2.16b, v5.16b, v2.16b
  e8:	addp	v0.16b, v3.16b, v0.16b
  ec:	addp	v1.16b, v1.16b, v4.16b
  f0:	addp	v0.16b, v2.16b, v0.16b
  f4:	addp	v1.16b, v1.16b, v1.16b
  f8:	addp	v0.16b, v0.16b, v0.16b
  fc:	str	d1, [x1]
 100:	str	d0, [x2]
 104:	ret
  → 66 instructions

════════════════════════════════════════════════════════════════
  sve::classify   (sve_classify_asm)
════════════════════════════════════════════════════════════════
0000000000000000 <sve_classify_asm>:
   0:	ptrue	p1.b
   4:	adrp	x8, 0 <sve_classify_asm>
   8:	add	x8, x8, #0x0
   c:	ptrue	p0.b, vl16
  10:	adrp	x9, 0 <sve_classify_asm>
  14:	add	x9, x9, #0x0
  18:	ld1b	{z0.b}, p1/z, [x8]
  1c:	mov	w8, #0x10                  	// #16
  20:	mov	w10, #0x30                  	// #48
  24:	ld1b	{z2.b}, p0/z, [x0, x8]
  28:	mov	w8, #0x20                  	// #32
  2c:	ld1b	{z1.b}, p0/z, [x0]
  30:	ld1b	{z3.b}, p1/z, [x9]
  34:	ld1b	{z4.b}, p0/z, [x0, x8]
  38:	ld1b	{z5.b}, p0/z, [x0, x10]
  3c:	adrp	x8, 0 <sve_classify_asm>
  40:	match	p1.b, p0/z, z1.b, z0.b
  44:	match	p2.b, p0/z, z2.b, z0.b
  48:	match	p3.b, p0/z, z4.b, z0.b
  4c:	match	p4.b, p0/z, z5.b, z0.b
  50:	ldr	q0, [x8]
  54:	match	p5.b, p0/z, z1.b, z3.b
  58:	match	p6.b, p0/z, z2.b, z3.b
  5c:	match	p7.b, p0/z, z4.b, z3.b
  60:	match	p0.b, p0/z, z5.b, z3.b
  64:	mov	z1.b, p1/z, #-1
  68:	mov	z2.b, p2/z, #-1
  6c:	mov	z3.b, p3/z, #-1
  70:	mov	z4.b, p4/z, #-1
  74:	and	v1.16b, v1.16b, v0.16b
  78:	mov	z5.b, p5/z, #-1
  7c:	and	v2.16b, v2.16b, v0.16b
  80:	mov	z6.b, p6/z, #-1
  84:	and	v3.16b, v3.16b, v0.16b
  88:	mov	z7.b, p7/z, #-1
  8c:	and	v4.16b, v4.16b, v0.16b
  90:	mov	z16.b, p0/z, #-1
  94:	and	v5.16b, v5.16b, v0.16b
  98:	addp	v1.16b, v1.16b, v2.16b
  9c:	and	v6.16b, v6.16b, v0.16b
  a0:	and	v7.16b, v7.16b, v0.16b
  a4:	addp	v2.16b, v3.16b, v4.16b
  a8:	and	v0.16b, v16.16b, v0.16b
  ac:	addp	v3.16b, v5.16b, v6.16b
  b0:	addp	v1.16b, v1.16b, v2.16b
  b4:	addp	v0.16b, v7.16b, v0.16b
  b8:	addp	v1.16b, v1.16b, v1.16b
  bc:	addp	v0.16b, v3.16b, v0.16b
  c0:	str	d1, [x1]
  c4:	addp	v0.16b, v0.16b, v0.16b
  c8:	str	d0, [x2]
  cc:	ret
  → 52 instructions
  ==========



Correctness check passed (16384 blocks).

sve::classify                                      :  0.069 ns/byte  14.45 GB/s   2.77 GHz   0.19 c/byte   0.67 i/byte   3.46 i/c
neon::classify                                     :  0.088 ns/byte  11.41 GB/s   2.77 GHz   0.25 c/byte   0.94 i/byte   3.82 i/c

https://github.com/liuyang-664