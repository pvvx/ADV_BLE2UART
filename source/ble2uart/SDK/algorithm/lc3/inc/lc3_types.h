#ifndef _LC3_TYPES_H
#define _LC3_TYPES_H

#include "config.h"

#ifndef ANDES_INTRINSIC
typedef char      Word8;
typedef short     Word16;
typedef int       Word32;
typedef long long Word64;

typedef unsigned char    UWord8;
typedef unsigned short   UWord16;
typedef unsigned int     UWord32;
#else
#include <nds_math_types.h>
typedef int8_t	Word8;
typedef int16_t	Word16;
typedef int32_t	Word32;
typedef int64_t	Word64;

typedef uint8_t    UWord8;
typedef uint16_t   UWord16;
typedef uint32_t     UWord32;
#endif
typedef struct {
	Word16 r;
	Word16 i;
}lc3_cpx;

typedef struct  {
	Word32 r;
	Word32 i;
}kiss_fft_cpx;

struct kiss_fft_state {
	int nfft;
	int inverse;
	int factors[2 * 32]; //  32->MAXFACTORS
	kiss_fft_cpx twiddles[1];
};

typedef struct kiss_fft_state* kiss_fft_cfg;
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX_32
#define MAX_32 (0x7fffffffL)
#endif // !MAX_32

#ifndef MIN_32
#define MIN_32 (0x80000000L)
#endif // !MIN_32

#ifndef MAX_16
#define MAX_16     (32767)
#endif

#ifndef MIN_16
#define MIN_16     (-32768)
#endif

#endif
