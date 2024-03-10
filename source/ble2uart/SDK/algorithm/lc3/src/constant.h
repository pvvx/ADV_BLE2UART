#ifndef _CONSTANT_H
#define _CONSTANT_H

#include "../inc/lc3_types.h"
#include "../inc/config.h"

#if  (ALG_LC3_ENABLE)

#define PI 3.1415926

extern  int Nms[2];
/* mdct */
#ifndef FIXED_POINT
/* W_10ms_NF for MDCT*/
extern  double w_0_N80[130];
extern  double w_0_N160[260];
extern  double w_0_N240[390];
extern  double w_0_N320[520];
extern  double w_0_N480[780];
/* W_7.5ms_NF for MDCT*/
extern  double w_1_N60[106];
extern  double w_1_N120[212];
extern  double w_1_N180[318];
extern  double w_1_N240[424];
extern  double w_1_N360[636];
extern  double * w_Nms_NF_all[2][5];
#else
#ifdef USE_SPREAD_FFT
extern  Word32 RotVector_40_32[2 * 28];
#ifndef LC3_16K_ONLY
extern  Word16 RotVector_240[2 * (240 - 30)];
extern  Word16 RotVector_150[150];
#endif
extern  Word16 RotVector_160[2 * (160 - 20)];
//extern  Word16 RotVector_120[2 * (120 - 20)];
//extern  Word16 RotVector_100[100];
extern  Word16 RotVector_330[330];
//extern  Word16 RotVector_220[220];
#endif
/* W_10ms_NF for MDCT*/
extern  Word16 w_0_N80[130];
extern  Word16 w_0_N160[260];
#ifndef LC3_16K_ONLY
extern  Word16 w_0_N240[390];
extern  Word16 w_0_N320[520];
extern  Word16 w_0_N480[780];
#endif
/* W_7.5ms_NF for MDCT*/
extern  Word16 w_1_N60[106];
extern  Word16 w_1_N120[212];
#ifndef LC3_16K_ONLY
extern  Word16 w_1_N180[318];
extern  Word16 w_1_N240[424];
extern  Word16 w_1_N360[636];
#endif
#ifndef LC3_16K_ONLY
extern  Word16*  w_Nms_NF_all[2][5];
#else
extern  Word16*  w_Nms_NF_all[2][2];
#endif
/* 0x5a82799a*/
#define TWIDDLE (0x5a82)

/* twiddle for dct_IV */
extern  Word16 mdct_twiddle_0_N80[80];
extern  Word16 mdct_twiddle_0_N160[160];
#ifndef LC3_16K_ONLY
extern  Word16 mdct_twiddle_0_N240[240];
extern  Word16 mdct_twiddle_0_N320[320];
extern  Word16 mdct_twiddle_0_N480[480];
#endif
extern  Word16 mdct_twiddle_1_N60[60];
extern  Word16 mdct_twiddle_1_N120[120];
#ifndef LC3_16K_ONLY
extern  Word16 mdct_twiddle_1_N180[180];
extern  Word16 mdct_twiddle_1_N240[240];
extern  Word16 mdct_twiddle_1_N360[360];
#endif
#ifndef LC3_16K_ONLY
extern  Word16*  mdct_twiddle_all[2][5];
#else
extern  Word16*  mdct_twiddle_all[2][2];
#endif
/* sin_twiddle for dct_IV*/
extern  Word16 mdct_sin_twiddle_161[];
extern  Word16 mdct_sin_twiddle_181[];
extern  Word16 mdct_sin_twiddle_241[];

/* sin_step for dct_IV */
extern  Word16 mdct_sin_step_all[5];

extern  Word16 InvIntTable[32];
#endif // !FIXED_PONIT
extern  Word32 I_8000[65];
extern  Word32 I_16000[65];
#ifndef LC3_16K_ONLY
extern  Word32 I_24000[65];
extern  Word32 I_32000[65];
extern  Word32 I_48000[65];
#endif
extern  Word32 I_8000_7_5ms[61];
extern  Word32 I_16000_7_5ms[65];
#ifndef LC3_16K_ONLY
extern  Word32 I_24000_7_5ms[65];
extern  Word32 I_32000_7_5ms[65];
extern  Word32 I_48000_7_5ms[65];
#endif
#ifndef LC3_16K_ONLY
extern  Word32 *  I_FS[2][5];
#else
extern  Word32*  I_FS[2][2];
#endif
extern  Word16 bands_nrg_scale[32];

