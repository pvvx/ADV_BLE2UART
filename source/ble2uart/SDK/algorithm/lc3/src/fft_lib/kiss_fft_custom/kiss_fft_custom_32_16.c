
#include "kiss_fft_type.h"
#include "kiss_fft_function.h"
#include "kiss_fft_fixed_op.h"

#if (ALG_LC3_ENABLE)

#include <stdlib.h>
#include <stdio.h>
#if __riscv
#include <nds_intrinsic.h>
#endif
static void kf_bfly2(
    kiss_fft_cpx_data_32* Fout,
    const int fstride,
    const kiss_fft_state16_ptr st,
    int m,
    int N,
    int mm
)
{
    kiss_fft_cpx_data_32* Fout2;
    kiss_fft_cpx_twiddle_16* tw1;
//    kiss_fft_cpx_data_32 t;
    if (!st->inverse) {
        int i, j;
        kiss_fft_cpx_data_32* Fout_beg = Fout;
        int tr, ti;
        for (i = 0; i < N; i++)
        {
            Fout = Fout_beg + i * mm;
            Fout2 = Fout + m;
            tw1 = st->twiddles; //Q15

            {
#if __riscv
                tr = (SUB32(__nds__kmmwb2_u(Fout2->r, tw1->r), __nds__kmmwb2_u(Fout2->i, tw1->i)));
                ti = (__nds__kmmwb2_u(Fout2->i, tw1->r) + __nds__kmmwb2_u(Fout2->r, tw1->i));
                //__nds__kmmawb2_u
                Fout2->r = (__nds__rsubw(((Fout->r)), tr));
                Fout2->i = (__nds__rsubw(((Fout->i)), ti));
                Fout->r = (__nds__raddw(((Fout->r)), tr));
                Fout->i = (__nds__raddw(((Fout->i)), ti));
#else
                tr = Fout2->r;
                ti = Fout2->i;

                Fout2->r = SHR32(SUB32(((Fout->r)), tr), 1);
                Fout2->i = SHR32(SUB32(((Fout->i)), ti), 1);
                Fout->r = SHR32(ADD32(((Fout->r)), tr), 1);
                Fout->i = SHR32(ADD32(((Fout->i)), ti), 1);
#endif
                ++Fout2;
                ++Fout;
                tw1 += fstride;

            }
            for (j = 1; j < m; j++)
            {
#if __riscv
                tr = (SUB32(__nds__kmmwb2_u(Fout2->r, tw1->r), __nds__kmmwb2_u(Fout2->i, tw1->i)));
                ti = (__nds__kmmwb2_u(Fout2->i, tw1->r) + __nds__kmmwb2_u(Fout2->r, tw1->i));

                Fout2->r = (__nds__rsubw(((Fout->r)), tr));
                Fout2->i = (__nds__rsubw(((Fout->i)), ti));
                Fout->r = (__nds__raddw(((Fout->r)), tr));
                Fout->i = (__nds__raddw(((Fout->i)), ti));
#else
                tr = (SUB32(MULT32_16_P15(Fout2->r, tw1->r), MULT32_16_P15(Fout2->i, tw1->i)));
                ti = (ADD32(MULT32_16_P15(Fout2->i, tw1->r), MULT32_16_P15(Fout2->r, tw1->i)));

                Fout2->r = SHR32(SUB32(((Fout->r)), tr), 1);
                Fout2->i = SHR32(SUB32(((Fout->i)), ti), 1);
                Fout->r = SHR32(ADD32(((Fout->r)), tr), 1);
                Fout->i = SHR32(ADD32(((Fout->i)), ti), 1);
#endif
                ++Fout2;
                ++Fout;
                tw1 += fstride;

            }
        }
    }

}

