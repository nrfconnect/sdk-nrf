/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/util.h>

#include <platform/include/tfm_platform_system.h>
#include <cmsis.h>
#include <stdio.h>
#include <pm_config.h>
#include <tfm_ioctl_api.h>
#include <string.h>
#include <arm_cmse.h>

#include "tfm_ioctl_core_api.h"
#include "tfm_platform_hal_ioctl.h"

/* This contains the user provided allowed ranges */
#include <tfm_read_ranges.h>

#include <hal/nrf_gpio.h>

void tfm_platform_hal_system_reset(void)
{
	/* Reset the system */
	NVIC_SystemReset();
}

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

	/* Board specific IOCTL services */

	/* Not a supported IOCTL service.*/
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}
}
