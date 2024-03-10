#ifndef _ENCODER_FUNCTION_H
#define _ENCODER_FUNCIION_H
#include"../inc/lc3_enc.h"
#if  (ALG_LC3_ENABLE)
#include<stdlib.h>
//#include<stdio.h>
#include "fft_lib/_kiss_fft_guts.h"
#include"constant.h"
#include"../inc/config.h"
#include"intrinsic.h"
#include"dump.h"
#include"basic_op.h"
#include"common_func.h"
#ifndef FIXED_POINT

void ld_mdct(LC3_ENC_STRU*enc,Word16 *input,double *X,double *E_B);

void bw_detector(LC3_ENC_STRU* enc, double* E_B, Word16* Pbw);

void tda_detector(LC3_ENC_STRU* enc, Word16* input, Word32* Fatt);

void sns(
    LC3_ENC_STRU* enc,		// i
    Word32 F_att,			// i: attack flag
    double* X,				// i: frequancy data
    double* E_B,			// i: frequancy bands energy
    double* X_S,			// o: frequanct data after SNS
    Word16* ind_LF,			// o: codebook index for LF vector
    Word16* ind_HF,			// o: codebook index for HF vector
    Word16* shape_j,		// o: shape index for pyramid vector
    Word32* PVQ_PARA		// o: PVQ parameter
);

void tns(LC3_ENC_STRU* enc, Word32 Pbw, double* X_S, Word16 num_tns_filter, Word16* rc_int,
    Word16* rc_order, double* X_F, double* max_abs_X_F, Word16* nbits_TNS, Word16* tns_lpc_weighting);


void ltpf(LC3_ENC_STRU* enc, Word16* input, Word32* pitch_present, Word32* ltpf_active, Word32* pitch_index, Word16* nbits_LTPF);

void spectral_quantization(LC3_ENC_STRU* enc, Word32 nbits_bw, Word32 nbits_TNS, Word32 nbits_LTPF, double* X_F,
    double max_abs_X_F, Word16* X_q, Word32* nbits_spec, Word32* nbits_trunc, double* gg, Word16* gg_ind, Word16* lsbMode, Word16* lastnz_trunc, Word16* rateFlag);

void residual_coding_encoder(LC3_ENC_STRU* enc, Word32 nbits_residual_max, Word16* X_q,
    double* X_F, double gg, Word16* res_bits, Word32* nbits_residual);

void noise_estimation_encoder(LC3_ENC_STRU* enc, Word16* X_q, Word32 Pbw, double* X_f, double gg, Word16* F_NF);

void bitstream_encoder(
    LC3_ENC_STRU* enc,                                                                                  // i
    Word16 nbits_bw, Word16 P_bw,                                                                       // i: bw_detector
    Word16 ind_LF, Word16 ind_HF, Word16 shape_j, Word32* SNS_VQ_ORDER,                                 // i: sns
    Word16 num_tns_filters, Word16* rc_order, Word16 tns_lpc_weighting, Word16* rc_i, Word16* X_q,      // i: tns
    Word16 pitch_present, Word16 ltpf_active, Word16 pitch_index,                                       // i: ltpf
    Word16 lastnz_trunc, Word16 lsbMode, Word16 gg_ind, Word16 rateFlag,                                // i: spectral quantization
    Word32 nbits_residual, Word16* res_bits,                                                            // i: residual coding
    Word16 F_NF,                                                                                        // i: noise level estimation
    UWord8* bytes                                                                                       // o: bitstream
);
#else
    void dct_IV(Word32 nMs_mode, Word16 fs_ind, Word32* dct_input, kiss_fft_state16_ptr kiss_fft_state, Word16 NF, Word16* data_exp);
    void ld_mdct(LC3_ENC_STRU* enc, Word16* input, Word32* X, Word32* E_B,Word16* data_exp, Word16* E_B_exp);
    void bw_detector(
        LC3_ENC_STRU* enc,		// i 
        Word32* E_B,			// i:from ld_mdct
        Word16 E_B_exp,
        Word16* Pbw				// o:P_bw 
    );
    void tda_detector(
        LC3_ENC_STRU* enc,	// i
        Word16* input,		// i:input data
        Word16* F_att		// o:attack Flag
    );
    void sns(
        LC3_ENC_STRU* enc,		// i
        Word16 F_att,			// i: attack flag
        Word32* X,				// i: frequancy data
        Word32* E_B,			// i: frequancy bands energy
        Word16* E_B_exp,		// i: exp of E_B
        Word32* X_S,			// o: frequanct data after SNS
        Word16* ind_LF,			// o: codebook index for LF vector
        Word16* ind_HF,			// o: codebook index for HF vector
        Word16* shape_j,		// o: shape index for pyramid vector
        Word32* PVQ_PARA		// o: PVQ parameter
    );
    void tns(LC3_ENC_STRU* enc, Word32 Pbw, Word32* X_S, Word16 num_tns_filter,Word32 near_nyquist_flag, Word16* rc_int, Word16* rc_order, Word32* X_F, Word16* nbits_TNS, Word16* tns_lpc_weighting);

    void ltpf(LC3_ENC_STRU* enc, Word16* input, Word32* pitch_present, Word32* ltpf_active, Word32* pitch_index, Word16* nbits_LTPF, Word16 ltpf_enable);


    void spectral_quantization(
        LC3_ENC_STRU* enc, Word16 nbits_bw, Word16 nbits_TNS, Word16 nbits_LTPF, Word32* X_F, Word16 X_exp,
        Word16* X_q, Word16* nbits_spec, Word16* nbits_trunc, Word16* gg, Word16* gg_e,
        Word16* gg_ind, Word16* lsbMode, Word16* lastnz_trunc, Word16* rateFlag, Word16* codingdata);


    void residual_coding_encoder(LC3_ENC_STRU* enc, Word16 nbits_residual_max, Word16* X_q,
        Word32* X_F, Word16 X_exp, Word16 gg, Word16 gg_e, Word8* res_bits, Word16* nbits_residual);

    void noise_estimation_encoder(LC3_ENC_STRU* enc, Word16* X_q, Word32 Pbw, 
        Word32* X_f, Word16 X_exp, Word16 gg, Word16 gg_e, Word16* F_NF);

    void bitstream_encoder(
        LC3_ENC_STRU* enc,                                                                          // i
        Word16 nbits_bw, Word16 P_bw,                                                                // i: bw_detector
        Word16 ind_LF, Word16 ind_HF, Word16 shape_j, Word32* PVQ_PARA,                                // i: sns
        Word16 num_tns_filters, Word16* rc_order, Word16 tns_lpc_weighting, Word16* rc_i, Word16* X_q,  // i: tns
        Word16 pitch_present, Word16 ltpf_active, Word16 pitch_index,                                 // i: ltpf
        Word16 lastnz_trunc, Word16 lsbMode, Word16 gg_ind, Word16 rateFlag, Word16* codingdata,         // i: spectral quantization
        Word32 nbits_residual, Word8* res_bits,                                                     // i: residual coding
        Word16 F_NF,                                                                                // i: noise level estimation
        UWord8* bytes                                                                               // o: bitstream
    );
#endif // FIXED_POINT
#endif //#if  (ALG_LC3_ENABLE)
#endif

