#ifndef SPREAD_FFT
#define SPREAD_FFT

#include "../../inc/lc3_types.h"
#if (ALG_LC3_ENABLE)
void BASOP_cfft(Word32* re, Word32* im, Word16 sizeOfFft, Word16 s, Word16* scale, Word32* x);
#endif //#if (ALG_LC3_ENABLE)
#endif
