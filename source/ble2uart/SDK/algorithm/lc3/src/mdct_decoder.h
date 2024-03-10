#ifndef _MDCT_DECODER_H
#define _MDCT_DECODER_H

#include "../inc/lc3_dec.h"

#if (ALG_LC3_ENABLE)

#ifdef FIXED_POINT
LC3DEC_Error dec_imdct_fix(LC3_DEC_STRU* dec, Word32* in, Word32* out, Word16* exp);
#else
LC3DEC_Error dec_imdct(LC3_DEC_STRU* dec, double* in, double* out);
#endif

#endif

#endif
