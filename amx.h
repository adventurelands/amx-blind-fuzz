// amx.h - hand-emitted encodings for Apple's undocumented AMX matrix coprocessor.
// No compiler emits these; we splat raw 32-bit instruction words into the stream.
// Encoding reverse-engineered by Peter Cawley (corsix/amx). M1/M2-class AMX.
#ifndef AMX_H
#define AMX_H
#include <stdint.h>

// Op goes in bits [9:5]; a GPR index goes in bits [4:0]. The "0%1" trick: clang
// substitutes %1 with a register name like "x5" -> "0x5" (=5). For x16..x30 the
// "0x10".."0x1e" reading is 6 too high per nibble, so we subtract 6*(val>>4).
#define AMX_OP_GPR(op, gpr) \
    __asm__ volatile(".word (0x201000 + (%0 << 5) + 0%1 - ((0%1 >> 4) * 6))" \
        :: "i"(op), "r"((uint64_t)(gpr)) : "memory")

// Immediate-form ops (SET/CLR). The 3 nops give the coprocessor time to settle.
#define AMX_OP_IMM5(op, imm5) \
    __asm__ volatile("nop \n nop \n nop \n .word (0x201000 + (%0 << 5) + %1)" \
        :: "i"(op), "i"(imm5) : "memory")

#define AMX_LDX(gpr)    AMX_OP_GPR( 0, gpr)
#define AMX_LDY(gpr)    AMX_OP_GPR( 1, gpr)
#define AMX_STX(gpr)    AMX_OP_GPR( 2, gpr)
#define AMX_STY(gpr)    AMX_OP_GPR( 3, gpr)
#define AMX_LDZ(gpr)    AMX_OP_GPR( 4, gpr)
#define AMX_STZ(gpr)    AMX_OP_GPR( 5, gpr)
#define AMX_LDZI(gpr)   AMX_OP_GPR( 6, gpr)
#define AMX_STZI(gpr)   AMX_OP_GPR( 7, gpr)
#define AMX_EXTRX(gpr)  AMX_OP_GPR( 8, gpr)
#define AMX_EXTRY(gpr)  AMX_OP_GPR( 9, gpr)
#define AMX_FMA64(gpr)  AMX_OP_GPR(10, gpr)
#define AMX_FMS64(gpr)  AMX_OP_GPR(11, gpr)
#define AMX_FMA32(gpr)  AMX_OP_GPR(12, gpr)
#define AMX_FMS32(gpr)  AMX_OP_GPR(13, gpr)
#define AMX_MAC16(gpr)  AMX_OP_GPR(14, gpr)
#define AMX_FMA16(gpr)  AMX_OP_GPR(15, gpr)
#define AMX_FMS16(gpr)  AMX_OP_GPR(16, gpr)
#define AMX_SET()       AMX_OP_IMM5(17, 0)
#define AMX_CLR()       AMX_OP_IMM5(17, 1)
#define AMX_VECINT(gpr) AMX_OP_GPR(18, gpr)
#define AMX_VECFP(gpr)  AMX_OP_GPR(19, gpr)
#define AMX_MATINT(gpr) AMX_OP_GPR(20, gpr)
#define AMX_MATFP(gpr)  AMX_OP_GPR(21, gpr)
#define AMX_GENLUT(gpr) AMX_OP_GPR(22, gpr)

#endif
