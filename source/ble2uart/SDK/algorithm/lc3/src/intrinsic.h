#ifndef _INTRINSIC_H
#define _INTRINSIC_H

#include "../inc/lc3_types.h"

#if  (ALG_LC3_ENABLE)
#ifdef   ANDES_INTRINSIC
#include <nds_intrinsic.h>
#include <nds_utils_math.h>
#endif

#ifdef   ANDES_INTRINSIC
/* 16-bit Count Leading Redundant Sign */
inline Word16 clrs16(Word16 a)
{
    return __nds__clrs16(a);
}
/* 32-bit count leading zero */
inline UWord32 clz32(UWord32 a) {
    return __nds__clz32(a);
}

/* vin1 : Q15,  vin2: Q15 ---> vout: Q15 */
inline Word16 mult_16(Word16 vin1, Word16 vin2)
{
    Word16 vout;
    vout = __nds__khmbb(vin1, vin2);
    return vout;
}
inline Word32 mult_16_16_32(Word16 vin1, Word16 vin2)
{
    return __nds__smul16(vin1, vin2);
}
/* vin1 : Q15,  vin2: Q15 ---> vout: Q15 */
inline Word16 mult_r_16(Word16 vin1, Word16 vin2)
{
    return __nds__sra_u(__nds__smul16(vin1, vin2),15);
}
inline Word32 mult_32(Word32 vin1, Word32 vin2)
{
    return __nds__smmul(vin1, vin2);
}

/* 32-bit count leading redundant sign */
inline UWord32 clrs32(UWord32 a){
    return  __nds__clrs32(a);
}

/* 32-bit shift right arithmetic */
inline UWord32 sra32(UWord32 a, UWord32 b) {
    return __nds__sra32(a, b);
}

inline Word32 mult_32_16_32(Word32 vin1, Word16 vin2)
{
    return __nds__smmwb(vin1, (UWord32)vin2);
}

inline Word32 mult_r_32_16_32(Word32 vin1, Word16 vin2)
{
    return __nds__smmwb_u(vin1, (UWord32)vin2);
}

inline Word32 mult_32_Q31_32(Word32 vin1, Word32 vin2)
{
    return __nds__kwmmul(vin1, vin2);
}

inline Word32 mult_r_32_Q31_32(Word32 vin1, Word32 vin2)
{
    return __nds__kwmmul_u(vin1, vin2);
}

inline Word32 mult_32_Q15_32(Word32 vin1, Word16 vin2)
{
    return __nds__kmmwb2(vin1, vin2);
}

inline Word32 mult_r_32_Q15_32(Word32 vin1, Word16 vin2)
{
    return __nds__kmmwb2_u(vin1, vin2);
}

inline Word32 mac_r_32_Q15_32(Word32 acc, Word32 vin1, Word16 vin2)
{
    return __nds__kmmawb2_u(acc, vin1, vin2);
}

inline Word32 mac_r_32_16_32(Word32 acc, Word32 vin1, Word16 vin2)
{
    return __nds__kmmawb_u(acc, vin1, vin2);
}

inline Word32 mac_16_16_32(Word32 vin, Word16 vin1, Word16 vin2)
{
    return __nds__kmabb(vin, vin1, vin2);
}

inline Word32 L_shl(Word32 L_var1, Word16 var2)
{
    return __nds__kslraw(L_var1, var2);
}

inline Word32 L_shr(Word32 L_var1, Word16 var2)
{
    return __nds__kslraw(L_var1, -var2);
}
/*
inline Word16 lshr(Word16 var1, Word16 var2)
{
	//printf("\ninput=%d %d",var1,var2);
	//printf("\noutput=%d",__nds__srl16(var1, var2));
    return (Word16)__nds__srl16(var1, var2);

}*/
inline Word32 L_add(Word32 L_var1, Word32 L_var2)
{
	return __nds__kaddw(L_var1, L_var2);
}

