#ifndef _GLOBAL_GAIN_H
#define _GLOBAL_GAIN_H

#include "../inc/lc3_dec.h"

#if  (ALG_LC3_ENABLE)
#ifdef FIXED_POINT
void global_gain_fix(LC3_DEC_STRU* dec, Word32* Xq_residual, Word16 nBits, Word16* exp);
#else
void global_gain(LC3_DEC_STRU* dec, double* Xq_residual, Word16 nBits);
#endif
#endif

#endif
