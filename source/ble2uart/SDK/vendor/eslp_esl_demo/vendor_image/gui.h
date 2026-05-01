/********************************************************************************************************
 * @file    gui.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef _TELINK_ESL_GUI_H
#define _TELINK_ESL_GUI_H

#define END_MARK          0

#define CNY_CHAR          'Y'
#define USD_CHAR          '$'
#define EUR_CHAR          'E'
#define GBP_CHAR          'F'

#define FONT_8_16_WIDTH   8
#define FONT_8_16_HEIGHT  16

#define FONT_16_32_WIDTH  16
#define FONT_16_32_HEIGHT 32

#define FONT_32_56_WIDTH  32
#define FONT_32_56_HEIGHT 56

typedef enum
{
    FONT_8_x_16,
    FONT_16_x_32,
    FONT_32_x_56,
    FONT_MAX
} FONT_STYLE_NAME_Typedef;

typedef struct
{
    int X_channel;
    int Y_channel;
    int X_MAX;
    int Y_MAX;
} NODESIZE_Typedef;

extern NODESIZE_Typedef Node_Size;

/**
 * @brief      Convert a byte array to a hexadecimal string.
 * @param[in]  bytes - Pointer to the byte array to be converted.
 * @param[in]  len - The length of the byte array.
 * @param[out] str - Pointer to the string where the hexadecimal result will be stored.
 * @return     none.
 */
void GUI_BytesToHexStr(const char *bytes, int len, char *str);

/**
 * @brief      Convert an integer to a decimal string.
 * @param[in]  data - The integer value to be converted.
 * @param[out] str - Pointer to the string where the decimal result will be stored.
 * @return     none.
 */
void GUI_IntToDecStr(unsigned int data, unsigned char *str);

/**
 * @brief      Display a single character on the image at the specified coordinates.
 * @param[in]  image - Pointer to the image buffer.
 * @param[in]  x - The x-coordinate of the character to be displayed.
 * @param[in]  y - The y-coordinate of the character to be displayed.
 * @param[in]  data - The character to be displayed.
 * @param[in]  font_style - The style of the font to be used for the character.
 * @return     unsigned char - 0 if successful, non-zero error code if failed.
 */
unsigned char GUI_DispChar(unsigned char *image, int x, int y, unsigned char data, FONT_STYLE_NAME_Typedef font_style);

/**
 * @brief      Display a string on the image at the specified coordinates.
 * @param[in]  image - Pointer to the image buffer.
 * @param[in]  x - The x-coordinate where the string will be displayed.
 * @param[in]  y - The y-coordinate where the string will be displayed.
 * @param[in]  str - Pointer to the string to be displayed.
 * @param[in]  dir - Direction of the string display: 1 for horizontal, 0 for vertical.
 * @param[in]  font_style - The style of the font to be used for the string.
 * @return     unsigned char - 0 if successful, non-zero error code if failed.
 */
unsigned char GUI_DispStr(unsigned char *image, int x, int y, const char *str, unsigned char dir, FONT_STYLE_NAME_Typedef font_style);

/**
 * @brief      Display an image (picture) on the screen at the specified coordinates.
 * @param[in]  image - Pointer to the image buffer.
 * @param[in]  x - The x-coordinate where the image will be displayed.
 * @param[in]  y - The y-coordinate where the image will be displayed.
 * @param[in]  pic - Pointer to the image data (picture).
 * @param[in]  width - The width of the image.
 * @param[in]  height - The height of the image.
 * @return     unsigned char - 0 if successful, non-zero error code if failed.
 */
unsigned char GUI_DispPic(unsigned char *image, int x, int y, const unsigned char *pic, unsigned char width, unsigned char height);

/**
 * @brief      Clear the image with a specified color.
 * @param[in]  image - Pointer to the image buffer.
 * @param[in]  colour - The color to clear the image with: 0 for black, 1 for white.
 * @return     none.
 */
void GUI_Clear(unsigned char *image, unsigned char colour);


#endif
