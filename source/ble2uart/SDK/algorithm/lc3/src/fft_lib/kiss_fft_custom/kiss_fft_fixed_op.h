#ifndef FIXED_OP_H
#define FIXED_OP_H

#include "../../../inc/config.h"
#if (ALG_LC3_ENABLE)

#if __riscv
#include <nds_intrinsic.h>
#endif
#define QCONST16(x,bits) ((short)(.5+(x)*(((int)1)<<(bits))))
#define QCONST32(x,bits) ((int)(.5+(x)*(((int)1)<<(bits))))

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) ((short)(x))
#define EXTEND32(x) ((int)(x))
#define SHR16(a,shift) ((a) >> (shift))
#define SHL16(a,shift) ((a) << (shift))
#define SHR32(a,shift) ((a) >> (shift))
#define SHL32(a,shift) ((a) << (shift))
#define PSHR16(a,shift) (SHR16((a)+((1<<((shift))>>1)),shift))
#define PSHR32(a,shift) (SHR32((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))
#define SATURATE16(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
#define SATURATE32(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))

#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) ((int)(a) << (shift))
#define PSHR(a,shift) (SHR((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define SATURATE(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))


#define ADD16(a,b) ((short)((short)(a)+(short)(b)))
#define SUB16(a,b) ((short)(a)-(short)(b))
#define ADD32(a,b) ((int)(a)+(int)(b))
#define SUB32(a,b) ((int)(a)-(int)(b))


/* result fits in 16 bits */
#define MULT16_16_16(a,b)     ((((short)(a))*((short)(b))))

/* (int)(short) gives TI compiler a hint that it's 16x16->32 multiply */
#define MULT16_16(a,b)     (((int)(short)(a))*((int)(short)(b)))

#define MAC16_16(c,a,b) (ADD32((c),MULT16_16((a),(b))))
#define MULT16_32_Q12(a,b) ADD32(MULT16_16((a),SHR((b),12)), SHR(MULT16_16((a),((b)&0x00000fff)),12))
#define MULT16_32_Q13(a,b) ADD32(MULT16_16((a),SHR((b),13)), SHR(MULT16_16((a),((b)&0x00001fff)),13))
#define MULT16_32_Q14(a,b) ADD32(MULT16_16((a),SHR((b),14)), SHR(MULT16_16((a),((b)&0x00003fff)),14))

#define MULT16_32_Q11(a,b) ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11))
#define MAC16_32_Q11(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11)))

#define MULT16_32_P15(a,b) ADD32(MULT16_16((a),SHR((b),15)), PSHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MULT16_32_Q15(a,b) ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MAC16_32_Q15(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15)))


#define MAC16_16_Q11(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),11)))
#define MAC16_16_Q13(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),13)))
#define MAC16_16_P13(c,a,b)     (ADD32((c),SHR(ADD32(4096,MULT16_16((a),(b))),13)))

#define MULT16_16_Q11_32(a,b) (SHR(MULT16_16((a),(b)),11))
#define MULT16_16_Q13(a,b) (SHR(MULT16_16((a),(b)),13))
#define MULT16_16_Q14(a,b) (SHR(MULT16_16((a),(b)),14))
#define MULT16_16_Q15(a,b) (SHR(MULT16_16((a),(b)),15))

#define MULT16_16_P13(a,b) (SHR(ADD32(4096,MULT16_16((a),(b))),13))
#define MULT16_16_P14(a,b) (SHR(ADD32(8192,MULT16_16((a),(b))),14))
#define MULT16_16_P15(a,b) (SHR(ADD32(16384,MULT16_16((a),(b))),15))

#define MUL_16_32_R15(a,bh,bl) ADD32(MULT16_16((a),(bh)), SHR(MULT16_16((a),(bl)),15))

#define MULT32_16_Q15(a,b) ((long long int)a*(long long int)b>>15)
#define MULT32_16_P15(a,b) (((long long int)a*(long long int)b+(1<<14))>>15)

#define DIV32_16(a,b) ((short)(((int)(a))/((short)(b))))
#define PDIV32_16(a,b) ((short)(((int)(a)+((short)(b)>>1))/((short)(b))))
#define DIV32(a,b) (((int)(a))/((int)(b)))
#define PDIV32(a,b) (((int)(a)+((short)(b)>>1))/((int)(b)))

#   define smul(a,b) ( (int)(a)*(b) )
#   define sround( x )  (short)( ( (x) + (1<<(15-1)) ) >> 15 )

#   define smul32(a,b) ( (long long int)(a)*(b) )
#   define sround32( x )  (int)( ( (x) + (1<<(15-1)) ) >> 15 )

