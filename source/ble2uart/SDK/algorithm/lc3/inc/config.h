#ifndef _CONFIG_H
#define _CONFIG_H

#if (LE_AUDIO_ENABLE)
#define ALG_LC3_ENABLE     1
#define ANDES_INTRINSIC
#endif

#if (ALG_LC3_ENABLE)
#define FIXED_POINT 32
#define DISABLE_LTPF 0
//#define LC3_16K_ONLY
//#define DUMP_IMTERMEDIATE
//#define USE_SPREAD_FFT
//#define ANDES_PROFILE
//#define ANDES_INTRINSIC
//#define EN_SPEC_INTERMEDIATE

#ifdef ANDES_INTRINSIC
#undef DUMP_IMTERMEDIATE
#define ANDES_DSPLIB_MAX
#define ANDES_DSPLIB_DPROD
#endif
#define Errata_ID_15041
//#define KNOWN_ISSUE
//#define KNOWN_ISSUE_NC
//#define OLD_HEADER 


#endif

#endif // !CONFIG_H