/* bw detector */
extern  Word32 Ibw_start_16000_10ms[4];
extern  Word32 Ibw_start_24000_10ms[4] ;
extern  Word32 Ibw_start_32000_10ms[4] ;
extern  Word32 Ibw_start_48000_10ms[4] ;
extern  Word32 Ibw_start_16000_7_5ms[4] ;
extern  Word32 Ibw_start_24000_7_5ms[4] ;
extern  Word32 Ibw_start_32000_7_5ms[4] ;
extern  Word32 Ibw_start_48000_7_5ms[4] ;
extern  Word32*  Ibw_start_all_10ms[4] ;
extern  Word32*  Ibw_start_all_7_5ms[4];

extern  Word32 Ibw_stop_16000_10ms[4] ;
extern  Word32 Ibw_stop_24000_10ms[4] ;
extern  Word32 Ibw_stop_32000_10ms[4] ;
extern  Word32 Ibw_stop_48000_10ms[4];
extern  Word32 Ibw_stop_16000_7_5ms[4] ;
extern  Word32 Ibw_stop_24000_7_5ms[4];
extern  Word32 Ibw_stop_32000_7_5ms[4] ;
extern  Word32 Ibw_stop_48000_7_5ms[4] ;
extern  Word32*  Ibw_stop_all_10ms[4] ;
extern  Word32*  Ibw_stop_all_7_5ms[4] ;

extern  Word32 L_10ms[4];
extern  Word32 L_7_5ms[4];
extern  Word16 bw_nbits_tab[5];
extern  Word32 BW_thresh_quiet[4] ;  
extern  Word16 BW_thresh_quiet_exp ;

/* sns */
extern  Word16 g_tilt_8000[64];
extern  Word16 g_tilt_8000_e[64];
extern  Word16 g_tilt_16000[64];
extern  Word16 g_tilt_16000_e[64];
#ifndef LC3_16K_ONLY
extern  Word16 g_tilt_24000[64];
extern  Word16 g_tilt_24000_e[64];
extern  Word16 g_tilt_32000[64];
extern  Word16 g_tilt_32000_e[64];
extern  Word16 g_tilt_48000[64];
extern  Word16 g_tilt_48000_e[64];
#endif
#ifndef LC3_16K_ONLY
extern  Word16*  g_tilt_all[5];
extern  Word16*  g_tilt_all_e[5];
#else
extern  Word16*  g_tilt_all[2];
extern  Word16*  g_tilt_all_e[2];
#endif
extern  Word16 isqrt_Q16tab[1 + 64];
extern  Word32 ISqrtTable[32];
extern  Word16 ISqrtDiffTable[32];
#ifdef FIXED_POINT
extern  Word16 LFCB_fix[256];
extern  Word16 HFCB_fix[256];
#else
extern  double LFCB[256];
extern  double HFCB[256];
#endif

#ifdef FIXED_POINT
extern  Word16 sns_vq_reg_adj_gains_fix[2];
extern  Word16 sns_vq_reg_lf_adj_gains_fix[4];
extern  Word16 sns_vq_near_adj_gains_fix[4];
extern  Word16 sns_vq_far_adj_gains_fix[8];
extern  Word16*  sns_vq_adj_gains_fix[4];
#else
extern  double sns_vq_reg_adj_gains[2] ;
extern  double sns_vq_reg_lf_adj_gains[4] ;
extern  double sns_vq_near_adj_gains[4] ;
extern  double sns_vq_far_adj_gains[8] ;
extern  double* sns_vq_adj_gains[4];
#endif

extern  Word32 sns_gainMSBbits[4] ;
extern  Word32 sns_gainLSBbits[4] ;

