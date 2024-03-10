
#ifndef __DUMP_H__
#define __DUMP_H__

#define    DUMP_INPUT  0x1
#define    DUMP_MDCT   0x2;

/* encoder part */
#define    FILE_MDCT     "enc_mdct.txt"
#define    FILE_ENERGY   "enc_energy.txt"
#define    FILE_BW_D     "enc_bw_detector.txt"
#define    FILE_SNS      "enc_sns.txt"
#define    FILE_SNS_SCF  "enc_sns_scf.txt"
#define    FILE_TNS_X_F  "enc_tns_X_F_f.txt"

/* decoder part */
#define    FILE_DEC_ARI  "dec_arithmatic.txt"
#define    FILE_DEC_RES  "dec_residual.txt"
#define    FILE_DEC_NF   "dec_noisefilling.txt"
#define    FILE_DEC_GG   "dec_globalgain.txt"
#define    FILE_DEC_TNS  "dec_tns.txt"
#define    FILE_DEC_SNS  "dec_sns.txt"
#define    FILE_DEC_IMDCT     "dec_imdct.txt"
#define    FILE_DEC_LTPF      "dec_ltpf.txt"
#define    FILE_DEC_SNS_SCF   "dec_sns_scf.txt"
#define    FILE_DEC_SNS_G_SNS "dec_sns_g_sns.txt"
#define    FILE_LTPF_X_12k8_D "ltpf_x12k8_D.txt"


#define    DUMP_FLAG   (DUMP_MDCT)

#define    FILE_OPEN(fp, name) \
    fp = fopen(name, "w"); \
    if (fp == NULL) { \
        printf("failed to open %s\n", name); \
        return -1; \
    }

int dump_open();
void dump_close();

void dump_dec_arithmatic(short* in, int len);
void dump_dec_residual(int* in, short exponent, int len);
void dump_dec_residual_f(double* in, int len);
void dump_dec_noisefilling(int* in, short exponent, int len);
void dump_dec_noisefilling_f(double* in, int len);
void dump_dec_global_gain(int* in, short exponent, int len);
void dump_dec_global_gain_f(double* in, int len);
void dump_dec_tns(int* in, short exponent, int len);
void dump_dec_tns_f(double* in, int len);
void dump_dec_sns(int* in, short exponent, int len);
void dump_dec_sns_f(double* in, int len);
void dump_dec_sns_scf(short* inbuf, short exponent, short len);
void dump_dec_sns_scf_f(double* inbuf, short len);
void dump_dec_sns_g_sns(short* inbuf, short* exponent, short len);
void dump_dec_sns_g_sns_f(double* inbuf, short len);
void dump_dec_imdct1(int* inbuf, short exponent, short len);
void dump_dec_imdct(short* inbuf, short exponent, short len);
void dump_dec_imdct_f(double* inbuf, short len);
void dump_dec_ltpf(short* inbuf, short exponent, short len);
void dump_dec_ltpf1(int* inbuf, short exponent, short len);
void dump_dec_ltpf_f(double* inbuf, short len, short ltpf_active);
void dump_mdct(int* inbuf, short exponent, short len);
void dump_mdct_f(double* inbuf, short len);
void dump_energy(int* inbuf, short exponent, short len);
void dump_energy_f(double* inbuf, short len);
void dump_bw_detector(int bw_idx);
void dump_sns(int* inbuf, short exponent, short len);
void dump_sns_f(double* inbuf, short len);
void dump_sns_scf(short* inbuf, short exponent, short len);
void dump_sns_scf_f(double* inbuf, short len);
void dump_sns_index(int index_LF, int index_HF);
void dump_tns_X_F(int* inbuf, short exponent, short len);
void dump_tns_X_F_f(double* inbuf, short len);
void dump_sq_X_q_f(short* X_q, short len);
void dump_ltpf_x_tilde_12k8D_f(double* inbuf, short len);
void dump_ltpf_x_tilde_12k8D(short* input, short exponent, short len);
void dump_bw0(int bw0);
void dump_sns_shape_index(int shape_j);
void dump_sns_y(short* y0, short* y1, short* y2, short* y3);
void dump_sns_t2_rot(short* t2_rot, short len);
void dump_sns_t2_rot_f(double* t2_rot, short len);
void dump_sns_E_P(int* input, short* exponent, short len);
void dump_sns_E_4_f(double* E_4, short len);
void dump_sns_E_4(int* input, short* exponent, short len);
void dump_sns_r1_f(double* input, short len);
void dump_sns_r1(short* input, short len);
void dump_sns_scf_q(short* input, short exponent, short len);
void dump_sns_scf_q_f(double* input, short len);
void dump_sns_scf_q_int(short* input, short exponent, short len);
void dump_sns_scf_q_int_f(double* input, short len);
void dump_tns_nbits_tns(short input);
void dump_tns_tns_lpc_weighting(short input);
void dump_tns_rc_order(short* input, short len);
void dump_tns_rc_int(short* input, short len);
void dump_ltpf_active(short input);
void dump_ltpf_pitch_index(int input);
void dump_x_6k4(short* input, short exponent);
void dump_x_12k8_HP(short* input, short exponent, int len);
void dump_x_12k8_HP_f(double* input);
void dump_ltpf_nc_T_f(double input);
void dump_ltpf_nc_T(short input, short exponent);
void dump_side_information(char* input, int len);
void dump_mdct_input(int* input, short exponent, int len);
void dump_tda_x_hp(short* input, int len);
void dump_F_att(short input);
void dump_short_data(short input);
void dump_short_matric(short* input, int len);
void dump_int_data(int input);
void dump_int_matric(int* input, int len);
void dump_sq_X_q(short* input, int len);
void dump_res_bits(char* input, int len);
void dump_F_NF(short input);
void dump_output(char* input, int len);
void dump_pitch_present(int input);
void dump_ltpf_bits(int input);
void dump_ltpf_x6k4(short* input, int len);
void dump_mdct_output(int* input, short exponent, int len);
#endif

