#ifndef BASIC_OP_H
#define BASIC_OP_H

#include "../inc/lc3_types.h"
#include "constant.h"

#if  (ALG_LC3_ENABLE)

/* short logical shift */
//Word16 lshr(Word16 var1, Word16 var2);
/* int shift*/
//Word32 L_shl(Word32 L_var1, Word16 var2);
//Word32 L_shr(Word32 L_var1, Word16 var2);
/* short shift*/
Word16 shr(Word16 var1, Word16 var2);
Word16 shl(Word16 var1, Word16 var2);
/* short shift and rounding*/
Word16 shr_r(Word16 var1, Word16 var2);

/* if x==0 return 0 */
Word16 getScaleFactor16(Word16* x, Word16 len);
/* if x==0 return 16 */
Word16 getScaleFactor16_0(Word16* x, Word16 len);
Word16 getScaleWord32(Word32* x, Word16 len);
/* if x==0 return 32 */
Word16 getScaleFactor32_0(Word32* x, Word16 len);
Word32 Norm32Norm(const Word32* x, Word16 shift, const Word16 length, Word16* result_e);

/* log2 */
inline Word16 BASOP_Util_Log2_16(Word32 x, Word16 x_e)
{
	Word16 shift, tmp1, tmp2;
	Word16 outInt, outFrac, out;

	if (x == 0)
	{
		return (MIN_16);
	}

	/* Scale Input */
	shift = clrs32(x);

	x = L_shl(x, (shift - 10));

	/* Integer part */
	outInt = ((x_e - shift - 1) << 9);

	/* Fractional part */

	tmp1 = mac_r(x, -33, 16384);
	tmp2 = lshr((Word16)(x) , 6);
	outFrac = mac_r(Log2_16_table1[tmp1], Log2_16_table2[tmp1], tmp2);

	/* Output */
	out = (outInt + outFrac);

	return out;
}

inline Word32 BASOP_Util_Log2(Word32 x)
{
	Word32 exp;
	Word16 exp_e;
	Word16 nIn;
	Word16 accuSqr;
	Word32 accuRes;

	/* x never == 0
	if (x == 0)
	{
		return ((Word32)MIN_32);
	}
	 */
	/* normalize input, calculate integer part */
	exp_e = clrs32(x);
	x = L_shl(x, exp_e);
	exp =(Word32) (exp_e);

	/* calculate (1-normalized_input) */
	nIn = ((MAX_32 - x))>>16;

	/* approximate ln() for fractional part (nIn *c0 + nIn^2*c1 + nIn^3*c2 + ... + nIn^8 *c7) */

	/* iteration 1, no need for accumulation */
	accuRes = L_mult(nIn, ldCoeff[0]); /* nIn^i * coeff[0] */
	accuSqr = mult_16(nIn, nIn);          /* nIn^2, nIn^3 .... */

	/* iteration 2 */
	accuRes = L_mac(accuRes, accuSqr, ldCoeff[1]); /* nIn^i * coeff[1] */
	accuSqr = mult_16(accuSqr, nIn);                  /* nIn^2, nIn^3 .... */

	/* iteration 3 */
	accuRes = L_mac(accuRes, accuSqr, ldCoeff[2]); /* nIn^i * coeff[2] */
	accuSqr = mult_16(accuSqr, nIn);                  /* nIn^2, nIn^3 .... */

	/* iteration 4 */
	accuRes = L_mac(accuRes, accuSqr, ldCoeff[3]); /* nIn^i * coeff[3] */
	accuSqr = mult_16(accuSqr, nIn);                  /* nIn^2, nIn^3 .... */

	/* iteration 5 */
	accuRes = L_mac(accuRes, accuSqr, ldCoeff[4]); /* nIn^i * coeff[4] */
	accuSqr = mult_16(accuSqr, nIn);                  /* nIn^2, nIn^3 .... */

	/* iteration 6 */
	accuRes = L_mac(accuRes, accuSqr, ldCoeff[5]); /* nIn^i * coeff[5] */
	accuSqr = mult_16(accuSqr, nIn);                  /* nIn^2, nIn^3 .... */

	/* iteration 7, no need to calculate accuSqr any more */
	accuRes = L_mac(accuRes, accuSqr, ldCoeff[6]); /* nIn^i * coeff[6] */

	/* ld(fractional part) = ln(fractional part)/ln(2), 1/ln(2) = (1 + 0.44269504) */
	accuRes = (L_shr(accuRes, 1) + mult_16_16_32( (accuRes)>>16, 14506));

	accuRes = L_shr(accuRes, 6 - 1); /* fractional part/LD_DATA_SCALE =6 */
	exp = L_shl(exp, (31 - 6));  /* integer part/LD_DATA_SCALE */
	accuRes = (accuRes - exp);               /* result = integer part + fractional part */

	return (accuRes);
}

