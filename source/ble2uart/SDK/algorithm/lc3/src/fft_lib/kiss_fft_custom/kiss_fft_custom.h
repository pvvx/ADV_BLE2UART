#ifndef KISS_FFT_CUSTOM_H
#define KISS_FFT_CUSTOM_H

#include "../../../inc/config.h"
#if (ALG_LC3_ENABLE)

#include "kiss_fft_type.h"
void kiss_fft_32_16(kiss_fft_state16_ptr cfg, const kiss_fft_cpx_data_32* fin, kiss_fft_cpx_data_32* fout);
kiss_fft_state16_ptr kiss_fft_init_32_16(int nfft, int inverse_fft);

#endif //#if (ALG_LC3_ENABLE)

#endif 
