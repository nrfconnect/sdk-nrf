/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>

#include <platform/include/tfm_platform_system.h>
#include <cmsis.h>
#include <stdio.h>
#include <pm_config.h>
#include <tfm_ioctl_api.h>
#include <string.h>
#include <arm_cmse.h>

#include "tfm_ioctl_api.h"
#include "tfm_platform_hal_ioctl.h"
#include <tfm_hal_isolation.h>
#include <tfm_memory_utils.h>

#include <hal/nrf_gpio.h>

void tfm_platform_hal_system_reset(void)
{
	/* Reset the system */
	NVIC_SystemReset();
}

#if CONFIG_FW_INFO
enum tfm_platform_err_t
tfm_platform_hal_fw_info_service(psa_invec  *in_vec, psa_outvec *out_vec)
{
	const struct fw_info *tfm_info;
	struct tfm_fw_info_args_t *args;
	struct tfm_fw_info_out_t *out;
	enum tfm_hal_status_t status;
	enum tfm_platform_err_t err;
	uint32_t attr = TFM_HAL_ACCESS_WRITABLE |
			TFM_HAL_ACCESS_READABLE |
			TFM_HAL_ACCESS_NS;

	if (in_vec->len != sizeof(struct tfm_fw_info_args_t) ||
	    out_vec->len != sizeof(struct tfm_fw_info_out_t)) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	args = (struct tfm_fw_info_args_t *)in_vec->base;
	out = (struct tfm_fw_info_out_t *)out_vec->base;

	/* Assume failure, unless valid region is hit in the loop */
	out->result = -1;
	err = TFM_PLATFORM_ERR_INVALID_PARAM;

	if (args->info == NULL) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	status = tfm_hal_memory_has_access((uintptr_t)args->info,
					    sizeof(struct fw_info),
					    attr);
	if (status != TFM_HAL_SUCCESS) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	tfm_info = fw_info_find((uintptr_t)args->fw_address);
	if (tfm_info != NULL) {
		tfm_memcpy(args->info, tfm_info, sizeof(struct fw_info));
		out->result = 0;
		err = TFM_PLATFORM_ERR_SUCCESS;
	}

	return err;
}
#endif

enum tfm_platform_err_t tfm_platform_hal_ioctl(tfm_platform_ioctl_req_t request,
                                               psa_invec  *in_vec,
                                               psa_outvec *out_vec)
{
	/* Core IOCTL services */
	switch (request) {
	case TFM_PLATFORM_IOCTL_READ_SERVICE:
		return tfm_platform_hal_read_service(in_vec, out_vec);
#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
	case TFM_PLATFORM_IOCTL_GPIO_SERVICE:
		return tfm_platform_hal_gpio_service(in_vec, out_vec);
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */

#if CONFIG_FW_INFO
	/* Board specific IOCTL services */
	case TFM_PLATFORM_IOCTL_FW_INFO:
		return tfm_platform_hal_fw_info_service(in_vec, out_vec);
#endif
	/* Not a supported IOCTL service.*/
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}
}