static void kf_bfly4(
    kiss_fft_cpx_data_32* Fout,
    const int fstride,
    const kiss_fft_state16_ptr st,
    int m,
    int N,
    int mm
)
{
    kiss_fft_cpx_twiddle_16* tw1, * tw2, * tw3;
    kiss_fft_cpx_data_32 scratch[6];
    const int m2 = 2 * m;
    const int m3 = 3 * m;
    int i, j;

    {
        kiss_fft_cpx_data_32* Fout_beg = Fout;
        for (i = 0; i < N; i++)
        {
            Fout = Fout_beg + i * mm;
            tw3 = tw2 = tw1 = st->twiddles;

            {
#if __riscv
                /* mult with W_kn */
                scratch[0].r = Fout[m].r;
                scratch[0].i = Fout[m].i;
                scratch[1].r = Fout[m2].r;
                scratch[1].i = Fout[m2].i;
                scratch[2].r = Fout[m3].r;
                scratch[2].i = Fout[m3].i;

                /* fft 4 */
                scratch[3].r = __nds__rsubw(Fout[0].r, scratch[1].r);//a=x0 - x2
                scratch[3].i = __nds__rsubw(Fout[0].i, scratch[1].i);
                Fout[0].r = __nds__raddw(Fout[0].r, scratch[1].r);//b=x0 + x2
                Fout[0].i = __nds__raddw(Fout[0].i, scratch[1].i);
                scratch[4].r = __nds__raddw(scratch[0].r, scratch[2].r);//c=x1 + x3
                scratch[4].i = __nds__raddw(scratch[0].i, scratch[2].i);
                scratch[5].r = __nds__rsubw(scratch[0].r, scratch[2].r);//d=x1 - x3
                scratch[5].i = __nds__rsubw(scratch[0].i, scratch[2].i);

                Fout[m2].r = __nds__rsubw(Fout[0].r, scratch[4].r);// fft2 = b - c
                Fout[m2].i = __nds__rsubw(Fout[0].i, scratch[4].i);
                Fout[0].r = __nds__raddw(Fout[0].r, scratch[4].r);// fft0 = b + c
                Fout[0].i = __nds__raddw(Fout[0].i, scratch[4].i);

                Fout[m].r = __nds__raddw(scratch[3].r, scratch[5].i); //fft1 = a - j(d)
                Fout[m].i = __nds__rsubw(scratch[3].i, scratch[5].r);
                Fout[m3].r = __nds__rsubw(scratch[3].r, scratch[5].i);//fft3 = a + j(d)
                Fout[m3].i = __nds__raddw(scratch[3].i, scratch[5].r);

                tw1 += fstride;
                tw2 += fstride * 2;
                tw3 += fstride * 3;
                ++Fout;
#else
                /* mult with W_kn */
                scratch[0].r = Fout[m].r;
                scratch[0].i = Fout[m].i;
                scratch[1].r = Fout[m2].r;
                scratch[1].i = Fout[m2].i;
                scratch[2].r = Fout[m3].r;
                scratch[2].i = Fout[m3].i;

                /* fft 4 */
                scratch[3].r = ((long long int)Fout[0].r - scratch[1].r) >> 1;//a=x0 - x2
                scratch[3].i = ((long long int)Fout[0].i - scratch[1].i) >> 1;
                Fout[0].r = ((long long int)Fout[0].r + scratch[1].r) >> 1;//b=x0 + x2
                Fout[0].i = ((long long int)Fout[0].i + scratch[1].i) >> 1;
                scratch[4].r = ((long long int)scratch[0].r + scratch[2].r) >> 1;//c=x1 + x3
                scratch[4].i = ((long long int)scratch[0].i + scratch[2].i) >> 1;
                scratch[5].r = ((long long int)scratch[0].r - scratch[2].r) >> 1;//d=x1 - x3
                scratch[5].i = ((long long int)scratch[0].i - scratch[2].i) >> 1;

                Fout[m2].r = ((long long int)Fout[0].r - scratch[4].r) >> 1;// fft2 = b - c
                Fout[m2].i = ((long long int)Fout[0].i - scratch[4].i) >> 1;
                Fout[0].r = ((long long int)Fout[0].r + scratch[4].r) >> 1;// fft0 = b + c
                Fout[0].i = ((long long int)Fout[0].i + scratch[4].i) >> 1;

                Fout[m].r = ((long long int)scratch[3].r + scratch[5].i) >> 1; //fft1 = a - j(d)
                Fout[m].i = ((long long int)scratch[3].i - scratch[5].r) >> 1;
                Fout[m3].r = ((long long int)scratch[3].r - scratch[5].i) >> 1;//fft3 = a + j(d)
                Fout[m3].i = ((long long int)scratch[3].i + scratch[5].r) >> 1;

                tw1 += fstride;
                tw2 += fstride * 2;
                tw3 += fstride * 3;
                ++Fout;
                //printf("%d,%d,\n", Fout[0].r, Fout[0].i);
#endif
            }
            for (j = 1; j < m; j++)
            {
#if __riscv
                /* mult with W_kn */
                //scratch[0].r = __nds__kmmwb2_u(Fout[m].r, (*tw1).r) - __nds__kmmwb2_u(Fout[m].i, (*tw1).i);
                //scratch[0].i = __nds__kmmawb2_u(__nds__kmmwb2_u(Fout[m].r, (*tw1).i), Fout[m].i, (*tw1).r);
                //scratch[1].r = __nds__kmmwb2_u(Fout[m2].r, (*tw2).r) - __nds__kmmwb2_u(Fout[m2].i, (*tw2).i);
                //scratch[1].i = __nds__kmmawb2_u(__nds__kmmwb2_u(Fout[m2].r, (*tw2).i), Fout[m2].i, (*tw2).r);
                //scratch[2].r = __nds__kmmwb2_u(Fout[m3].r, (*tw3).r) - __nds__kmmwb2_u(Fout[m3].i, (*tw3).i);
                //scratch[2].i = __nds__kmmawb2_u(__nds__kmmwb2_u(Fout[m3].r, (*tw3).i), Fout[m3].i, (*tw3).r);
                C_MUL_32_16(scratch[0], Fout[m], (*tw1));
                C_MUL_32_16(scratch[1], Fout[m2], (*tw2));
                C_MUL_32_16(scratch[2], Fout[m3], (*tw3));

                /* fft 4 */
                scratch[3].r = __nds__rsubw(Fout[0].r, scratch[1].r);//a=x0 - x2
                scratch[3].i = __nds__rsubw(Fout[0].i, scratch[1].i);
                Fout[0].r = __nds__raddw(Fout[0].r, scratch[1].r);//b=x0 + x2
                Fout[0].i = __nds__raddw(Fout[0].i, scratch[1].i);
                scratch[4].r = __nds__raddw(scratch[0].r, scratch[2].r);//c=x1 + x3
                scratch[4].i = __nds__raddw(scratch[0].i, scratch[2].i);
                scratch[5].r = __nds__rsubw(scratch[0].r, scratch[2].r);//d=x1 - x3
                scratch[5].i = __nds__rsubw(scratch[0].i, scratch[2].i);

                Fout[m2].r = __nds__rsubw(Fout[0].r, scratch[4].r);// fft2 = b - c
                Fout[m2].i = __nds__rsubw(Fout[0].i, scratch[4].i);
                Fout[0].r = __nds__raddw(Fout[0].r, scratch[4].r);// fft0 = b + c
                Fout[0].i = __nds__raddw(Fout[0].i, scratch[4].i);

                Fout[m].r = __nds__raddw(scratch[3].r, scratch[5].i); //fft1 = a - j(d)
                Fout[m].i = __nds__rsubw(scratch[3].i, scratch[5].r);
                Fout[m3].r = __nds__rsubw(scratch[3].r, scratch[5].i);//fft3 = a + j(d)
                Fout[m3].i = __nds__raddw(scratch[3].i, scratch[5].r);

                tw1 += fstride;
                tw2 += fstride * 2;
                tw3 += fstride * 3;
                ++Fout;
#else
                /* mult with W_kn */
                scratch[0].r = MULT32_16_P15(Fout[m].r, (*tw1).r) - MULT32_16_P15(Fout[m].i, (*tw1).i);
                scratch[0].i = MULT32_16_P15(Fout[m].r, (*tw1).i) + MULT32_16_P15(Fout[m].i, (*tw1).r);
                scratch[1].r = MULT32_16_P15(Fout[m2].r, (*tw2).r) - MULT32_16_P15(Fout[m2].i, (*tw2).i);
                scratch[1].i = MULT32_16_P15(Fout[m2].r, (*tw2).i) + MULT32_16_P15(Fout[m2].i, (*tw2).r);
                scratch[2].r = MULT32_16_P15(Fout[m3].r, (*tw3).r) - MULT32_16_P15(Fout[m3].i, (*tw3).i);
                scratch[2].i = MULT32_16_P15(Fout[m3].r, (*tw3).i) + MULT32_16_P15(Fout[m3].i, (*tw3).r);

                /* fft 4 */
                scratch[3].r = ((long long int)Fout[0].r - scratch[1].r) >> 1;//a=x0 - x2
                scratch[3].i = ((long long int)Fout[0].i - scratch[1].i) >> 1;
                Fout[0].r = ((long long int)Fout[0].r + scratch[1].r) >> 1;//b=x0 + x2
                Fout[0].i = ((long long int)Fout[0].i + scratch[1].i) >> 1;
                scratch[4].r = ((long long int)scratch[0].r + scratch[2].r) >> 1;//c=x1 + x3
                scratch[4].i = ((long long int)scratch[0].i + scratch[2].i) >> 1;
                scratch[5].r = ((long long int)scratch[0].r - scratch[2].r) >> 1;//d=x1 - x3
                scratch[5].i = ((long long int)scratch[0].i - scratch[2].i) >> 1;

                Fout[m2].r = ((long long int)Fout[0].r - scratch[4].r) >> 1;// fft2 = b - c
                Fout[m2].i = ((long long int)Fout[0].i - scratch[4].i) >> 1;
                Fout[0].r = ((long long int)Fout[0].r + scratch[4].r) >> 1;// fft0 = b + c
                Fout[0].i = ((long long int)Fout[0].i + scratch[4].i) >> 1;

                Fout[m].r = ((long long int)scratch[3].r + scratch[5].i) >> 1; //fft1 = a - j(d)
                Fout[m].i = ((long long int)scratch[3].i - scratch[5].r) >> 1;
                Fout[m3].r = ((long long int)scratch[3].r - scratch[5].i) >> 1;//fft3 = a + j(d)
                Fout[m3].i = ((long long int)scratch[3].i + scratch[5].r) >> 1;

                tw1 += fstride;
                tw2 += fstride * 2;
                tw3 += fstride * 3;
                ++Fout;
                //printf("%d,%d,\n", Fout[0].r, Fout[0].i);
#endif
            }
        }
    }
}

