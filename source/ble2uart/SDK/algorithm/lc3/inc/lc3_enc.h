#ifndef _LC3_ENC_H
#define _LC3_ENC_H

#include "lc3_types.h"
#include "lc3_common.h"
#include "config.h"

#if (ALG_LC3_ENABLE)

#include "../src/fft_lib/_kiss_fft_guts.h"
#include "../src/fft_lib/kiss_fft_custom/kiss_fft_custom.h"
#ifdef ANDES_INTRINSIC
#include "nds_filtering_math.h"
#endif // DEBUG


//#define EN_SPEC_INTERMEDIATE 
/*! LC3 encoder error codes. */
typedef enum
{
    LC3ENC_OK = 0,
    LC3ENC_ERROR_INIT = 1,
	LC3ENC_ERROR_GETPARA,
	LC3ENC_ERROR_SETPARA
} LC3ENC_Error;


typedef struct LC3_ENC
{
#ifndef FIXED_POINT
	// Spectral quantization
	double nbits_offset_old;
	Word32 nbits_spec_old;
	Word32 nbits_est_old;
	Word32 reset_offset_old;
	// LTPF
	double stage1;
	double stage2;
	Word32 len_12k8;
	Word32 D_LTPF;
	double* x_12k8_HP_last; // input data of last frame
	double* x_12k8_HP_D_last; //
	double* x_i_fr_last;
	double* x_6k4_last;
	Word16 T_prev; // pirch_lag estimated in previous frame
	Word16 mem_ltpf_active;
	double mem_nc;
	double mem_mem_nc;
	Word16 mem_pitch;
	// att
	double A_att_last;
	double E_att_last;
	Word32 P_att_last;
	double* x_att_last;
	// BW
	Word16 nbits_bw;
	// mdct
	double* W_MDCT;
	Word16 W_size;
	Word16 Nb; // number of bands
#else

	/* mdct */
	//kiss_fft_cfg kiss_fft_state;
	kiss_fft_state16_ptr kiss_fft_state;
	const Word16* mdct_twiddle;
	const Word16* mdct_sin_twiddle;
	Word16	sin_step;
	const Word16* W_MDCT;
	Word16 W_size;
	Word16 Nb; // number of bands

	/* bw detector */
	Word16 nbits_bw;

	/* attack detector */
	Word16 M_F;
	Word16 m_stop;
	Word16 att_scaling;
	Word16 x_att_last[2];
	Word32 A_att_last;
	Word32 E_att_last;
	Word32 P_att_last;
	/* tns */
	Word32 nn_idx;
	/* ltpf */
#if 0
	nds_bq_df1_q15_t ltpf_highpass_q15;
#endif // ANDES_INTRINSIC
	Word16 ltpf_enable;
	Word32 len_12k8;
	Word32 D_LTPF;
	Word32 stage1;
	Word32 stage2;
	Word16 x_12k8_last[128]; // x_12k8 of last frame
	Word16 x_12k8_D_buf[232+130]; //x_12k8_D_buf[232] is x_12k8_D[0]
	Word16 x_12k8_D_last_exp;
	Word16 x_12k8_D_last_3[3];
	Word16 x_6k4_last[114+64];
	Word16 x_6k4_last_exp;
	Word32 T_prev; // pirch_lag estimated in previous frame
	Word16 mem_ltpf_active;
	Word16 mem_nc;
	Word16 mem_mem_nc;
	Word16 mem_pitch;

	/* Spectral quantization */
	Word32 nbits_offset_old;
	Word16 nbits_spec_old;
	Word16 nbits_est_old;
	Word32 reset_offset_old;
	Word16 gg_off;
	Word16 nbits_ari;
#endif // !FIXED_POINT
	/* overall */
	Word32 frame_counter;
	Word32 fs; // sample rate
	Word16 fs_ind; // index of fs
	Word16 channelNum; //channal number
	Word32 bitrate;      // bit rate
	Word16 NF; // sample number of one frame
	Word16 NE; // frequency number of one frame
	Word16 Nms_mode;// 0->10ms,1->7.5ms
	Word16 Nms;     // 20 ->10ms,15->7.5ms
	Word16 nbytes; // bytes of one frame
	Word32 nbits; // bits of one frame
	Word16 input_last[LC3_MAX_NF]; // input data of last frame

} LC3_ENC_STRU;

LC3ENC_Error lc3enc_init(LC3_ENC_STRU* enc, Word32 nSamplerate, Word16 nChannels, Word32 nBitrate,int nMs_mode);
LC3ENC_Error lc3enc_encode_frame(LC3_ENC_STRU* enc, Word16* input, UWord8* output);
LC3ENC_Error lc3enc_get_parameter(LC3_ENC_STRU* enc, LC3_PARAMETER para, Word32* val);
LC3ENC_Error lc3enc_set_parameter(LC3_ENC_STRU* enc, LC3_PARAMETER para, Word32* val);
LC3ENC_Error lc3enc_change_bitrate(LC3_ENC_STRU* enc, Word32 bitrate);
LC3ENC_Error lc3enc_free(LC3_ENC_STRU* enc);


#endif

#endif
