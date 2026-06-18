# Committed run log

This file is a captured run of the repo's benchmark and demo programs so the
headline numbers are visible without building. Every block below is verbatim
stdout from running the binary named in its command; nothing here is hand-edited
or invented. Where a run differs from a number quoted in `README.md`, the real
captured number is recorded as-is.

## Machine / toolchain

```
$ uname -a
Darwin MacBook-Pro.local 25.2.0 Darwin Kernel Version 25.2.0: Tue Nov 18 21:09:34 PST 2025; root:xnu-12377.61.12~1/RELEASE_ARM64_T8112 arm64

$ clang --version
Apple clang version 21.0.0 (clang-2100.1.1.101)
Target: arm64-apple-darwin25.2.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

- CPU: Apple M2 (SoC T8112)
- OS: macOS, Darwin 25.2.0
- Compiler: Apple clang 21.0.0
- Run date: 2026-06-21

Every program was built with the README's build line, varying only the source
file and output name:

```
clang -O2 -arch arm64 <dir>/<file>.c -o <file>
```

All 14 source files compiled cleanly (no warnings emitted, no build failures).
No program needs command-line arguments to run; `fuzz/behavior_census.c`
optionally takes an operand count as `argv[1]` (default 1,000,000,000).

## A note on run-to-run variance

The timing/throughput tools were not pinned to a performance core for this run,
so the throughput-bound numbers (NEON SDOT speedup, the sparsity-timing span)
move a few percent between runs from scheduler/thermal noise. Where that matters
the observed range across repeated runs is recorded alongside the single
captured block. The instruction-decode, operand-map, matmul-verification, and
behavioral-census results are deterministic and reproduce exactly.

---

## fuzz/opcode_census.c  --  the 5-bit opcode map

```
$ clang -O2 -arch arm64 fuzz/opcode_census.c -o opcode_census && ./opcode_census
op  status            name
 0  DECODED (ran)     LDX
 1  DECODED (ran)     LDY
 2  DECODED (ran)     STX
 3  DECODED (ran)     STY
 4  DECODED (ran)     LDZ
 5  DECODED (ran)     STZ
 6  DECODED (ran)     LDZI
 7  DECODED (ran)     STZI
 8  DECODED (ran)     EXTRX
 9  DECODED (ran)     EXTRY
10  DECODED (ran)     FMA64
11  DECODED (ran)     FMS64
12  DECODED (ran)     FMA32
13  DECODED (ran)     FMS32
14  DECODED (ran)     MAC16
15  DECODED (ran)     FMA16
16  DECODED (ran)     FMS16
17  SIGILL (invalid)  SET/CLR
18  DECODED (ran)     VECINT
19  DECODED (ran)     VECFP
20  DECODED (ran)     MATINT
21  DECODED (ran)     MATFP
22  DECODED (ran)     GENLUT
23  SIGILL (invalid)  (undocumented?)
24  SIGILL (invalid)  (undocumented?)
25  SIGILL (invalid)  (undocumented?)
26  SIGILL (invalid)  (undocumented?)
27  SIGILL (invalid)  (undocumented?)
28  SIGILL (invalid)  (undocumented?)
29  SIGILL (invalid)  (undocumented?)
30  SIGILL (invalid)  (undocumented?)
31  SIGILL (invalid)  (undocumented?)
```

Real time ~0.28 s. Matches the documented AMX opcode map: opcodes 0-22 decode
and run, 17 (SET/CLR, which needs its specific operand form) and 23-31 trap.

---

## fuzz/operand_hunt.c  --  6M-operand MATINT signedness/footprint hunt

```
$ clang -O2 -arch arm64 fuzz/operand_hunt.c -o operand_hunt && ./operand_hunt
  0M... fully=0
  1M... fully=0
  2M... fully=0
  3M... fully=0
  4M... fully=0
  5M... fully=0

=== 6M operand hunt ===
X-signed observed: 2564   Y-signed observed: 4   FULLY-SIGNED int32: 0

