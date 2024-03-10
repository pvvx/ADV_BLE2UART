#ifndef _LC3_COMMON_H
#define _LC3_COMMON_H

#include "config.h"

#if (ALG_LC3_ENABLE)

#define LC3_MODE_10MS        (0)
#define LC3_MODE_75MS        (1)

#define LC3_MAX_CHANNELS     (2)
#define LC3_MAX_SAMPLES      (480)
#define LC3_MAX_FRAME_SIZE   (870)

#ifdef LC3_16K_ONLY
#define LC3_MAX_NF           (160)
#define LC3_MAX_NE           (160)
#else
#define LC3_MAX_NF           (480)
#define LC3_MAX_NE           (400)
#endif

#if LC3_MAX_NF==480
#define LC3_MAX_INPUT_DELAY 60
#elif LC3_MAX_NF==320
#define LC3_MAX_INPUT_DELAY 40
#elif LC3_MAX_NF==240
#define LC3_MAX_INPUT_DELAY 30
#elif LC3_MAX_NF==160
#define LC3_MAX_INPUT_DELAY 20
#else
#define LC3_MAX_INPUT_DELAY 10
#endif // LC3_MAX_NF==480


#define   L_NUM_MAX          (11)
#define   L_DEN_MAX          (13)
#define   P_INT_MAX          (228)
#define   LTPF_MEM_MAX       (P_INT_MAX * LC3_MAX_NF / 128 + (L_DEN_MAX + 1) / 2)

typedef enum {
	LC3_PARA_FRAMESIZE = 0,
	LC3_PARA_OUTSIZE,
	LC3_PARA_CHANNELS,
	LC3_PARA_SAMPLERATE,
	LC3_PARA_BITRATE,
	LC3_PARA_SAMPLES,
	LC3_PARA_DELAY,
	LC3_PARA_END
} LC3_PARAMETER;


#endif

#endif
