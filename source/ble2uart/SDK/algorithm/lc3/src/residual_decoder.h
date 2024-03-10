#ifndef _RESIDUAL_DECODER_H
#define _RESIDUAL_DECODER_H

#include "../inc/lc3_dec.h"

#if (ALG_LC3_ENABLE)

#define  RESIDUAL_Q    (15)
#define  RESIDUAL_A    (3 << (RESIDUAL_Q - 4))   // 0.1875
#define  RESIDUAL_B    (5 << (RESIDUAL_Q - 4))   // 0.3125

LC3DEC_Error dec_residual_fix(Word16* X_q, Word16 N_E, Word16* resBits, Word16* nResBits, Word32* X_q_residual);

LC3DEC_Error dec_residual(Word16* X_q, Word16 N_E, Word16* resBits, Word16* nResBits, double* X_q_residual);

#endif //#if (ALG_LC3_ENABLE)

#endif