Word32 BASOP_Util_InvLog2(Word32 x);
Word16 Inv16(Word16  mantissa, Word16* exponent);
Word16 BASOP_Util_Divide3216_Scale(Word32  x,Word16  y,Word16* s);
Word16 BASOP_Util_Divide1616_Scale(Word16 x, Word16 y, Word16* s);

void Mpy_32_16_ss(Word32 L_var1, Word16 var2, Word32* L_varout_h, UWord16* varout_l);

Word32 Mpy_32_16(Word32 x, Word16 y);
Word16 div_s(Word16 var1, Word16 var2);

#ifdef USE_SPREAD_FFT


void Mpy_32_32_ss(Word32 L_var1, Word32 L_var2, Word32* L_varout_h, UWord32* L_varout_l);
Word32 Mpy_32_32(Word32 x, Word32 y);

#define SHC(x) ((Word16)x)
#define shr_pos shr
#define shl_pos shl
#define L_shr_pos L_shr
#define L_shl_pos L_shl
#define SCALEFACTORN2 (3)
#define SCALEFACTOR5 (4)
#define SCALEFACTOR6 (4)
#define SCALEFACTOR8 (4)
#define SCALEFACTOR10 (5)
#define SCALEFACTOR15 (5)
#define SCALEFACTOR30_1 (5)
#define SCALEFACTOR30_2 (1)
#define Mpy_32_xx mult_32_Q15_32
#define SCALEFACTOR20 (5)

#define WORD32_BITS 32
#define WORD16_BITS 16

#define WORD322WORD16(val)                                                                                             \
    ((((((val) >> (WORD32_BITS - WORD16_BITS - 1)) + 1) > (((long)1 << WORD16_BITS) - 1)) && ((long)(val) > 0))      \
         ? (Word16)(short)(((long)1 << (WORD16_BITS - 1)) - 1)                                                       \
         : (Word16)(short)((((val) >> (WORD32_BITS - WORD16_BITS - 1)) + 1) >> 1))

#define FFTC(x) WORD322WORD16((Word32)x)

#define C31 (FFTC(0x91261468)) /* FL2WORD32( -0.86602540) -sqrt(3)/2 */
#define C31_32 0x91261468
#define C51 (FFTC(0x79bc3854)) /* FL2WORD32( 0.95105652)   */
#define C52 (FFTC(0x9d839db0)) /* FL2WORD32(-1.53884180/2) */
#define C53 (FFTC(0xd18053ce)) /* FL2WORD32(-0.36327126)   */
#define C54 (FFTC(0x478dde64)) /* FL2WORD32( 0.55901699)   */
#define C55 (FFTC(0xb0000001)) /* FL2WORD32(-1.25/2)       */
#define C81 (FFTC(0x5a82799a)) /* FL2WORD32( 7.071067811865475e-1) */
#define C82 (FFTC(0xa57d8666)) /* FL2WORD32(-7.071067811865475e-1) */
#define C51_32 (0x79bc3854) /* FL2WORD32( 0.95105652)   */
#define C52_32 (0x9d839db0) /* FL2WORD32(-1.53884180/2) */
#define C53_32 (0xd18053ce) /* FL2WORD32(-0.36327126)   */
#define C54_32 (0x478dde64) /* FL2WORD32( 0.55901699)   */
#define C55_32 (0xb0000001) /* FL2WORD32(-1.25/2)       */
#define C81_32 (0x5a82799a) /* FL2WORD32( 7.071067811865475e-1) */
#define C82_32 (0xa57d8666) /* FL2WORD32(-7.071067811865475e-1) */

#define cplxMpy4_8_0(re, im, a, b, c, d)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        re = L_shr(L_sub(Mpy_32_xx(a, c), Mpy_32_xx(b, d)), 1);                                                    \
        im = L_shr(L_add(Mpy_32_xx(a, d), Mpy_32_xx(b, c)), 1);                                                    \
    } while (0)


#define cplxMpy4_8_1(re, im, a, b)                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        re = L_shr(a, 1);                                                                                          \
        im = L_shr(b, 1);                                                                                          \
    } while (0)


#define cplxMpy4_8_2(re, im, a, b, c, d)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        re = L_shr(L_add(Mpy_32_32(a, c), Mpy_32_32(b, d)), 1);                                                    \
        im = L_shr(L_sub(Mpy_32_32(b, c), Mpy_32_32(a, d)), 1);                                                    \
    } while (0)


#define cplxMpy4_12_0(re, im, a, b, c, d)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        re = L_sub(Mpy_32_xx(a, c), Mpy_32_xx(b, d));                                                                  \
        im = L_add(Mpy_32_xx(a, d), Mpy_32_xx(b, c));                                                                  \
    } while (0)

#define cplxMpy4_12_1(re, im, a, b)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        re = a;                                                                                                        \
        im = b;                                                                                                        \
    } while (0)
#endif

#endif //#if  (ALG_LC3_ENABLE)

#endif // !BASIC_OP_H

#pragma once
