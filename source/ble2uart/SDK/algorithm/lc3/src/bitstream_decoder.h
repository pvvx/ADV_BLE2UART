#ifndef _BITSTREAM_DECODER_H
#define _BITSTREAM_DECODER_H

#include "../inc/lc3_dec.h"

#if  (ALG_LC3_ENABLE)

LC3DEC_Error dec_bitstream(LC3_DEC_STRU* dec, UWord8* inbuf, UWord32 inlen);

#endif

#endif
