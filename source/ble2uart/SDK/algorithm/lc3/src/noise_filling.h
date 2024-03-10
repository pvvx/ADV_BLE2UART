#ifndef _NOISE_FILLING_H
#define _NOISE_FILLING_H

#include "../inc/lc3_dec.h"

#if (ALG_LC3_ENABLE)

#ifdef FIXED_POINT
#define  NOISEFILLING_Q    (15)
LC3DEC_Error dec_noisefilling_fix(LC3_DEC_STRU* dec, Word32* Xq);

#else
LC3DEC_Error dec_noisefilling(LC3_DEC_STRU* dec, double* Xq);
#endif

#endif //#if (ALG_LC3_ENABLE)

#endif
