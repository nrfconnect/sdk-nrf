/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief TFM IOCTL API header.
 */


#ifndef TFM_IOCTL_API_H__
#define TFM_IOCTL_API_H__

/**
 * @defgroup tfm_ioctl_api TFM IOCTL API
 * @{
 *
 */

#include <limits.h>
#include <stdint.h>
#include <tfm_api.h>
#include <tfm_platform_api.h>
#include <hal/nrf_gpio.h>

/* Include core IOCTL services */
#include <tfm_ioctl_core_api.h>

#include <autoconf.h>

#if CONFIG_FW_INFO
#include <fw_info_bare.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Board specific IOCTL services can be added here */
enum tfm_platform_ioctl_reqest_types_t {
	TFM_PLATFORM_IOCTL_FW_INFO = TFM_PLATFORM_IOCTL_CORE_LAST,
};

#if CONFIG_FW_INFO

/** @brief Argument list for each platform firmware info service.
 */
struct tfm_fw_info_args_t {
	void *fw_address;
	struct fw_info *info;
};

/** @brief Output list for each platform firmware info service
 */
struct tfm_fw_info_out_t {
	uint32_t result;
};

/** Search for the fw_info structure in firmware image located at address.
 *
 * @param[in]   fw_address  Address where firmware image is stored.
 * @param[out]  info        Pointer to where found info is to be written.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info is NULL or if no info is found.
 * @retval -EPERM   If the TF-M platform service request failed.
 */
int tfm_platform_firmware_info(uint32_t fw_address, struct fw_info *info);

/** Check if S0 is the active B1 slot.
 *
 * @param[in]   s0_address Address of s0 slot.
 * @param[in]   s1_address Address of s1 slot.
 * @param[out]  s0_active  Set to 'true' if s0 is active slot, 'false' otherwise
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info for both slots could not be found.
 * @retval -EPERM   If the TF-M platform service request failed.
 */
int tfm_platform_s0_active(uint32_t s0_address, uint32_t s1_address,
			   bool *s0_active);

#endif /* CONFIG_FW_INFO */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* TFM_IOCTL_API_H__ */