inline Word32 L_sub(Word32 L_var1, Word32 L_var2)
{
    return __nds__ksubw(L_var1, L_var2);
}
inline Word32 L_add_half(Word32 L_var1, Word32 L_var2)
{
    return __nds__raddw(L_var1, L_var2);
}

inline Word32 L_sub_half(Word32 L_var1, Word32 L_var2)
{
    return __nds__rsubw(L_var1, L_var2);
}
inline Word16 abs16(Word16 x)
{
    return __nds__kabs16(x);
}
inline Word32 abs32(Word32 x)
{
    return __nds__kabsw(x);
}
inline Word16 convert_q31_q15_u(Word32 x)
{
    return __nds__sra_u(x, 16);
}
inline Word32 L_mult(Word16 var1, Word16 var2)
{
    return __nds__kdmbb(var1, var2);
}
inline Word32 L_mac(Word32 L_var3, Word16 var1, Word16 var2)
{
    return __nds__kaddw(L_var3, __nds__kdmbb(var1, var2));
}
inline Word32 L_msu(Word32 L_var3, Word16 var1, Word16 var2)
{
    return __nds__ksubw(L_var3, __nds__kdmbb(var1, var2));
}
/*  Multiply var1 by var2 and shift the result left by 1. Add the 32 bit
    result to L_var3 with saturation. Round the LS 16 bits of the result
    into the MS 16 bits with saturation and shift the result right by 16. */
inline Word16 mac_r(Word32 L_var3, Word16 var1, Word16 var2)
{
    return __nds__sra_u(__nds__kaddw(L_var3, __nds__kdmbb(var1, var2)), 16);
}
inline Word16 msu_r(Word32 L_var3, Word16 var1, Word16 var2)
{
    return __nds__sra_u(__nds__ksubw(L_var3, __nds__kdmbb(var1, var2)), 16);
}
#else

inline UWord32 clz32(UWord32 a) {
    int i;
    for (i = 32; i >= 0; i--) {
        if (a == 0)
            break;
        a = a >> 1;
    }
    return i;
}

/* vin1 : Q15,  vin2: Q15 ---> vout: Q15 */
inline Word16 mult_16(Word16 vin1, Word16 vin2)
{
    Word32 tmp;
    Word16 vout;
    tmp = (Word32)vin1 * (Word32)vin2;
    if (tmp == 0x40000000) {
        tmp = 0x3fffffff;
    }
    vout = (Word16)(tmp >> 15);
    return vout;
}
inline Word32 mult_16_16_32(Word16 vin1, Word16 vin2)
{
    Word32 vout = (Word32)vin1 * (Word32)vin2;
    return vout;
}
/* vin1 : Q15,  vin2: Q15 ---> vout: Q15 */
inline Word16 mult_r_16(Word16 vin1, Word16 vin2)
{
    Word32 tmp;
    Word16 vout;
    tmp = mult_16_16_32(vin1, vin2);
    tmp += 0x4000;
    vout = (Word16)(tmp >> 15);
    return vout;
}
inline Word32 mult_32(Word32 vin1, Word32 vin2)
{
    Word64 tmp;
    Word32 vout;
    tmp = (Word64)vin1 * (Word64)vin2;
    vout = (Word32)(tmp >> 32);
    return vout;
}
inline Word16 clrs16(Word16 a)
{
    int i;
    UWord32 cnt = 0;
    for (i = 14; i >= 0; i--)
    {

        if (((a >> i) & 1) == ((a >> 15) & 1))
        {
            cnt++;
        }
        else
        {
            break;
        }
    }
    return cnt;
}
inline UWord32 clrs32(UWord32 a) {
    int i;
    UWord32 cnt=0;
    for ( i = 30; i >= 0; i--)
    {

        if ( ((a>>i)&1) == ((a>>31)&1) )
        {
            cnt++;
        }
        else
        {
            break;
        }
    }
    return cnt;
}

inline UWord32 sra32(Word32 a, UWord32 b)
{
    return a >> b;
}