static void kf_bfly3(
    kiss_fft_cpx_data_32* Fout,
    const int fstride,
    const kiss_fft_state16_ptr st,
    int m
)
{
    int k = m;
    const int m2 = 2 * m;
    kiss_fft_cpx_twiddle_16* tw1, * tw2;
    kiss_fft_cpx_data_32 scratch[5];
    kiss_fft_cpx_twiddle_16 epi3;
    epi3 = st->twiddles[fstride * m];

    tw1 = tw2 = st->twiddles;

    {


#if __riscv
        scratch[1].r = Fout[m].r;
        scratch[1].i = Fout[m].i;
        scratch[2].r = Fout[m2].r;
        scratch[2].i = Fout[m2].i;

        scratch[3].r = __nds__raddw(scratch[1].r, scratch[2].r);
        scratch[3].i = __nds__raddw(scratch[1].i, scratch[2].i);
        scratch[0].r = __nds__rsubw(scratch[1].r, scratch[2].r);
        scratch[0].i = __nds__rsubw(scratch[1].i, scratch[2].i);



        Fout[m].r = __nds__rsubw(Fout->r, (scratch[3].r));
        Fout[m].i = __nds__rsubw(Fout->i, (scratch[3].i));

        Fout[0].r = Fout[0].r / 2;
        Fout[0].i = Fout[0].i / 2;
        C_MULBYSCALAR_32(scratch[0], epi3.i);

        Fout[0].r = __nds__raddw(Fout[0].r, scratch[3].r);
        Fout[0].i = __nds__raddw(Fout[0].i, scratch[3].i);

        Fout[m2].r = __nds__raddw(Fout[m].r, scratch[0].i);
        Fout[m2].i = __nds__rsubw(Fout[m].i, scratch[0].r);

        Fout[m].r = __nds__rsubw(Fout[m].r, scratch[0].i);
        Fout[m].i = __nds__raddw(Fout[m].i, scratch[0].r);

        tw1 += fstride;
        tw2 += fstride * 2;
        ++Fout;
        k--;
#else
        scratch[1].r = Fout[m].r;
        scratch[1].i = Fout[m].i;
        scratch[2].r = Fout[m2].r;
        scratch[2].i = Fout[m2].i;

        scratch[3].r = ((long long int)scratch[1].r + scratch[2].r) >> 1;
        scratch[3].i = ((long long int)scratch[1].i + scratch[2].i) >> 1;
        scratch[0].r = ((long long int)scratch[1].r - scratch[2].r) >> 1;
        scratch[0].i = ((long long int)scratch[1].i - scratch[2].i) >> 1;



        Fout[m].r = ((long long int)Fout->r - (scratch[3].r)) >> 1;
        Fout[m].i = ((long long int)Fout->i - (scratch[3].i)) >> 1;

        Fout[0].r = Fout[0].r / 2;
        Fout[0].i = Fout[0].i / 2;
        C_MULBYSCALAR_32(scratch[0], epi3.i);

        Fout[0].r = ((long long int)Fout[0].r + scratch[3].r) >> 1;
        Fout[0].i = ((long long int)Fout[0].i + scratch[3].i) >> 1;

        Fout[m2].r = ((long long int)Fout[m].r + scratch[0].i) >> 1;
        Fout[m2].i = ((long long int)Fout[m].i - scratch[0].r) >> 1;

        Fout[m].r = ((long long int)Fout[m].r - scratch[0].i) >> 1;
        Fout[m].i = ((long long int)Fout[m].i + scratch[0].r) >> 1;

        tw1 += fstride;
        tw2 += fstride * 2;
        ++Fout;
        k--;
#endif
    }

    for (; k > 0; k--)
    {

        //if (!st->inverse) {
           // C_FIXDIV(*Fout, 3); C_FIXDIV(Fout[m], 3); C_FIXDIV(Fout[m2], 3);
        //}
        /* X(k) = x1(k)+W_kn*x2(k)+W_2kn*x3(k) */
        /* X(k+N/3) = x1(k)+W_13*W_kn*x2(k)+W_23*W_2kn*x3(k) */
        /* X(k+2N/3) = x1(k)+W_23*W_kn*x2(k)+W_13*W_2kn*x3(k) */
        /* W_13 = cos(2/3*pi) -j*sin(2/3*pi) = -1/2 - sqrt(3)/2 */
        /* W_23 = cos(4/3*pi) -j*sin(4/3*pi) = -1/2 + sqrt(3)/2 */
#if __riscv
        //scratch[1].r = __nds__kmmwb2_u(Fout[m].r, (*tw1).r) - __nds__kmmwb2_u(Fout[m].i, (*tw1).i);
        //scratch[1].i = __nds__kmmawb2_u(__nds__kmmwb2_u(Fout[m].r, (*tw1).i), Fout[m].i, (*tw1).r);
        //scratch[2].r = __nds__kmmwb2_u(Fout[m2].r, (*tw2).r) - __nds__kmmwb2_u(Fout[m2].i, (*tw2).i);
        //scratch[2].i = __nds__kmmawb2_u(__nds__kmmwb2_u(Fout[m2].r, (*tw2).i), Fout[m2].i, (*tw2).r);
        C_MUL_32_16(scratch[1], Fout[m], (*tw1));
        C_MUL_32_16(scratch[2], Fout[m2], (*tw2));

        scratch[3].r = __nds__raddw(scratch[1].r, scratch[2].r);
        scratch[3].i = __nds__raddw(scratch[1].i, scratch[2].i);
        scratch[0].r = __nds__rsubw(scratch[1].r, scratch[2].r);
        scratch[0].i = __nds__rsubw(scratch[1].i, scratch[2].i);



        Fout[m].r = __nds__rsubw(Fout->r, (scratch[3].r));
        Fout[m].i = __nds__rsubw(Fout->i, (scratch[3].i));

        Fout[0].r = Fout[0].r / 2;
        Fout[0].i = Fout[0].i / 2;
        C_MULBYSCALAR_32(scratch[0], epi3.i);

        Fout[0].r = __nds__raddw(Fout[0].r, scratch[3].r);
        Fout[0].i = __nds__raddw(Fout[0].i, scratch[3].i);

        Fout[m2].r = __nds__raddw(Fout[m].r, scratch[0].i);
        Fout[m2].i = __nds__rsubw(Fout[m].i, scratch[0].r);

        Fout[m].r = __nds__rsubw(Fout[m].r, scratch[0].i);
        Fout[m].i = __nds__raddw(Fout[m].i, scratch[0].r);

        tw1 += fstride;
        tw2 += fstride * 2;
        ++Fout;
#else
        scratch[1].r = MULT32_16_P15(Fout[m].r, (*tw1).r) - MULT32_16_P15(Fout[m].i, (*tw1).i);
        scratch[1].i = MULT32_16_P15(Fout[m].r, (*tw1).i) + MULT32_16_P15(Fout[m].i, (*tw1).r);
        scratch[2].r = MULT32_16_P15(Fout[m2].r, (*tw2).r) - MULT32_16_P15(Fout[m2].i, (*tw2).i);
        scratch[2].i = MULT32_16_P15(Fout[m2].r, (*tw2).i) + MULT32_16_P15(Fout[m2].i, (*tw2).r);

        scratch[3].r = ((long long int)scratch[1].r + scratch[2].r) >> 1;
        scratch[3].i = ((long long int)scratch[1].i + scratch[2].i) >> 1;
        scratch[0].r = ((long long int)scratch[1].r - scratch[2].r) >> 1;
        scratch[0].i = ((long long int)scratch[1].i - scratch[2].i) >> 1;



        Fout[m].r = ((long long int)Fout->r - (scratch[3].r)) >> 1;
        Fout[m].i = ((long long int)Fout->i - (scratch[3].i)) >> 1;

        Fout[0].r = Fout[0].r / 2;
        Fout[0].i = Fout[0].i / 2;

        C_MULBYSCALAR_32(scratch[0], epi3.i);

        Fout[0].r = ((long long int)Fout[0].r + scratch[3].r) >> 1;
        Fout[0].i = ((long long int)Fout[0].i + scratch[3].i) >> 1;

        Fout[m2].r = ((long long int)Fout[m].r + scratch[0].i) >> 1;
        Fout[m2].i = ((long long int)Fout[m].i - scratch[0].r) >> 1;

        Fout[m].r = ((long long int)Fout[m].r - scratch[0].i) >> 1;
        Fout[m].i = ((long long int)Fout[m].i + scratch[0].r) >> 1;

        tw1 += fstride;
        tw2 += fstride * 2;
        ++Fout;
#endif
    };

}