distinct output footprints (nz -> count, example op):
  nz=1    x853      op=0xa606a7f20270000
  nz=2    x3306     op=0x714a68e4e60000
  nz=3    x1628     op=0x60172c5d86e0000
  nz=4    x892      op=0xa008c9931980000
  nz=5    x131      op=0x801314974920000
  nz=6    x906      op=0x221730560080000
  nz=7    x130      op=0xc013aaea0470000
  nz=8    x656      op=0x47116c6e4d90000
  nz=9    x279      op=0x401eb31d90d0000
  nz=10   x576      op=0x110a9c4320000
  nz=11   x119      op=0xc01eef555a10000
  nz=12   x927      op=0x2602c88f8670000
  nz=13   x144      op=0x63124da0b500000
  nz=14   x555      op=0x62152ee31c20000
  nz=15   x229      op=0x27048898d570000
  nz=16   x4086     op=0x6213489db950000
  nz=17   x10       op=0xa716b11e04e0000
  nz=18   x714      op=0xa21537244b40000
  nz=19   x4        op=0xa018f73b8f60000
  nz=20   x455      op=0x63010b39d6a0000
  nz=21   x106      op=0x70454d38b00000
  nz=22   x449      op=0x261ea9254600000
  nz=23   x7        op=0xc018cb774c70000
  nz=24   x835      op=0x63012f888200000
  nz=25   x16       op=0x4014e99d40f0000
```

Real time ~8.9 s. Deterministic (fixed `srandom` seed).

---

## fuzz/behavior_census.c  --  the billion-operand behavioral census

Default run is 1,000,000,000 operands (the headline census). Captured in full
below.

Progress lines and the final census line below are verbatim; the run took
**real 1080.41 s (~18 min)** of wall time on this M2 (user 1023.20 s, sys
54.88 s), and is deterministic (fixed `srandom(0xFACE1234)` seed). It throttles
as the open-addressing behavior table fills, which is why the second half is
slower than the first.

```
$ clang -O2 -arch arm64 fuzz/behavior_census.c -o behavior_census && ./behavior_census
  0M ops, 1 valid, 1 DISTINCT behaviors
  50M ops, 50000001 valid, 91927 DISTINCT behaviors
  100M ops, 100000001 valid, 143438 DISTINCT behaviors
  150M ops, 150000001 valid, 183617 DISTINCT behaviors
  200M ops, 200000001 valid, 216365 DISTINCT behaviors
  250M ops, 250000001 valid, 243525 DISTINCT behaviors
  300M ops, 300000001 valid, 266806 DISTINCT behaviors
  350M ops, 350000001 valid, 286923 DISTINCT behaviors
  400M ops, 400000001 valid, 304349 DISTINCT behaviors
  450M ops, 450000001 valid, 319398 DISTINCT behaviors
  500M ops, 500000001 valid, 332951 DISTINCT behaviors
  550M ops, 550000001 valid, 345204 DISTINCT behaviors
  600M ops, 600000001 valid, 356238 DISTINCT behaviors
  650M ops, 650000001 valid, 365974 DISTINCT behaviors
  700M ops, 700000001 valid, 375072 DISTINCT behaviors
  750M ops, 750000001 valid, 383259 DISTINCT behaviors
  800M ops, 800000001 valid, 390759 DISTINCT behaviors
  850M ops, 850000001 valid, 397711 DISTINCT behaviors
  900M ops, 900000001 valid, 404231 DISTINCT behaviors
  950M ops, 950000001 valid, 410189 DISTINCT behaviors

=== census done: 1000000000 ops, 1000000000 valid, 415834 distinct behaviors ===
sample distinct-behavior operands:
 0x47060aa54080000 0xc20e367c4680000 0x804670e1a280000 0xe6132eb61080000 0xe20eaf5ad880000 0x31333a1d700000
 0xc6068b060be0000 0xa30d88b27480000 0xe61f70d7fd80000 0x43034beb2e80000 0x26168ebcc760000 0x831cea8a6a80000
 0xe21cf5dcc780000 0xa21bcfa90c80000 0x261a854fb380000 0x400848173080000 0xc0022dc95480000 0x401e89cf6780000
 0x6314e02b4580000 0xe01cd4fb1f00000 0x60af6b4cc00000 0xc03353f3b580000 0xe202ad74fa80000 0x63122cbe6190000
 0x80116a798580000 0xa30b32bd1900000 0xe70ab5235f00000 0x6200d798f800000 0xc004e5f92300000 0x2046ad5d8880000
 0xe30032b67380000 0x4368d8e080000 0x630308693a80000 0x271357333800000 0x401e07811670000 0xc6173518a800000
 0xc30ab4bf1e80000 0xc3076d289400000 0xc01c64c98400000 0x231609689e80000 0xc209298d5780000 0xc71fe8a28700000
 0x20acf0dcd00000 0x61cb7799700000 0x1e244e7600000 0x670ab1737500000 0x260129a7a580000 0x66120d3fd500000
 0x230a32c91280000 0x630ba8811900000 0xa00116b0d580000 0x2045f604f280000 0x870fea344500000 0xc30413557f80000
 0x861017d88700000 0xa30bc5744000000 0x860ac42c1b80000 0xa219c4eb0600000 0xa312c7abe980000 0x40000e5dfa80000