inline Word32 mult_32_16_32(Word32 vin1, Word16 vin2)
{
    return (Word32)(((Word64)vin1 * (Word64)vin2) >> 16);
}

inline Word32 mult_r_32_16_32(Word32 vin1, Word16 vin2)
{
    Word64 round_factor = 1 << 15;
    return (Word32)(((Word64)vin1 * (Word64)vin2 + round_factor) >> 16);
}

inline Word32 mult_32_Q31_32(Word32 vin1, Word32 vin2)
{
    return (Word32)(((Word64)vin1 * (Word64)vin2) >> 31);
}

inline Word32 mult_r_32_Q31_32(Word32 vin1, Word32 vin2)
{
    Word64 round_factor = 1 << 30;
    return (Word32)(((Word64)vin1 * (Word64)vin2 + round_factor) >> 31);
}

inline Word32 mult_32_Q15_32(Word32 vin1, Word16 vin2)
{
    Word64 tmp = ((Word64)vin1 * (Word64)vin2);
    return (Word32)(tmp >> 15);
}

inline Word32 mult_r_32_Q15_32(Word32 vin1, Word16 vin2)
{
    Word64 round_factor = 1 << 14;
    Word64 tmp = ((Word64)vin1 * (Word64)vin2);
    return (Word32)((tmp + round_factor) >> 15);
}

inline Word32 mac_r_32_Q15_32(Word32 acc, Word32 vin1, Word16 vin2)
{
    Word64 round_factor = 1 << 14;
    Word64 tmp = ((Word64)vin1 * (Word64)vin2);
    return (Word32)(acc + ((tmp + round_factor) >> 15));
}

inline Word32 mac_r_32_16_32(Word32 acc, Word32 vin1, Word16 vin2)
{
    Word64 round_factor = 1 << 15;
    Word64 tmp = ((Word64)vin1 * (Word64)vin2);
    return (Word32)(acc + ((tmp + round_factor) >> 16));
}

inline Word32 mac_16_16_32(Word32 vin, Word16 vin1, Word16 vin2)
{
    Word32 vout = vin + (Word32)vin1 * (Word32)vin2;
    return vout;
}
/* 32-bit logical left shift or arithmetic right shift*/
inline Word32 L_shl(Word32 L_var1, Word16 var2)
{

    Word32 L_var_out = 0L;
    //int Overflow = 0;

    if (var2 <= 0)
    {
        if (var2 < -32)
            var2 = -32;
        var2 = -var2;
        //L_var_out = L_shr(L_var1, var2);
        if (var2 >= 31)
        {
            L_var_out = (L_var1 < 0L) ? -1 : 0;
        }
        else
        {
            if (L_var1 < 0)
            {
                L_var_out = ~((~L_var1) >> var2);
            }
            else
            {
                L_var_out = L_var1 >> var2;
            }
        }
    }
    else
    {
        for (; var2 > 0; var2--)
        {
            if (L_var1 > (Word32)0X3fffffffL)
            {
                //Overflow = 1;
                L_var_out = MAX_32;
                break;
            }
            else
            {
                if (L_var1 < (Word32)0xc0000000L)
                {
                    //Overflow = 1;
                    L_var_out = MIN_32;
                    break;
                }
            }
            L_var1 *= 2;
            L_var_out = L_var1;
        }
    }

    return (L_var_out);
}

inline Word32 L_shr(Word32 L_var1, Word16 var2)
{
    Word32 L_var_out;

    if (var2 < 0)
    {
        if (var2 < -32)
            var2 = -32;
        var2 = -var2;
        L_var_out = L_shl(L_var1, var2);
    }
    else
    {
        if (var2 >= 31)
        {
            L_var_out = (L_var1 < 0L) ? -1 : 0;
        }
        else
        {
            if (L_var1 < 0)
            {
                L_var_out = ~((~L_var1) >> var2);
            }
            else
            {
                L_var_out = L_var1 >> var2;
            }
        }
    }

    return (L_var_out);
}