static void kf_bfly5(
    kiss_fft_cpx_data_32* Fout,
    const int fstride,
    const kiss_fft_state16_ptr st,
    int m
)
{
    kiss_fft_cpx_data_32* Fout0, * Fout1, * Fout2, * Fout3, * Fout4;
    int u;
    kiss_fft_cpx_data_32 scratch[13];
    kiss_fft_cpx_twiddle_16* twiddles = st->twiddles;
    kiss_fft_cpx_twiddle_16* tw;
    kiss_fft_cpx_twiddle_16 ya, yb;
    ya = twiddles[fstride * m];
    yb = twiddles[fstride * 2 * m];

    Fout0 = Fout;
    Fout1 = Fout0 + m;
    Fout2 = Fout0 + 2 * m;
    Fout3 = Fout0 + 3 * m;
    Fout4 = Fout0 + 4 * m;

    tw = st->twiddles;
    int a, b, c, d;
    {
#if __riscv
        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;

        scratch[0] = *Fout0; // x0
        scratch[1] = *Fout1;
        scratch[2] = *Fout2;
        scratch[3] = *Fout3;
        scratch[4] = *Fout4;

        C_ADD_HALF(scratch[7], scratch[1], scratch[4]);// a=x1 + x4
        C_SUB_HALF(scratch[10], scratch[1], scratch[4]);// b=x1-x4
        C_ADD_HALF(scratch[8], scratch[2], scratch[3]);// c=x2 + x3
        C_SUB_HALF(scratch[9], scratch[2], scratch[3]);//d=x2 - x3

        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        a = __nds__raddw(scratch[7].r, scratch[8].r);
        b = __nds__raddw(scratch[7].i, scratch[8].i);
        Fout0->r = __nds__raddw(Fout0->r, a);//  x0 + a + b
        Fout0->i = __nds__raddw(Fout0->i, b);

        //scratch[0].r = scratch[0].r >> 1;
        //scratch[0].i = scratch[0].i >> 1;
        a = S_MUL32(scratch[7].r, ya.r);
        b = S_MUL32(scratch[8].r, yb.r);
        c = S_MUL32(scratch[7].i, ya.r);
        d = S_MUL32(scratch[8].i, yb.r);
        scratch[5].r = ((scratch[0].r >> 1) + __nds__raddw(a, b));// x0 + a
        scratch[5].i = ((scratch[0].i >> 1) + __nds__raddw(c, d));
        a = S_MUL32(scratch[10].i, ya.i);
        b = S_MUL32(scratch[9].i, yb.i);
        c = S_MUL32(scratch[10].r, ya.i);
        d = S_MUL32(scratch[9].r, yb.i);
        scratch[6].r = __nds__raddw(a, b);
        scratch[6].i = -__nds__raddw(c, d);

        C_SUB_HALF(*Fout1, scratch[5], scratch[6]);
        C_ADD_HALF(*Fout4, scratch[5], scratch[6]);

        a = S_MUL32(scratch[7].r, yb.r);
        b = S_MUL32(scratch[8].r, ya.r);
        c = S_MUL32(scratch[7].i, yb.r);
        d = S_MUL32(scratch[8].i, ya.r);
        int tmp1 = (scratch[0].r) >> 1;
        int tmp2 = (scratch[0].i) >> 1;
        scratch[11].r = (tmp1 + __nds__raddw(a, b));
        scratch[11].i = (tmp2 + __nds__raddw(c, d));
        a = S_MUL32(scratch[10].i, yb.i);
        b = S_MUL32(scratch[9].i, ya.i);
        c = S_MUL32(scratch[10].r, yb.i);
        d = S_MUL32(scratch[9].r, ya.i);
        scratch[12].r = __nds__rsubw(b, a);
        scratch[12].i = __nds__rsubw(c, d);

        C_ADD_HALF(*Fout2, scratch[11], scratch[12]);
        C_SUB_HALF(*Fout3, scratch[11], scratch[12]);

        ++Fout0; ++Fout1; ++Fout2; ++Fout3; ++Fout4;
#else

        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        scratch[0] = *Fout0; // x0
        scratch[1] = *Fout1;
        scratch[2] = *Fout2;
        scratch[3] = *Fout3;
        scratch[4] = *Fout4;

        C_ADD_HALF(scratch[7], scratch[1], scratch[4]);// a=x1 + x4
        C_SUB_HALF(scratch[10], scratch[1], scratch[4]);// b=x1-x4
        C_ADD_HALF(scratch[8], scratch[2], scratch[3]);// c=x2 + x3
        C_SUB_HALF(scratch[9], scratch[2], scratch[3]);//d=x2 - x3

        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        a = ((long long int)scratch[7].r + scratch[8].r) >> 1;
        b = ((long long int)scratch[7].i + scratch[8].i) >> 1;
        Fout0->r = ((long long int)Fout0->r + a) >> 1;//  x0 + a + b
        Fout0->i = ((long long int)Fout0->i + b) >> 1;

        //scratch[0].r = scratch[0].r >> 1;
        //scratch[0].i = scratch[0].i >> 1;
        a = S_MUL32(scratch[7].r, ya.r);
        b = S_MUL32(scratch[8].r, yb.r);
        c = S_MUL32(scratch[7].i, ya.r);
        d = S_MUL32(scratch[8].i, yb.r);
        scratch[5].r = ((scratch[0].r >> 1) + (((long long int)a + b) >> 1));// x0 + a
        scratch[5].i = ((scratch[0].i >> 1) + (((long long int)c + d) >> 1));
        a = S_MUL32(scratch[10].i, ya.i);
        b = S_MUL32(scratch[9].i, yb.i);
        c = S_MUL32(scratch[10].r, ya.i);
        d = S_MUL32(scratch[9].r, yb.i);
        scratch[6].r = ((long long int)a + b) >> 1;
        scratch[6].i = -(((long long int)c + d) >> 1);

        C_SUB_HALF(*Fout1, scratch[5], scratch[6]);
        C_ADD_HALF(*Fout4, scratch[5], scratch[6]);

        a = S_MUL32(scratch[7].r, yb.r);
        b = S_MUL32(scratch[8].r, ya.r);
        c = S_MUL32(scratch[7].i, yb.r);
        d = S_MUL32(scratch[8].i, ya.r);
        int tmp1 = (scratch[0].r) >> 1;
        int tmp2 = (scratch[0].i) >> 1;
        scratch[11].r = (tmp1 + (((long long int)a + b) >> 1));
        scratch[11].i = (tmp2 + (((long long int)c + d) >> 1));
        a = S_MUL32(scratch[10].i, yb.i);
        b = S_MUL32(scratch[9].i, ya.i);
        c = S_MUL32(scratch[10].r, yb.i);
        d = S_MUL32(scratch[9].r, ya.i);
        scratch[12].r = (-(long long int)a + b) >> 1;
        scratch[12].i = ((long long int)c - d) >> 1;

        C_ADD_HALF(*Fout2, scratch[11], scratch[12]);
        C_SUB_HALF(*Fout3, scratch[11], scratch[12]);

        ++Fout0; ++Fout1; ++Fout2; ++Fout3; ++Fout4;
#endif
    }
    for (u = 1; u < m; ++u) {
#if __riscv
        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        scratch[0] = *Fout0; // x0

        C_MUL_32_16(scratch[1], *Fout1, tw[u * fstride]);
        C_MUL_32_16(scratch[2], *Fout2, tw[2 * u * fstride]);
        C_MUL_32_16(scratch[3], *Fout3, tw[3 * u * fstride]);
        C_MUL_32_16(scratch[4], *Fout4, tw[4 * u * fstride]);

        C_ADD_HALF(scratch[7], scratch[1], scratch[4]);// a=x1 + x4
        C_SUB_HALF(scratch[10], scratch[1], scratch[4]);// b=x1-x4
        C_ADD_HALF(scratch[8], scratch[2], scratch[3]);// c=x2 + x3
        C_SUB_HALF(scratch[9], scratch[2], scratch[3]);//d=x2 - x3

        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        a = __nds__raddw(scratch[7].r, scratch[8].r);
        b = __nds__raddw(scratch[7].i, scratch[8].i);
        Fout0->r = __nds__raddw(Fout0->r, a);//  x0 + a + b
        Fout0->i = __nds__raddw(Fout0->i, b);

        //scratch[0].r = scratch[0].r >> 1;
        //scratch[0].i = scratch[0].i >> 1;
        a = S_MUL32(scratch[7].r, ya.r);
        b = S_MUL32(scratch[8].r, yb.r);
        c = S_MUL32(scratch[7].i, ya.r);
        d = S_MUL32(scratch[8].i, yb.r);
        scratch[5].r = ((scratch[0].r >> 1) + __nds__raddw(a, b));// x0 + a
        scratch[5].i = ((scratch[0].i >> 1) + __nds__raddw(c, d));
        a = S_MUL32(scratch[10].i, ya.i);
        b = S_MUL32(scratch[9].i, yb.i);
        c = S_MUL32(scratch[10].r, ya.i);
        d = S_MUL32(scratch[9].r, yb.i);
        scratch[6].r = __nds__raddw(a, b);
        scratch[6].i = -__nds__raddw(c, d);

        C_SUB_HALF(*Fout1, scratch[5], scratch[6]);
        C_ADD_HALF(*Fout4, scratch[5], scratch[6]);

        a = S_MUL32(scratch[7].r, yb.r);
        b = S_MUL32(scratch[8].r, ya.r);
        c = S_MUL32(scratch[7].i, yb.r);
        d = S_MUL32(scratch[8].i, ya.r);
        int tmp1 = (scratch[0].r) >> 1;
        int tmp2 = (scratch[0].i) >> 1;
        scratch[11].r = (tmp1 + __nds__raddw(a, b));
        scratch[11].i = (tmp2 + __nds__raddw(c, d));
        a = S_MUL32(scratch[10].i, yb.i);
        b = S_MUL32(scratch[9].i, ya.i);
        c = S_MUL32(scratch[10].r, yb.i);
        d = S_MUL32(scratch[9].r, ya.i);
        scratch[12].r = __nds__rsubw(b, a);
        scratch[12].i = __nds__rsubw(c, d);

        C_ADD_HALF(*Fout2, scratch[11], scratch[12]);
        C_SUB_HALF(*Fout3, scratch[11], scratch[12]);

        ++Fout0; ++Fout1; ++Fout2; ++Fout3; ++Fout4;
#else

        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        scratch[0] = *Fout0; // x0

        C_MUL_32_16(scratch[1], *Fout1, tw[u * fstride]);
        C_MUL_32_16(scratch[2], *Fout2, tw[2 * u * fstride]);
        C_MUL_32_16(scratch[3], *Fout3, tw[3 * u * fstride]);
        C_MUL_32_16(scratch[4], *Fout4, tw[4 * u * fstride]);

        C_ADD_HALF(scratch[7], scratch[1], scratch[4]);// a=x1 + x4
        C_SUB_HALF(scratch[10], scratch[1], scratch[4]);// b=x1-x4
        C_ADD_HALF(scratch[8], scratch[2], scratch[3]);// c=x2 + x3
        C_SUB_HALF(scratch[9], scratch[2], scratch[3]);//d=x2 - x3

        Fout0->r = Fout0->r >> 1;
        Fout0->i = Fout0->i >> 1;
        a = ((long long int)scratch[7].r + scratch[8].r) >> 1;
        b = ((long long int)scratch[7].i + scratch[8].i) >> 1;
        Fout0->r = ((long long int)Fout0->r + a) >> 1;//  x0 + a + b
        Fout0->i = ((long long int)Fout0->i + b) >> 1;

        //scratch[0].r = scratch[0].r >> 1;
        //scratch[0].i = scratch[0].i >> 1;
        a = S_MUL32(scratch[7].r, ya.r);
        b = S_MUL32(scratch[8].r, yb.r);
        c = S_MUL32(scratch[7].i, ya.r);
        d = S_MUL32(scratch[8].i, yb.r);
        scratch[5].r = ((scratch[0].r >> 1) + (((long long int)a + b) >> 1));// x0 + a
        scratch[5].i = ((scratch[0].i >> 1) + (((long long int)c + d) >> 1));
        a = S_MUL32(scratch[10].i, ya.i);
        b = S_MUL32(scratch[9].i, yb.i);
        c = S_MUL32(scratch[10].r, ya.i);
        d = S_MUL32(scratch[9].r, yb.i);
        scratch[6].r = ((long long int)a + b) >> 1;
        scratch[6].i = -(((long long int)c + d) >> 1);

        C_SUB_HALF(*Fout1, scratch[5], scratch[6]);
        C_ADD_HALF(*Fout4, scratch[5], scratch[6]);

        a = S_MUL32(scratch[7].r, yb.r);
        b = S_MUL32(scratch[8].r, ya.r);
        c = S_MUL32(scratch[7].i, yb.r);
        d = S_MUL32(scratch[8].i, ya.r);
        int tmp1 = (scratch[0].r) >> 1;
        int tmp2 = (scratch[0].i) >> 1;
        scratch[11].r = (tmp1 + (((long long int)a + b) >> 1));
        scratch[11].i = (tmp2 + (((long long int)c + d) >> 1));
        a = S_MUL32(scratch[10].i, yb.i);
        b = S_MUL32(scratch[9].i, ya.i);
        c = S_MUL32(scratch[10].r, yb.i);
        d = S_MUL32(scratch[9].r, ya.i);
        scratch[12].r = (-(long long int)a + b) >> 1;
        scratch[12].i = ((long long int)c - d) >> 1;

        C_ADD_HALF(*Fout2, scratch[11], scratch[12]);
        C_SUB_HALF(*Fout3, scratch[11], scratch[12]);

        ++Fout0; ++Fout1; ++Fout2; ++Fout3; ++Fout4;
#endif
    }
}


