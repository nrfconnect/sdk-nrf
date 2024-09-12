/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __PLATFORM_OTP_IDS_H__
#define __PLATFORM_OTP_IDS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tfm_otp_element_id_t {
	PLAT_OTP_ID_LCS = UINT32_MAX -1,
	PLAT_OTP_ID_MAX = UINT32_MAX,
};

#ifdef __cplusplus
}
#endif

#endif /* __PLATFORM_OTP_IDS_H__ */