inline Word32 L_add(Word32 L_var1, Word32 L_var2)
{
    Word32 L_var_out;
   int Overflow = 0;

    L_var_out = L_var1 + L_var2;

    if (((L_var1 ^ L_var2) & 0x80000000L) == 0)
    {
        if ((L_var_out ^ L_var1) & 0x80000000L)
        {
            L_var_out = (L_var1 < 0) ? 0x80000000L : 0x7fffffffL;
            Overflow = 1;
        }
    }
    if (Overflow == 1)
    {
        //printf("LaddÒç³ö\n");
    }
    return (L_var_out);
}

inline Word32 L_sub(Word32 L_var1, Word32 L_var2)
{
    Word32 L_var_out;

    L_var_out = L_var1 - L_var2;

    if (((L_var1 ^ L_var2) & MIN_32) != 0)
    {
        if ((L_var_out ^ L_var1) & MIN_32)
        {
            L_var_out = (L_var1 < 0L) ? MIN_32 : MAX_32;
            //printf("LsubÒç³ö\n");
        }
    }
    return (L_var_out);
}

inline Word32 L_add_half(Word32 L_var1, Word32 L_var2)
{
    return (L_add(L_var1, L_var2)) >> 1;
}

inline Word32 L_sub_half(Word32 L_var1, Word32 L_var2)
{
    return (L_sub(L_var1, L_var2)) >> 1;
}

inline Word16 abs16(Word16 x)
{
    if (x < 0)
    {
        if (x == MIN_16)
        {
            return MAX_16;
        }
        else
        {
            return -x;
        }

    }
    else
    {
        return x;
    }
}
inline Word32 abs32(Word32 x)
{
    if (x < 0)
    {
        if (x == MIN_32)
        {
            return MAX_32;
        }
        else
        {
            return -x;
        }

    }
    else
    {
        return x;
    }
}
inline Word16 convert_q31_q15_u(Word32 x)
{
    return (x + 0x8000) >> 16;
}
inline Word32 L_mult(Word16 var1, Word16 var2)
{
    Word32 L_var_out;
    L_var_out = (Word32)var1 * (Word32)var2;

    if (L_var_out != (Word32)0x40000000L)
    {
        L_var_out *= 2;
    }
    else
    {
        L_var_out = 0x7fffffffL;
    }

    return (L_var_out);
}
inline Word32 L_mac(Word32 L_var3, Word16 var1, Word16 var2)
{
    Word32 L_var_out;
    Word32 L_product;

    L_product = L_mult(var1, var2);
    L_var_out = L_add(L_var3, L_product);

    return (L_var_out);
}
inline Word32 L_msu(Word32 L_var3, Word16 var1, Word16 var2)
{
    Word32 L_var_out;
    Word32 L_product;

    L_product = L_mult(var1, var2);
    L_var_out = (L_var3 - L_product);

    return (L_var_out);
}
/*  Multiply var1 by var2 and shift the result left by 1. Add the 32 bit
    result to L_var3 with saturation. Round the LS 16 bits of the result
    into the MS 16 bits with saturation and shift the result right by 16. */
inline Word16 mac_r(Word32 L_var3, Word16 var1, Word16 var2)
{
    Word16 var_out;

    L_var3 = L_mac(L_var3, var1, var2);
    L_var3 = L_add(L_var3, (Word32)0x00008000L);
    var_out = (L_var3) >> 16;

    return (var_out);
}
inline Word16 msu_r(Word32 L_var3, Word16 var1, Word16 var2)
{
    Word16 var_out;

    L_var3 = L_msu(L_var3, var1, var2);
    L_var3 = L_add(L_var3, (Word32)0x00008000L);
    var_out = (L_var3) >> 16;

    return (var_out);
}
#endif



/* below functions needs to be further optimized */
inline Word32 L_negate(Word32 L_var1)
{
    Word32 L_var_out;

    L_var_out = (L_var1 == MIN_32) ? MAX_32 : -L_var1;

    return (L_var_out);
}