static
void kf_shuffle(
    kiss_fft_cpx_data_32* Fout,
    const kiss_fft_cpx_data_32* f,
    const int fstride,
    int in_stride,
    int* factors,
    const kiss_fft_state16_ptr st
)
{
    const int p = *factors++; /* the radix  */
    const int m = *factors++; /* stage's fft length/p */

    // printf ("fft %d %d %d %d\n", p*m, m, p, fstride*in_stride);
    if (m == 1)
    {
        int j;
        for (j = 0; j < p; j++)
        {
            Fout[j] = *f;
            f += fstride * in_stride;
        }
    }
    else {
        int j;
        for (j = 0; j < p; j++)
        {
            kf_shuffle(Fout, f, fstride * p, in_stride, factors, st);
            f += fstride * in_stride;
            Fout += m;
        }
    }
}

static
void kf_work(
    kiss_fft_cpx_data_32* Fout,
    const kiss_fft_cpx_data_32* f,
    const int fstride,
    int in_stride,
    int* factors,
    const kiss_fft_state16_ptr st,
    int N,
    int s2,
    int m2
)
{
    int i;
    kiss_fft_cpx_data_32* Fout_beg = Fout;
    const int p = *factors++; /* the radix  */
    const int m = *factors++; /* stage's fft length/p */
#if 0
    /*printf ("fft %d %d %d %d %d %d\n", p*m, m, p, s2, fstride*in_stride, N);*/
    if (m == 1)
    {
        /*   int j;
           for (j=0;j<p;j++)
           {
              Fout[j] = *f;
              f += fstride*in_stride;
           }*/
    }
    else {
        int j;
        for (j = 0; j < p; j++)
        {
            kf_work(Fout, f, fstride * p, in_stride, factors, st, N * p, fstride * in_stride, m);
            f += fstride * in_stride;
            Fout += m;
        }
    }

    Fout = Fout_beg;

    switch (p) {
    case 2: kf_bfly2(Fout, fstride, st, m); break;
    case 3: kf_bfly3(Fout, fstride, st, m); break;
    case 4: kf_bfly4(Fout, fstride, st, m); break;
    case 5: kf_bfly5(Fout, fstride, st, m); break;
    default: kf_bfly_generic(Fout, fstride, st, m, p); break;
    }
#else
    /*printf ("fft %d %d %d %d %d %d %d\n", p*m, m, p, s2, fstride*in_stride, N, m2);*/
    if (m == 1)
    {
        /*for (i=0;i<N;i++)
        {
           int j;
           Fout = Fout_beg+i*m2;
           const kiss_fft_cpx_data_32 * f2 = f+i*s2;
           for (j=0;j<p;j++)
           {
              *Fout++ = *f2;
              f2 += fstride*in_stride;
           }
        }*/
    }
    else {
        kf_work(Fout, f, fstride * p, in_stride, factors, st, N * p, fstride * in_stride, m);
    }




    switch (p) {
    case 2: kf_bfly2(Fout, fstride, st, m, N, m2); break;
    case 3: for (i = 0; i < N; i++) { Fout = Fout_beg + i * m2; kf_bfly3(Fout, fstride, st, m); } break;
    case 4: kf_bfly4(Fout, fstride, st, m, N, m2); break;
    case 5: for (i = 0; i < N; i++) { Fout = Fout_beg + i * m2; kf_bfly5(Fout, fstride, st, m); } break;
    default:  break;
    }
#endif
}