#   define S_MUL(a,b) sround( smul(a,b) )
#if __riscv
#   define S_MUL32(a,b) ( __nds__kmmwb2_u(a,b) )
#else
#   define S_MUL32(a,b) sround32( smul32(a,b) )
#endif
#   define C_MUL_16_16(m,a,b) \
      do{ (m).r = sround( smul((a).r,(b).r) - smul((a).i,(b).i) ); \
          (m).i = sround( smul((a).r,(b).i) + smul((a).i,(b).r) ); }while(0)
#if __riscv
/*
#   define C_MUL_32_16(m,a,b) \
      do{ (m).r = ( __nds__kmmwb2_u((a).r,(b).r) - __nds__kmmwb2_u((a).i,(b).i) ); \
          (m).i = __nds__kmmawb2_u( __nds__kmmwb2_u((a).r,(b).i) , (a).i,(b).r ); }while(0)
*/
#   define C_MUL_32_16(m,a,b) \
      do{ (m).r = ( __nds__kmmwb2_u((a).r,(b).r) - __nds__kmmwb2_u((a).i,(b).i) ); \
          (m).i = ( __nds__kmmwb2_u((a).r,(b).i) + __nds__kmmwb2_u((a).i,(b).r) ); }while(0)

#else
#   define C_MUL_32_16(m,a,b) \
      do{ (m).r = (sround32 (smul32((a).r,(b).r)) - sround32 (smul32((a).i,(b).i)) ); \
          (m).i = (sround32 (smul32((a).r,(b).i)) + sround32(smul32((a).i,(b).r)) ); }while(0)
#endif

// C_MUL4 ½á¹ûÓÒÒÆ17Î»
#   define C_MUL4(m,a,b) \
               do{ (m).r = PSHR32( smul((a).r,(b).r) - smul((a).i,(b).i),17 ); \
               (m).i = PSHR32( smul((a).r,(b).i) + smul((a).i,(b).r),17 ); }while(0)

#   define DIVSCALAR(x,k) \
	(x) = sround( smul(  x, 32767/k ) )

#   define C_FIXDIV(c,div) \
	do {    DIVSCALAR( (c).r , div);  \
		DIVSCALAR( (c).i  , div); }while (0)

#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r =  sround( smul( (c).r , s ) ) ;\
        (c).i =  sround( smul( (c).i , s ) ) ; }while(0)
#if __riscv
#   define C_MULBYSCALAR_32( c, s ) \
    do{ (c).r =  ( __nds__kmmwb2_u( (c).r , s ) ) ;\
        (c).i =  ( __nds__kmmwb2_u( (c).i , s ) ) ; }while(0)
#else
#   define C_MULBYSCALAR_32( c, s ) \
    do{ (c).r =  sround32( smul32( (c).r , s ) ) ;\
        (c).i =  sround32( smul32( (c).i , s ) ) ; }while(0)
#endif
#define  C_ADD( res, a,b)\
    do { \
	    (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)
#define  C_SUB( res, a,b)\
    do { \
	    (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)
#if __riscv
#define  C_ADD_HALF( res, a,b)\
    do { \
	    (res).r=__nds__raddw((a).r,(b).r);  (res).i=__nds__raddw((a).i,(b).i); \
    }while(0)
#define  C_SUB_HALF( res, a,b)\
    do { \
	    (res).r=__nds__rsubw((a).r,(b).r);  (res).i=__nds__rsubw((a).i,(b).i); \
    }while(0)
#else
#define  C_ADD_HALF( res, a,b)\
    do { \
	    (res).r=((long long int)(a).r+(b).r)>>1;  (res).i=((long long int)(a).i+(b).i)>>1; \
    }while(0)
#define  C_SUB_HALF( res, a,b)\
    do { \
	    (res).r=((long long int)(a).r-(b).r)>>1;  (res).i=((long long int)(a).i-(b).i)>>1; \
    }while(0)
#endif
#define C_ADDTO( res , a)\
    do { \
	    (res).r += (a).r;  (res).i += (a).i;\
    }while(0)

#define C_SUBFROM( res , a)\
    do {\
	    (res).r -= (a).r;  (res).i -= (a).i; \
    }while(0)

#  define HALF_OF(x) ((x)>>1)
/*
#ifdef FIXED_POINT
#  define KISS_FFT_COS(phase)  floor(MIN(32767,MAX(-32767,.5+32768 * cos (phase))))
#  define KISS_FFT_SIN(phase)  floor(MIN(32767,MAX(-32767,.5+32768 * sin (phase))))
#  define HALF_OF(x) ((x)>>1)

#else
#  define KISS_FFT_COS(phase) (kiss_fft_scalar) cos(phase)
#  define KISS_FFT_SIN(phase) (kiss_fft_scalar) sin(phase)
#  define HALF_OF(x) ((x)*.5)
#endif
*/

#endif //#if (ALG_LC3_ENABLE)

#endif