inline UWord32 sll32(UWord32 a, UWord32 b)
{
    return a << b;
}

inline Word16 saturate(Word32 vin)
{
    Word16 vout;

    if (vin > MAX_16) {
        vout = MAX_16;
    }
    else if (vin < MIN_16) {
        vout = MIN_16;
    }
    else {
        vout = (Word16)vin;
    }

    return vout;
}

inline Word16 add_s_16(Word16 vin1, Word16 vin2)
{
    Word16 vout;
    Word32 tmp;

    tmp = (Word32)vin1 + (Word32)vin2;
    vout = saturate(tmp);

    return (vout);
}

inline Word16 sub_s_16(Word16 vin1, Word16 vin2)
{
    Word16 vout;
    Word32 tmp;

    tmp = (Word32)vin1 - (Word32)vin2;
    vout = saturate(tmp);

    return (vout);
}

inline Word16 mult_16_shift(Word16 vin1, Word16 vin2, Word32 shift)
{
    Word32 tmp;
    Word16 vout;

    tmp = (Word32)vin1 * (Word32)vin2 >> shift;

    vout = saturate(tmp);
    return vout;
}

inline Word16 mult_r_16_shift(Word16 vin1, Word16 vin2, Word32 shift)
{
    Word32 tmp;
    Word32 round_factor;
    Word16 vout;

    round_factor = 1 << (shift - 1);

    tmp = (Word32)vin1 * (Word32)vin2;
    tmp = (tmp + round_factor) >> shift;

    vout = saturate(tmp);
    return vout;
}

inline Word32 mult_32_shift(Word32 vin1, Word32 vin2, Word32 shift)
{
    Word64 tmp;
    Word32 vout;

    tmp = (Word64)vin1 * (Word64)vin2;
    vout = (Word32)(tmp >> shift);
    return vout;
}

inline Word16 asr_r_16(Word16 vin, Word32 shift)
{
    Word16 vout;
    Word32 round_factor = 1 << (shift - 1);

    vout = (vin + round_factor) >> shift;
    return vout;
}

inline Word32 asr_r_32(Word32 vin, Word32 shift)
{
    Word32 vout;
    Word32 round_factor = 1 << (shift - 1);

    vout = (vin + round_factor) >> shift;
    return vout;
}
inline Word16 norm32_l(Word32 x)
{
    if (x==0)
    {
        return 0;
    }
    else
    {
        return (Word16)clrs32(x);
    }

}

inline Word64 mac_32_32_64(Word32 vin1, Word32 vin2, Word64 vout)
{
    vout = vout + (Word64)vin1 * (Word64)vin1;
    if (vout >= 0x80000000)
    {
        vout = 0x7FFFFFF;
    }
    return vout;
}


inline Word16 norm16_l(Word16 x)
{
    if (x == 0)
    {
        return 0;
    }
    else
    {
        return clrs16(x);
    }
}

inline Word16 negate(Word16 var1)
{
    Word16 var_out;

    var_out = (var1 == MIN_16) ? MAX_16 : -var1;

    return (var_out);
}

inline Word16 lshr(Word16 var1, Word16 var2) {
    Word16 var_out;


    var_out = var1 >> 1;
    var_out = var_out & 0x7fff;
    var_out = var_out >> (var2 - 1);

    //printf("\noutput=%d %d\n", (Word16)__nds__srl16(var1, var2), var_out);
    return (var_out);
}
#define cplxMult32_16_32(r, i, a, b, c, d)                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        r =(( ( (mult_32_16_32(a, c)) ) -  ( (mult_32_16_32(b, d)) ) ));                                              \
        i =(( ( (mult_32_16_32(a, d)) ) +  ( (mult_32_16_32(b, c)) ) ));                                              \
    } while (0)

#endif //#if  (ALG_LC3_ENABLE)

#endif
