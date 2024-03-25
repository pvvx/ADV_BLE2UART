/********************************************************************************************************
 * @file	user_config.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions
 *              in binary form must reproduce the above copyright notice, this list of
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *
 *              3. Neither the name of TELINK, nor the names of its contributors may be
 *              used to endorse or promote products derived from this software without
 *              specific prior written permission.
 *
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or
 *              relating to such deletion(s), modification(s) or alteration(s).
 *
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************************************/
#pragma once

#if(__PROJECT_827x_SLAVE__ )
	#include "vendor/b85m_slave/app_config.h"
#elif(__PROJECT_825x_SLAVE__ )
	#include "vendor/b85m_slave/app_config.h"
#elif(__PROJECT_827x_M1S1__ )
	#include "vendor/b85m_m1s1/app_config.h"
#elif(__PROJECT_825x_M1S1__ )
	#include "vendor/b85m_m1s1/app_config.h"
#elif(__PROJECT_8258_CONTROLLER__ )
	#include "vendor/b85m_controller/app_config.h"
#elif(__PROJECT_8278_CONTROLLER__)
	#include "vendor/b85m_controller/app_config.h"
#elif(__PROJECT_8258_HIGHER_TESTER__ )
	#include "vendor/8258_higher_tester/app_config.h"
#elif(__PROJECT_8258_LOWER_TESTER__ )
	#include "vendor/8258_lower_tester/app_config.h"
#elif(__PROJECT_8258_CIS_MASTER__ )
	#include "vendor/8258_cis_master/app_config.h"
#elif(__PROJECT_8258_CIS_SLAVE__ )
	#include "vendor/8258_cis_slave/app_config.h"
#elif(__PROJECT_8258_INTERNAL_TEST__ )
	#include "vendor/b85m_internal_test/app_config.h"
#elif(__PROJECT_8278_INTERNAL_TEST__ )
	#include "vendor/b85m_internal_test/app_config.h"
#elif(__PROJECT_8258_DEMO__ )
	#include "vendor/b85m_demo/app_config.h"
#elif(__PROJECT_8278_DEMO__ )
	#include "vendor/b85m_demo/app_config.h"
#elif(__PROJECT_8278_FEATURE_TEST__ )
	#include "vendor/b85m_feature/app_config.h"
#elif(__PROJECT_8258_FEATURE_TEST__ )
	#include "vendor/b85m_feature/app_config.h"
#elif(__PROJECT_8258_MASTER_DONGLE__)
	#include "vendor/b85m_master_dongle/app_config.h"
#elif(__PROJECT_8278_MASTER_DONGLE__)
	#include "vendor/b85m_master_dongle/app_config.h"
#else
	#include "vendor/common/default_config.h"
#endif

