#ifndef _LTPF_DECODER_H
#define _LTPF_DECODER_H

#include "../inc/lc3_dec.h"

#if  (ALG_LC3_ENABLE)

#ifdef FIXED_POINT
LC3DEC_Error dec_ltpf_fix(LC3_DEC_STRU* dec, Word32* x, Word32 nbits, Word32* x_ltpf);
#else
LC3DEC_Error dec_ltpf(LC3_DEC_STRU* dec, double* x, Word32 nbits, double* x_ltpf);
#endif

#endif //#if  (ALG_LC3_ENABLE)

#endif