```

The headline: **1,000,000,000 MATINT operands fuzzed, all decoded/ran (1e9
valid), 415,834 distinct output behaviors cataloged.** Distinct-behavior count
grows sublinearly (32,406 at 10M; 143,438 at 100M; 415,834 at 1B). For a quick
reproduce, `./behavior_census 10000000` finishes in ~11 s and reports
`10000000 ops, 10000000 valid, 32406 distinct behaviors`.

---

## opmap/genlut_probe.c  --  which register each GENLUT operand bit touches

Output is one line per probed operand; the body is highly repetitive (almost
every probed operand writes register X). Reproduced here are the header, the
first 64 single-value operands (all write X), and the high-bit sweep, which is
where the only X-vs-Y differences appear (bits 24-25 select the Y target).

```
$ clang -O2 -arch arm64 opmap/genlut_probe.c -o genlut_probe && ./genlut_probe
GENLUT operand sweep -- what changes:
  op= 0 : X
  op= 1 : X
   ... (op=2 .. op=63 all report: X) ...
  op=63 : X
  op=(2<<22): X
  op=(1<<23): X
  op=(2<<23): X
  op=(3<<23): X
  op=(1<<24): X
  op=(2<<24): Y
  op=(3<<24): Y
  op=(1<<25): Y
  op=(2<<25): X
  op=(3<<25): Y
  op=(1<<26): X
   ... (op=(2<<26) .. op=(3<<58) all report: X) ...
  op=(3<<58): X
```

Real time ~0.15 s. GENLUT writes X/Y/Z register state with no memory access, as
documented; the X-vs-Y target is selected by operand bits in the 24-25 region.

---

## opmap/mac16_operand_map.c  --  MAC16 outer-product Z layout

```
$ clang -O2 -arch arm64 opmap/mac16_operand_map.c -o mac16_operand_map && ./mac16_operand_map
X[ 0]=1, Y[ 0]=1  ->  Z[row 0][col 0]=1
X[ 1]=1, Y[ 0]=1  ->  Z[row 0][col 1]=1
X[ 2]=1, Y[ 0]=1  ->  Z[row 0][col 2]=1
X[ 0]=1, Y[ 1]=1  ->  Z[row 2][col 0]=1
X[ 0]=1, Y[ 2]=1  ->  Z[row 4][col 0]=1
X[ 3]=1, Y[ 0]=1  ->  Z[row 0][col 3]=1
X[ 0]=1, Y[ 3]=1  ->  Z[row 6][col 0]=1
X[ 5]=1, Y[ 7]=1  ->  Z[row 14][col 5]=1
X[ 7]=1, Y[ 5]=1  ->  Z[row 10][col 7]=1
X[31]=1, Y[31]=1  ->  Z[row 62][col 31]=1
```

Real time ~0.15 s. Confirms the X picks the Z column, Y picks Z row 2*j.

---

## opmap/matint_operand_map.c  --  MATINT accumulate width / wrap behavior

```
$ clang -O2 -arch arm64 opmap/matint_operand_map.c -o matint_operand_map && ./matint_operand_map
SZ=4 K=3:
 10/10 5/5 4/4 8/8
 22/22 11/11 13/13 20/20
 34/34 17/17 22/22 32/32
 46/46 23/23 31/31 44/44
SZ=4 K=4:
 -131071/1! -393211/5! -393212/4! -65538/-2!
 65541/5! 262142/-2! 458738/-14! -131068/4!
 -131070/2! -393211/5! -131069/3! 131068/-4!
 196607/-1! 524272/-16! -1/-1 -524279/9!
