#ifndef _TNS_DECODER_H
#define _TNS_DECODER_H

#include "../inc/lc3_dec.h"
#if (ALG_LC3_ENABLE)

#ifdef FIXED_POINT
LC3DEC_Error dec_tns_fix(LC3_DEC_STRU* dec, Word32* Xq);
#else
LC3DEC_Error dec_tns(LC3_DEC_STRU* dec, double* Xq);
#endif

#endif //#if (ALG_LC3_ENABLE)

#endif
