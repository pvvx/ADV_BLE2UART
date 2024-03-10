#ifndef _COMMON_FUNC_H
#define _COMMON_FUNC_H

#include "../inc/lc3_dec.h"

#if  (ALG_LC3_ENABLE)

#ifdef ANDES_INTRINSIC
#include "nds_basic_math.h"
#include "nds_utils_math.h"
#include "nds_filtering_math.h"
#include "nds_statistics_math.h"
#endif
#ifdef FIXED_POINT
void idct_fix(const Word16* in, Word16* out);
void dct_fix(const Word16* in, Word16* out);
Word16 pow2_16(Word16 x, Word16* y_e);
Word32 Isqrt(Word32  x,  /* mantissa */
    Word16* x_e /* pointer to exponent */
);
#else
void dct(
    double* x,	// i: input
    double* y	// o: dct output
);

void idct(
    double* x,	// i:input
    double* y	// o:idct output
);

#endif

void duplicate_q15(Word16* src, Word16* dst, Word32 size);
void duplicate_q31(Word32* src, Word32* dst, Word32 size);
void shift_q15(Word16       x[], /* i/o: signal to scale                 Qx        */
    const Word16 lg,  /* i  : size of x[]                     Q0        */
    const Word16 exp0 /* i  : exponent: x = round(x << exp)   Qx ?exp  */
);
void set_q31(Word32 val, Word32* dst, Word32 size);

#endif //#if  (ALG_LC3_ENABLE)

#endif