SZ=4 K=3:
 -131071/1! -393211/5! -393212/4! -65538/-2!
 4/4 65538/2! 196603/-5! 131070/-2!
 393202/-14! -1/-1 -524281/7! -196603/5!
 -196605/3! -131069/3! 65534/-2! 131070/-2!
```

Real time ~0.13 s. The `value/value!` pairs flag where the hardware result and
the scalar reference diverge; the `!` rows demonstrate the documented int16/
narrow-accumulate wrap behavior, not a defect in the run.

---

## opmap/matmul_int16_verified.c  --  32x32 int16->int32 matmul check

```
$ clang -O2 -arch arm64 opmap/matmul_int16_verified.c -o matmul_int16_verified && ./matmul_int16_verified
32x32 int16 matmul, K=512:  mismatches=0 / 1024   checksum=2360059
sample C[0][0]=111930 (ref 111930)   C[5][7]=24688 (ref 24688)   C[31][31]=-79155 (ref -79155)

BIT-EXACT int16->int32 matmul on AMX. ✓
```

Real time ~0.20 s. **Bit-exact after a mode fix (2026-06-21).** This program
initially failed 1024/1024 with the old mode `(3<<42)`. Root cause was *not* the
Z-readout layout (an earlier guess): a distinct-prime single-MATINT probe
re-confirmed the layout `C[i][j] = z[2*i+(j&1)][j/2]` is correct. The real bug
was **signedness**: the MATINT mode was missing the two per-operand sign bits,
so signed int16 inputs were read as unsigned (every negative value off by
+65536, polluting the high 16 bits of each int32 lane; the low 16 bits were
always correct). Recovered the bit-exact signed mode
`(3<<42)|(1<<26)|(1<<63)` (int32 accumulate · bit 26 = X-signed · bit 63 =
Y-signed) by fuzzing the mode field against a negative-input known-answer case:
no single added bit works; the pair (26, 63) does. This signed-mode recovery is
itself a small original result beyond the corsix map.

---

## opmap/matmul_int16_signed_verified.c  --  signed-variant matmul check

```
$ clang -O2 -arch arm64 opmap/matmul_int16_signed_verified.c -o matmul_int16_signed_verified && ./matmul_int16_signed_verified
signed 32x32 int16->int32 matmul K=512: 0/1024 mismatch  BIT-EXACT
  layout: row=2i+(j&1)? rbase[1]=2 roff[1]=1 col[1]=0  (i.e. C[i][j]=z[2*i+roff_j][col_j])
```

Real time ~0.13 s. Bit-exact after the same mode fix: this file was missing the
X-signed bit (it had `(3<<42)|(1<<63)`, i.e. Y-signed only). With both sign bits
`(3<<42)|(1<<26)|(1<<63)` it auto-recovers the layout via one-hot probing and
verifies all 1024 elements. Deterministic.

---

## int8/amx_int8_constrained_matmul.c  --  AMX int8 outer-product (16-bit accumulate)

```
$ clang -O2 -arch arm64 int8/amx_int8_constrained_matmul.c -o amx_int8_constrained_matmul && ./amx_int8_constrained_matmul
int8->int32 16x16 K=256 matmul via bit-50: 256/256 mismatch  checksum=8166693  FAIL
  C[0][0]=47656/2472488  C[9][13]=2538/2427370  C[15][15]=40712/2531080
```

Real time ~0.12 s. This `FAIL` is the intended demonstration, not a regression:
the clean int8 outer product accumulates into 16 bits and overflows after a few
K terms (here K=256), so it cannot match a 32-bit scalar reference. This is the
README's "int8 outer-product overflows after a few terms" point shown directly.

---

## int8/why_amx_int8_fails.c  --  search for a true int8 2D outer-product mode

```
$ clang -O2 -arch arm64 int8/why_amx_int8_fails.c -o why_amx_int8_fails && ./why_amx_int8_fails
  0M int8-2D hits=0
  1M int8-2D hits=214
  2M int8-2D hits=440
  3M int8-2D hits=684
int8 2D-outer-product hits: 898
  example op=0x2718ea772f80000
