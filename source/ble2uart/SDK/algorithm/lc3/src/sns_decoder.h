#ifndef _SNS_DECODER_H
#define _SNS_DECODER_H

#include "../inc/lc3_dec.h"
#if (ALG_LC3_ENABLE)

#ifdef FIXED_POINT
LC3DEC_Error dec_sns_fix(LC3_DEC_STRU* dec, Word32* X_s, Word32* X);
#else
LC3DEC_Error dec_sns(LC3_DEC_STRU* dec, double* X_s, double* X);
#endif

#endif //#if (ALG_LC3_ENABLE)

#endif
