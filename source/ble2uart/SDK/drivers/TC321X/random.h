/********************************************************************************************************
 * @file    random.h
 *
 * @brief   This is the header file for TC321X
 *
 * @author  Driver Group
 * @date    2024
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
#pragma once

#include "register.h"

/**
 * @brief     This function performs to get one unsigned char random number
 * @param[in] none.
 * @return    the value of one random number.
 */
unsigned char rand_uchar(void);

/**
 * @brief     This function performs to get one unsigned short random number
 * @param[in] none.
 * @return    the value of one random number.
 */
unsigned short rand_ushort(void);

/**
 * @brief     This function performs to get one unsigned int random number
 * @param[in] none.
 * @return    the value of one random number.
 */
unsigned int rand(void);

/**
 * @brief     This function performs to get a serial of random number.
 * @param[in]  len- the length of random number
 * @param[in]  data - the first address of buffer store random number in
 * @return    the result of a serial of random number.
 */
void generateRandomNum(int len, unsigned char *data);
