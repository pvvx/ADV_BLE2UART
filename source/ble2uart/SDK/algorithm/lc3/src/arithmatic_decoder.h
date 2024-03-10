#ifndef _ARITHMATIC_DECODER_H
#define _ARITHMATIC_DECODER_H

#include "../inc/lc3_dec.h"
#include "constant.h"

#if (ALG_LC3_ENABLE)


typedef struct {
    Word32 low;
    Word32 range;
} ST_STRU;

LC3DEC_Error dec_arithmatic(LC3_DEC_STRU* dec, UWord8* inbuf, UWord32 inlen, 
                            Word16* X_q, Word16* resBits, Word16* pBitsResidual);


#endif //#if (ALG_LC3_ENABLE)

#endif