extern  UWord32 MPVQ_offsets[16][11] ;
#ifndef FIXED_POINT
extern  double D[256];
#endif // !FIXED_POINT



/* TNS */
extern  Word16 tns_sub_start_freq_NB_10ms[6];
extern  Word16 tns_sub_start_freq_WB_10ms[6];
extern  Word16 tns_sub_start_freq_SSWB_10ms[6];
extern  Word16 tns_sub_start_freq_SWB_10ms[6];
extern  Word16 tns_sub_start_freq_FB_10ms[6];
extern  Word16 tns_sub_start_freq_NB_7_5ms[6];
extern  Word16 tns_sub_start_freq_WB_7_5ms[6];
extern  Word16 tns_sub_start_freq_SSWB_7_5ms[6];
extern  Word16 tns_sub_start_freq_SWB_7_5ms[6];
extern  Word16 tns_sub_start_freq_FB_7_5ms[6];
extern  Word16 tns_sub_stop_freq_NB_10ms[6];
extern  Word16 tns_sub_stop_freq_WB_10ms[6];
extern  Word16 tns_sub_stop_freq_SSWB_10ms[6];
extern  Word16 tns_sub_stop_freq_SWB_10ms[6];
extern  Word16 tns_sub_stop_freq_FB_10ms[6];

extern  Word16 tns_sub_stop_freq_NB_7_5ms[6];
extern  Word16 tns_sub_stop_freq_WB_7_5ms[6];
extern  Word16 tns_sub_stop_freq_SSWB_7_5ms[6];
extern  Word16 tns_sub_stop_freq_SWB_7_5ms[6];
extern  Word16 tns_sub_stop_freq_FB_7_5ms[6];

extern  Word16*  tns_sub_start_freq_all[2][5];
extern  Word16*  tns_sub_stop_freq_all[2][5];

extern  Word16 ac_tns_order_bits[2][8] ;
extern  Word16 ac_tns_order_freq[2][8];
extern  Word16 ac_tns_order_cumfreq[2][8];
extern  Word16 ac_tns_coef_bits[8][17] ;
extern  Word16 ac_tns_coef_freq[8][17] ;
extern  Word16 ac_tns_coef_cumfreq[8][17] ;
extern  Word32 tnsAcfWindow[8];
extern  Word16 tnsQuantThr[16];
extern  Word16 tnsQuantPts[17];
/* Long Term Postfiltering*/
extern  Word16 resamp_params[5][4];
#ifdef FIXED_POINT
/*extern  Word16 tab_resamp_filter_8k_16k[239];// Q_16k = 15,Q_8k =14
extern  Word16 tab_resamp_filter_24k[239];//Q_24 = 15
extern  Word16 tab_resamp_filter_32k[239];//Q_32 = 15
extern  Word16 tab_resamp_filter_48k[239];//Q_48 = 15
extern  Word16*  tab_resamp_filter_all[5];*/
extern  Word16 resamp_filt_8k[240];
extern  Word16 resamp_filt_16k[240];
#ifndef LC3_16K_ONLY
extern  Word16 resamp_filt_24k[240];
extern  Word16 resamp_filt_32k[240];
extern  Word16 resamp_filt_48k[240];
#endif
#ifndef LC3_16K_ONLY
extern  Word16*  resamp_filts[5];
#else
extern  Word16*  resamp_filts[2];
#endif
extern  Word16 ltpf_ac_interp_filt[7][9];
extern  Word16 inter_filter[5][4][12];
#ifdef ANDES_INTRINSIC
extern  Word16 inter_filter_i[3] ;
extern  Word16 inter_filter_fr[5][4][4];
#endif
extern  Word16 inter_filter_shift[5];
extern  Word16 inter_filter_len[5];
extern  Word32 isqrt_table[128 + 2];
extern  Word16 highpass50_num[3];
extern  Word16 highpass50_den[2];
#ifdef ANDES_INTRINSIC
extern  Word16 highpass50[6];
#endif // ANDES_INTRINSIC
extern  Word16 DownSamp_h2[5]; //Q15
extern  Word16 R_6k4_corr_weighting[98];
extern  Word16 tab_ltpf_interp_R[31];
#else
extern  double tab_resamp_filter[239];
extern  double tab_ltpf_interp_R[31];

