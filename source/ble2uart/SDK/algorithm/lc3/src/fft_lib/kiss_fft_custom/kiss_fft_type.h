#ifndef KISS_FFT_TYPE_H
#define KISS_FFT_TYPE_H

#include "../../../inc/config.h"
#if (ALG_LC3_ENABLE)

//#define FIXED_POINT
#define ABS(x) ((x) < 0 ? (-(x)) : (x))      /**< Absolute integer value. */
#define ABS16(x) ((x) < 0 ? (-(x)) : (x))    /**< Absolute 16-bit value.  */
#define MIN16(a,b) ((a) < (b) ? (a) : (b))   /**< Maximum 16-bit value.   */
#define MAX16(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum 16-bit value.   */
#define ABS32(x) ((x) < 0 ? (-(x)) : (x))    /**< Absolute 32-bit value.  */
#define MIN32(a,b) ((a) < (b) ? (a) : (b))   /**< Maximum 32-bit value.   */
#define MAX32(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum 32-bit value.   */

typedef struct {
    short r;
    short i;
}kiss_fft_cpx_twiddle;

typedef struct {
    short r;
    short i;
}kiss_fft_cpx_data_16;

typedef struct {
    short r;
    short i;
}kiss_fft_cpx_twiddle_16;

typedef struct {
    int r;
    int i;
}kiss_fft_cpx_data_32;

#define MAX_STAGE 32
#define NFFT 240

struct kiss_fft_state16 {
    int nfft;
    int inverse;
    int factors[2 * MAX_STAGE];
    int Q_shift;
    kiss_fft_cpx_twiddle_16 twiddles[NFFT];
};
typedef struct kiss_fft_state16* kiss_fft_state16_ptr;

#endif //#if (ALG_LC3_ENABLE)

#endif // !KISS_FFT_H