static
int kf_factor(int n, int* facbuf)
{
    int p = 4;
    int Q_shift = 0;
    /*factor out powers of 4, powers of 2, then any remaining primes */
    do {
        while (n % p) {
            switch (p) {
            case 4: p = 2; break;
            case 2: p = 3; break;
            default: p += 2; break;
            }
            if (p > 32000 || (int)p * (int)p > n)
                p = n;          /* no more factors, skip to end */
        }
        n /= p;
        if (p == 4)
        {
            Q_shift += 2;
        }
        else if (p == 2)
        {
            Q_shift += 1;
        }
        else if (p == 3)
        {
            Q_shift += 2;
        }
        else if (p == 5)
        {
            Q_shift += 3;
        }
        *facbuf++ = p;
        *facbuf++ = n;
    } while (n > 1);
    return Q_shift;
}


struct kiss_fft_state16 kiss_fft_st;
struct kiss_fft_state16 kiss_ifft_st;
kiss_fft_state16_ptr kiss_fft_init_32_16(int nfft, int inverse_fft)
{

    kiss_fft_state16_ptr st;
    if(inverse_fft)
        st = &(kiss_ifft_st);
    else
        st = &(kiss_fft_st);
    if (st) {
        int i;
        st->nfft = nfft;
        st->inverse = inverse_fft;

        for (i = 0; i < nfft; ++i) {
            int phase = i;
            if (!st->inverse)
                phase = -phase;
            kf_cexp2(st->twiddles + i, DIV32(SHL32(phase, 17), nfft));
        }

        st->Q_shift = kf_factor(nfft, st->factors);
    }
    return st;
}