```

Real time ~27 s. Deterministic.

---

## int8/neon_sdot_4x_bench.c  --  the headline NEON SDOT vs fp32 speedup

```
$ clang -O2 -arch arm64 int8/neon_sdot_4x_bench.c -o neon_sdot_4x_bench && ./neon_sdot_4x_bench
GEMV 4096x4096 x30 reps:
  fp32 NEON : 0.043s  23.5 GFLOP/s  47.0 GB/s
  int8 SDOT : 0.011s  90.7 GOP/s    45.4 GB/s
  SPEEDUP: 3.86x   (quant rel-error 1.27%)
```

Real time ~0.31 s. The README quotes 4.04x at ~1.3% error. The **quantization
error reproduces exactly at 1.27%** (deterministic). The **speedup is
throughput-noisy without perf-core pinning**: across 10 back-to-back runs on
this machine it ranged ~3.62x to ~4.28x, clustering around 3.9-4.0x, so the
README's 4.04x is within the observed range. Captured single run above shows
3.86x.

---

## consttime/fma32_value_timing.c  --  FMA32 value-dependence (denormal slow-path test)

```
$ clang -O2 -arch arm64 consttime/fma32_value_timing.c -o fma32_value_timing && ./fma32_value_timing
AMX FMA32 value-dependent timing (min ns/op, back-to-back accumulate):
  normal 1.0             : 1.3346 ns  (+0.0% vs normal)
  normal small 1e-10     : 1.3327 ns  (-0.1% vs normal)
  DENORMAL 1e-40         : 1.3346 ns  (+0.0% vs normal)
  DENORMAL x normal      : 1.3377 ns  (+0.2% vs normal)
  zero                   : 1.3380 ns  (+0.3% vs normal)
  large 1e30             : 1.3349 ns  (+0.0% vs normal)
  inf (1e30*1e30)        : 1.3351 ns  (+0.0% vs normal)
  tiny normal FLT_MIN    : 1.3364 ns  (+0.1% vs normal)

=> any regime >> normal = VALUE-DEPENDENT timing leak (esp. denormals)
```

Real time ~1.0 s. All regimes within +-0.3% of the normal case, including
denormals and zero. No value-dependent timing leak, matching the README's
constant-time finding.

---

## consttime/genlut_value_timing.c  --  GENLUT data-dependence

```
$ clang -O2 -arch arm64 consttime/genlut_value_timing.c -o genlut_value_timing && ./genlut_value_timing
GENLUT data-dependent timing (min ns/iter of LDX+GENLUT):
  all-0x00         : 3.4517 ns
  all-0xFF         : 3.4492 ns
  all-0x3F         : 3.4443 ns
  ramp 0..63       : 3.4451 ns
  random           : 3.4463 ns
  one-hot lane0    : 3.4445 ns
  half 0/half FF   : 3.4447 ns
  0x01 (idx 1)     : 3.4450 ns
spread: 0.0074 ns (0.22%)  flat = constant-time, no leak
```

Real time ~2.6 s. 0.22% spread across table contents: GENLUT is constant-time,
matching the README's claim that it adds a GENLUT timing check with no leak.

---

## consttime/sparsity_timing.c  --  can MATINT timing leak operand sparsity?

```
$ clang -O2 -arch arm64 consttime/sparsity_timing.c -o sparsity_timing && ./sparsity_timing
zeros_in_X  ns/op
   0/16    0.3876
   1/16    0.3897
   2/16    0.3888
   3/16    0.3876
   4/16    0.3915
   5/16    0.3919
   6/16    0.3898
   7/16    0.3862
   8/16    0.3910
   9/16    0.3898
  10/16    0.3919
  11/16    0.3889
  12/16    0.3871
  13/16    0.3915
  14/16    0.3918
  15/16    0.3887
  16/16    0.3903

dense(0 zeros)=0.3876  full-sparse(16 zeros)=0.3903  span=-0.7%
slope 0.00008 ns per zero-lane.  ==> weak/non-monotonic; need power.
```

Real time ~0.7 s. Consistent with the README's constant-time conclusion: no
monotonic dependence of timing on operand sparsity (span -0.7%, near-zero
slope). NOTE: a single cold first run in this session once showed a ~33% span
with a "we can READ activation sparsity off the clock" message; every warm
repeat (4x) reported a flat span near 0% as above. The 33% reading was a
cold-start/scheduling artifact, not a real sparsity leak.
