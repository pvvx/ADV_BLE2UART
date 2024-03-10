/********************************************************************************************************
 * @file	hw_ecc.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2020.06
 *
 * @par		Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
 *			All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef HW_ECC_H_
#define HW_ECC_H_

#include "config.h"
#include "algorithm/ecc/hw_ecc.h"
#include "algorithm/ecc/ecc_curve.h"

#if ( (MCU_CORE_TYPE == MCU_CORE_827x) || (MCU_CORE_TYPE == MCU_CORE_9518) )

/* hECC_RNG_Function type
The RNG function should fill 'size' random bytes into 'dest'. It should return 1 if
'dest' was filled with random data, or 0 if the random data could not be generated.
The filled-in values should be either truly random, or from a cryptographically-secure PRNG.
A correctly functioning RNG function must be set (using hECC_set_rng()) before calling
hECC_make_key(). */

typedef int (*hECC_rng_func)(unsigned char *dest, unsigned size);


/**
 * @brief		The function that will be used to generate random bytes.
 * @param[in]	resister predefined TRNG function
 * @return		none
 */
void			hwECC_set_rng(hECC_rng_func rng_func);


/**
 * @brief		get ECCP key pair(the key pair could be used in ECDH).
 * @param[out]	public_key	- public key, big--endian.
 * @param[out]	private_key	- private key, big--endian.
 * @param[in]	curve_sel	- ecc_curve select, e.g.: p-256r1.
 * @return		1(success), 0(error).
 */
unsigned char 	hwECC_make_key(unsigned char *public_key, unsigned char *private_key, ecc_curve_t curve_sel);


/**
 * @brief		ECDH compute key.
 * @param[in]	local_prikey	- local private key, big--endian.
 * @param[in]	public_key		- peer public key, big--endian.
 * @param[out]	dhkey			- output dhkey, big--endian.
 * @param[in]	curve_sel		- ecc_curve select, e.g.: p-256r1.
 * @Return		1(success); 0(error).
 */
unsigned char	hwECC_shared_secret(const unsigned char *public_key, const unsigned char *private_key,
									unsigned char *secret, ecc_curve_t curve_sel);




#endif /* The end of #if((MCU_CORE_TYPE == MCU_CORE_827x) || (MCU_CORE_TYPE == MCU_CORE_9518)) */

#endif /* HW_ECC_H_ */