__attribute__((section(".ram_code")))
void kiss_fft_32_16(kiss_fft_state16_ptr cfg, const kiss_fft_cpx_data_32* fin, kiss_fft_cpx_data_32* fout)
{
    if (fin == fout) {
        //NOTE: this is not really an in-place FFT algorithm.
        //It just performs an out-of-place FFT into a temp buffer
        kiss_fft_cpx_data_32 tmpbuf[NFFT];
        kf_shuffle(tmpbuf, fin, 1, 1, cfg->factors, cfg);
        kf_work(tmpbuf, fin, 1, 1, cfg->factors, cfg, 1, 1, 1);
        for (unsigned int i = 0; i < NFFT; i++)
        {
            fout[i].r = tmpbuf[i].r;
            fout[i].i = tmpbuf[i].i;
        }
        //memcpy(fout, tmpbuf, sizeof(kiss_fft_cpx_data_32) * NFFT);
    }
    else {
        kf_shuffle(fout, fin, 1, 1, cfg->factors, cfg);
        //int start = __nds__csrr(NDS_MCYCLE);
        kf_work(fout, fin, 1, 1, cfg->factors, cfg, 1, 1, 1);
        // int end = __nds__csrr(NDS_MCYCLE);
        // printf("fftcyc:%d \n",end-start);
    }
}

#endif //#if (ALG_LC3_ENABLE)