extern  double h12k8_coef_4[240];
extern  double h12k8_coef_3[240];
extern  double h12k8_coef_2[240];
extern  double h12k8_coef_1[240];
extern  double h12k8_coef_0[240];
extern  double* h12k8_coef_all[5];
extern  double highpass50_num[3];
extern  double highpass50_den[2];
extern  double DownSamp_h2[5];
extern  double R_6k4_corr_weighting[98];

extern  double tab_ltpf_interp_x12k8[15];
#endif // FIXED_POINT


#define LTPF_K_MAX 114
#define LTPF_K_MIN 17
#ifdef FIXED_POINT
extern  Word16 tab_ltpf_num_8000_fix[4 * 3];
extern  Word16 tab_ltpf_num_16000_fix[4 * 3];
#ifndef LC3_16K_ONLY
extern  Word16 tab_ltpf_num_24000_fix[4 * 5];
extern  Word16 tab_ltpf_num_32000_fix[4 * 7];
extern  Word16 tab_ltpf_num_48000_fix[4 * 11];
#endif
#ifndef LC3_16K_ONLY
extern  Word16*  tab_ltpf_num_fix[5];
#else
extern  Word16*  tab_ltpf_num_fix[2];
#endif
extern  Word16 tab_ltpf_den_8000_fix[4 * 5];
extern  Word16 tab_ltpf_den_16000_fix[4 * 5];
#ifndef LC3_16K_ONLY
extern  Word16 tab_ltpf_den_24000_fix[4 * 7];
extern  Word16 tab_ltpf_den_32000_fix[4 * 9];
extern  Word16 tab_ltpf_den_48000_fix[4 * 13];
#endif
#ifndef LC3_16K_ONLY
extern  Word16*  tab_ltpf_den_fix[5];
#else
extern  Word16*  tab_ltpf_den_fix[2];
#endif
#else
extern  double tab_ltpf_num_8000[4*3];
extern  double tab_ltpf_num_16000[4*3];
extern  double tab_ltpf_num_24000[4*5];
extern  double tab_ltpf_num_32000[4*7];
extern  double tab_ltpf_num_48000[4*11];
extern  double* tab_ltpf_num[5];
extern  double tab_ltpf_den_8000[4*5];
extern  double tab_ltpf_den_16000[4*5];
extern  double tab_ltpf_den_24000[4*7];
extern  double tab_ltpf_den_32000[4*9];
extern  double tab_ltpf_den_48000[4*13];
extern  double* tab_ltpf_den[5];
#endif
extern  Word16 tab_ltpf_num_len[5];
extern  Word16 tab_ltpf_den_len[5];

/* spectral data */
#define NBITS_SNS 38
#define NBITS_GAIN 8
#define NBITS_NF 3
extern  UWord8 ac_spec_lookup[4096];
extern  Word16 ac_spec_cumfreq[64][17];
extern  Word16 ac_spec_freq[64][17];
extern  Word16 ac_spec_bits[64][17];
extern  Word16 Tab_esc_nb[4];
extern  Word32 global_gain_tab[28];
extern  Word16 global_gain_t1[5];
extern  Word16 global_gain_t2[5];
extern  Word16 global_gain_t3[5];
extern  Word16 global_gain_t4[5];
extern  Word16 global_gain_t5[5];
/* noise estimation */
extern Word16 bw_stop_10ms[5];
extern Word16 bw_stop_7_5ms[5];

/* basic op */
extern  Word32 Log2_16_table1[16];

extern  Word16 Log2_16_table2[16] ;
extern  Word16 ldCoeff[7];
/* table for 2^x */
extern  UWord32 exp2x_tab_long[32] ;
extern  UWord32 exp2_tab_long[32] ;
extern  UWord32 exp2w_tab_long[32];
/* 1/x tables */
extern  Word32 InvTable[32];
extern  Word16 InvDiffTable[32];

#endif //#if  (ALG_LC3_ENABLE)

#endif // CONSTANT_H

