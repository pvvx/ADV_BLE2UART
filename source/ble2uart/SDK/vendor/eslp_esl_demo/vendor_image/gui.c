/********************************************************************************************************
 * @file    gui.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    01,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#include "driver.h"
#include "tl_common.h"
//#include "epd.h"
#include "fonts.h"
#include "gui.h"
#include "helpers.h"

#define X_channel     16
#define Y_channel     15
#define X_MAX         296
#define Y_MAX         128

#define E_UNIT        8
#define T_UNIT        2

#define EPD_DATA_SIZE 4736

enum
{
    GUI_ERROR_CODE_NO_ERROR = 0,
    GUI_ERROR_CODE_OVERFLOW,
    GUI_ERROR_CODE_NOT_SUPPORT_CHAR,
};

typedef struct
{
    const unsigned char *font;
    unsigned char        data;
} Font_TypeDef;

typedef struct
{
    const u8       font_width;
    const u8       font_height;
    const Font_TypeDef *font_bitmap;
} Font_Style_t;

const Font_TypeDef font_8_16_data_tbl[] = {
    {FONT_8_16_NUM_0,      '0'     },
    {FONT_8_16_NUM_1,      '1'     },
    {FONT_8_16_NUM_2,      '2'     },
    {FONT_8_16_NUM_3,      '3'     },
    {FONT_8_16_NUM_4,      '4'     },
    {FONT_8_16_NUM_5,      '5'     },
    {FONT_8_16_NUM_6,      '6'     },
    {FONT_8_16_NUM_7,      '7'     },
    {FONT_8_16_NUM_8,      '8'     },
    {FONT_8_16_NUM_9,      '9'     },
    {FONT_8_16_CHAR_A,     'A'     },
    {FONT_8_16_CHAR_B,     'B'     },
    {FONT_8_16_CHAR_C,     'C'     },
    {FONT_8_16_CHAR_D,     'D'     },
    {FONT_8_16_CHAR_E,     'E'     },
    {FONT_8_16_CHAR_F,     'F'     },
    {FONT_8_16_CHAR_G,     'G'     },
    {FONT_8_16_CHAR_H,     'H'     },
    {FONT_8_16_CHAR_I,     'I'     },
    {FONT_8_16_CHAR_J,     'J'     },
    {FONT_8_16_CHAR_K,     'K'     },
    {FONT_8_16_CHAR_L,     'L'     },
    {FONT_8_16_CHAR_M,     'M'     },
    {FONT_8_16_CHAR_N,     'N'     },
    {FONT_8_16_CHAR_O,     'O'     },
    {FONT_8_16_CHAR_P,     'P'     },
    {FONT_8_16_CHAR_Q,     'Q'     },
    {FONT_8_16_CHAR_R,     'R'     },
    {FONT_8_16_CHAR_S,     'S'     },
    {FONT_8_16_CHAR_T,     'T'     },
    {FONT_8_16_CHAR_U,     'U'     },
    {FONT_8_16_CHAR_V,     'V'     },
    {FONT_8_16_CHAR_W,     'W'     },
    {FONT_8_16_CHAR_X,     'X'     },
    {FONT_8_16_CHAR_Y,     'Y'     },
    {FONT_8_16_CHAR_Z,     'Z'     },
    {FONT_8_16_CHAR_Ove,   '.'     },
    {FONT_8_16_CHAR_Space, ' '     },
    {FONT_8_16_SYM_SLASH,  '/'     },

    {FONT_8_16_CHAR_LC,    'c'     },
    {FONT_8_16_CHAR_LE,    'e'     },
    {FONT_8_16_CHAR_LG,    'g'     },
    {FONT_8_16_CHAR_LI,    'i'     },
    {FONT_8_16_CHAR_LK,    'k'     },
    {FONT_8_16_CHAR_LL,    'l'     },
    {FONT_8_16_CHAR_LN,    'n'     },
    {FONT_8_16_CHAR_LP,    'p'     },
    {FONT_8_16_CHAR_LU,    'u'     },
    {END_MARK,             END_MARK}
};

const Font_TypeDef font_16_32_data_tbl[] = {
    {FONT_16_32_NUM_0,     '0'     },
    {FONT_16_32_NUM_1,     '1'     },
    {FONT_16_32_NUM_2,     '2'     },
    {FONT_16_32_NUM_3,     '3'     },
    {FONT_16_32_NUM_4,     '4'     },
    {FONT_16_32_NUM_5,     '5'     },
    {FONT_16_32_NUM_6,     '6'     },
    {FONT_16_32_NUM_7,     '7'     },
    {FONT_16_32_NUM_8,     '8'     },
    {FONT_16_32_NUM_9,     '9'     },
    {FONT_16_32_SYM_EUR,   EUR_CHAR},
    {FONT_16_32_SYM_USD,   USD_CHAR},
    {FONT_16_32_SYM_CNY,   CNY_CHAR},
    {FONT_16_32_SYM_GBP,   GBP_CHAR},
    {FONT_16_32_SYM_DOT,   '.'     },
    {FONT_16_32_SYM_SLASH, '/'     },
    {END_MARK,             END_MARK}
};

const Font_TypeDef font_32_56_data_tbl[] = {
    {FONT_32_56_NUM_0,   '0'     },
    {FONT_32_56_NUM_1,   '1'     },
    {FONT_32_56_NUM_2,   '2'     },
    {FONT_32_56_NUM_3,   '3'     },
    {FONT_32_56_NUM_4,   '4'     },
    {FONT_32_56_NUM_5,   '5'     },
    {FONT_32_56_NUM_6,   '6'     },
    {FONT_32_56_NUM_7,   '7'     },
    {FONT_32_56_NUM_8,   '8'     },
    {FONT_32_56_NUM_9,   '9'     },
    {FONT_32_56_SYM_DOT, '.'     },
    {END_MARK,           END_MARK}
};

const Font_Style_t font_styles[] = {
    {.font_width = FONT_8_16_WIDTH,  .font_height = FONT_8_16_HEIGHT,  .font_bitmap = font_8_16_data_tbl },
    {.font_width = FONT_16_32_WIDTH, .font_height = FONT_16_32_HEIGHT, .font_bitmap = font_16_32_data_tbl},
    {.font_width = FONT_32_56_WIDTH, .font_height = FONT_32_56_HEIGHT, .font_bitmap = font_32_56_data_tbl}
};

static const unsigned char *GUI_GetFont(unsigned char data, FONT_STYLE_NAME_Typedef font_style)
{
    unsigned int i = 0;

    if (font_style >= FONT_MAX) {
        return NULL;
    }

    while (font_styles[font_style].font_bitmap[i].data != END_MARK) {
        if (data == font_styles[font_style].font_bitmap[i].data) {
            return font_styles[font_style].font_bitmap[i].font;
        }
        i++;
    }
    return NULL;
}

void GUI_BytesToHexStr(const char *bytes, int len, char *str)
{
    unsigned char tmp;
    int           i;
    for (i = 0; i < len; i++) {
        tmp = bytes[len - 1 - i] / 16;
        if (tmp < 10) {
            str[i * 2] = '0' + tmp;
        } else {
            str[i * 2] = 'A' + (tmp - 10);
        }

        tmp = bytes[len - 1 - i] % 16;
        if (tmp < 10) {
            str[i * 2 + 1] = '0' + tmp;
        } else {
            str[i * 2 + 1] = 'A' + (tmp - 10);
        }
    }
    str[2 * len] = '\0';
}

void GUI_IntToDecStr(unsigned int data, unsigned char *str)
{
    unsigned char tmp;
    int           i = 0;
    while (data) {
        tmp = data % 10;
        data /= 10;
        str[i++] = tmp + '0';
    }
    str[i] = '\0';

    int j = app_strlen((const char *)str) - 1;
    for (i = 0; i < j; i++, j--) {
        tmp    = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
}

unsigned char GUI_DispPic(unsigned char *image, int x, int y, const unsigned char *pic, unsigned char width, unsigned char height)
{
    int i, j, n_bit;

    n_bit = (height) / E_UNIT;

    //    if (y) {
    //        bit = (height + y) / E_UNIT;
    //    }
    //    else {
    //        bit = height / E_UNIT;
    //    }

    if (x > X_MAX || y > Y_MAX) {
        return GUI_ERROR_CODE_OVERFLOW;
    }

    for (i = 0; i < width; i++) {
        for (j = 0; j < n_bit; j++) {
            image[((x + i) * X_channel + Y_channel - y - j)] = pic[n_bit * (1 + i) - j - 1];
        }
    }

    return GUI_ERROR_CODE_NO_ERROR;
}

unsigned char GUI_DispChar(unsigned char *image, int x, int y, unsigned char data, FONT_STYLE_NAME_Typedef font_style)
{
    assert(image);

    u8 GUI_font_height = font_styles[font_style].font_height;
    u8 GUI_font_width  = font_styles[font_style].font_width;

    if ((x + GUI_font_width) > X_MAX || (y + GUI_font_height) > Y_MAX) {
        return GUI_ERROR_CODE_OVERFLOW;
    }

    const unsigned char *font = GUI_GetFont(data, font_style);
    if (NULL == font) {
        return GUI_ERROR_CODE_NOT_SUPPORT_CHAR;
    }

    GUI_DispPic(image, x, y, font, GUI_font_width, GUI_font_height);

    return GUI_ERROR_CODE_NO_ERROR;
}

unsigned char GUI_DispStr(unsigned char *image, int x, int y, const char *str, unsigned char dir, FONT_STYLE_NAME_Typedef font_style)
{
    assert(image);
    int     len             = app_strlen((const char *)str);
    u8 GUI_font_height = font_styles[font_style].font_height;
    u8 GUI_font_width  = font_styles[font_style].font_width;

    int i;
    if (dir) { //horizontal
        if ((len * GUI_font_width + x) > X_MAX) {
            return GUI_ERROR_CODE_OVERFLOW;
        }
        for (i = 0; i < len; i++) {
            GUI_DispChar(image, x + i * GUI_font_width, y, str[i], font_style);
        }
    } else {
        if ((len * GUI_font_height + y) > Y_MAX) {
            return GUI_ERROR_CODE_OVERFLOW;
        }
        for (i = 0; i < len; i++) {
            GUI_DispChar(image, x, y + i * 2, str[i], font_style);
        }
    }
    return GUI_ERROR_CODE_NO_ERROR;
}

void GUI_Clear(unsigned char *image, unsigned char colour)
{
    unsigned char data = colour ? 0xff : 0;
    memset(image, data, EPD_DATA_SIZE);
}
