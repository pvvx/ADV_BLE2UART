#ifndef _LC3_DEC_H
#define _LC3_DEC_H

#include "lc3_types.h"
#include "lc3_common.h"
#include "config.h"

#if (ALG_LC3_ENABLE)

#include "../src/fft_lib/_kiss_fft_guts.h"  // FIXME
#include "../src/fft_lib/kiss_fft_custom/kiss_fft_custom.h"
/*! LC3 decoder error codes. */
typedef enum
{
    LC3DEC_OK = 0,
    LC3DEC_ERROR_GETPARA = 1,
    LC3DEC_ERROR_INIT,
    LC3DEC_ERROR_SHAPE,
    LC3DEC_ERROR_SETPARA,
    LC3DEC_ERROR_BITSTREAM,
    LC3DEC_ERROR_ARITHMATIC,
} LC3DEC_Error;

typedef struct LC3_DEC
{
	Word32     nSamplerate;
	Word32     nBitrate;
    Word32     nChannels;
    Word16     NF;
    Word16     NE;
    Word16     Nb;
    Word16     Nms;                // 20 ->10ms,15->7.5ms
    Word16     Nms_mode;           // 0->10ms,1->7.5ms
    Word16     Z;
    /* Bandwidth detector */
    Word16     nBitsBw;
    Word16     nFsIdx;
    /* side infor */
    Word16     nPbw;
    Word16     lastnz;
    Word16     lsbMode;
    Word16     gg_ind;             // used in global_gain
    Word16     num_tns_filters;    // used in tns decoder
    Word16     rc_order[2];        // tns
    Word16     rc_i[8][2];         // tns
    Word16     ind_LF;             // sns
    Word16     ind_HF;             // sns
    Word16     shape_j;            // sns
    Word16     gain_i;             // sns
    Word32     idxA;               // sns
    Word32     idxB;               // sns
    Word16     LS_indA;            // sns
    Word16     LS_indB;            // sns
    Word16     pitch_present;      // ltpf
    Word16     ltpf_active;        // ltpf
    Word16     pitch_index;        // ltpf
    Word16     p_int;              // ltpf
    Word16     p_fr;               // ltpf
    Word16     mem_ltpf_active;    // ltpf
    Word16     mem_p_int;          // ltpf
    Word16     mem_p_fr;           // ltpf
    Word16     F_NF;
    /* bitstream pointer */
    Word16     bp_side;
    Word16     mask_side;
    /* noise filling */
    Word16     zeroFrameFlag;
    Word32     nf_seed;
    /* imdct */
   // kiss_fft_cfg kiss_fft_state;
    kiss_fft_state16_ptr kiss_fft_state;
    Word16*    mdct_twiddle;
    Word16*    mdct_sin_twiddle;
    Word16	   sin_step;
    const Word16*    W_MDCT;
    Word16     W_size;
    /* plc */
    Word32     plc_seed;
#ifdef FIXED_POINT
    Word32*    mem_imdct;
    Word32*    mem_plc_buf;
    Word32*    mem_x;
    Word32*    mem_x_ltpf;
    Word16*    mem_c_num;
    Word16*    mem_c_den;
#else
    double*    mem_imdct;
    double*    mem_plc_buf;
    double*    mem_x;
    double*    mem_x_ltpf;
    double*    mem_c_num;
    double*    mem_c_den;
#endif
    /* plc */
    Word16     bec_detect;
} LC3_DEC_STRU;

LC3DEC_Error lc3dec_init(LC3_DEC_STRU* dec, Word32 samplerate, Word32 bitrate, Word16 channels, Word16 Nms_mode);
LC3DEC_Error lc3dec_decode_frame(LC3_DEC_STRU* dec, UWord8* inbuf, UWord32 inlen, Word16* outbuf);
LC3DEC_Error lc3dec_get_parameter(LC3_DEC_STRU* dec, LC3_PARAMETER para, Word32* val);
LC3DEC_Error lc3dec_set_parameter(LC3_DEC_STRU* dec, LC3_PARAMETER para, Word32* val);
LC3DEC_Error lc3dec_free(LC3_DEC_STRU* dec);


#endif

#endif
