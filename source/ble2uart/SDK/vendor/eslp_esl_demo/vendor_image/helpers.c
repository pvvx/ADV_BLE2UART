/********************************************************************************************************
 * @file    helpers.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    02,2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "helpers.h"

int app_strlen(const char *s)
{
	int len = 0;

	while (s[len]) {
		len++;
	}

	return len;
}

char app_tolower(char a)
{
	if ((a >= 'A') && (a <= 'Z')) {
		return a + 0x20;
	}

	return a;
}

char app_toupper(char a)
{
	if ((a >= 'a') && (a <= 'z')) {
		return a - 0x20;
	}

	return a;
}

int app_strcasecmp(const char *a, const char *b)
{
	char _a, _b;

	do {
		_a = app_tolower(*a);
		_b = app_tolower(*b);
		a++;
		b++;
	} while ((_a == _b) && (_a != 0));

	return _a - _b;
}
