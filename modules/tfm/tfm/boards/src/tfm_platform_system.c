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

/* This contains the user provided allowed ranges */
#include <tfm_read_ranges.h>

#include <hal/nrf_gpio.h>

static bool ptr_in_secure_area(intptr_t ptr)
{
	return cmse_TT((void *)ptr).flags.secure == 1;
}

static enum tfm_platform_err_t
tfm_platform_hal_read_service(const psa_invec  *in_vec,
			      const psa_outvec *out_vec)
{
	struct tfm_read_service_args_t *args;
	struct tfm_read_service_out_t *out;
	enum tfm_platform_err_t err;


	if (in_vec->len != sizeof(struct tfm_read_service_args_t) ||
	    out_vec->len != sizeof(struct tfm_read_service_out_t)) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	args = (struct tfm_read_service_args_t *)in_vec->base;
	out = (struct tfm_read_service_out_t *)out_vec->base;

	/* Assume failure, unless valid region is hit in the loop */
	out->result = -1;
	err = TFM_PLATFORM_ERR_INVALID_PARAM;

	if (args->destination == NULL || args->len <= 0) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	if (ptr_in_secure_area((intptr_t)args->destination)) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ranges); i++) {
		uint32_t start = ranges[i].start;
		uint32_t size = ranges[i].size;

		if (args->addr >= start &&
		    args->addr + args->len <= start + size) {
			memcpy(args->destination,
			       (const void *)args->addr,
			       args->len);
			out->result = 0;
			err = TFM_PLATFORM_ERR_SUCCESS;
			break;
		}
	}

	in_vec++;
	out_vec++;

	return err;
}

#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
static bool valid_mcu_select(uint32_t mcu)
{
	switch (mcu) {
	case NRF_GPIO_PIN_MCUSEL_APP:
	case NRF_GPIO_PIN_MCUSEL_NETWORK:
	case NRF_GPIO_PIN_MCUSEL_PERIPHERAL:
	case NRF_GPIO_PIN_MCUSEL_TND:
		return true;
	default:
		return false;
	}
}

static uint32_t gpio_service_mcu_select(struct tfm_gpio_service_args * args)
{
	if (nrf_gpio_pin_present_check(args->mcu_select.pin_number) &&
	    valid_mcu_select(args->mcu_select.mcu)) {
		nrf_gpio_pin_mcu_select(args->mcu_select.pin_number, args->mcu_select.mcu);
		return 0;
	} else {
		return -1;
	}
}

static enum tfm_platform_err_t
tfm_platform_hal_gpio_service(const psa_invec  *in_vec, const psa_outvec *out_vec)
{
	struct tfm_gpio_service_args *args;
	struct tfm_gpio_service_out *out;

	if (in_vec->len != sizeof(struct tfm_gpio_service_args) ||
	    out_vec->len != sizeof(struct tfm_gpio_service_out)) {
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	args = (struct tfm_gpio_service_args *)in_vec->base;
	out = (struct tfm_gpio_service_out *)out_vec->base;
	out->result = -1;

	switch(args->type)
	{
	case TFM_GPIO_SERVICE_TYPE_PIN_MCU_SELECT:
		out->result = gpio_service_mcu_select(args);
		break;
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}


	return TFM_PLATFORM_ERR_SUCCESS;
}
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */

void tfm_platform_hal_system_reset(void)
{
	/* Reset the system */
	NVIC_SystemReset();
}

enum tfm_platform_err_t
tfm_platform_hal_ioctl(tfm_platform_ioctl_req_t request, psa_invec  *in_vec,
		       psa_outvec *out_vec)
{
	switch (request) {
	case TFM_PLATFORM_IOCTL_READ_SERVICE:
		return tfm_platform_hal_read_service(in_vec, out_vec);
#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
	case TFM_PLATFORM_IOCTL_GPIO_SERVICE:
		return tfm_platform_hal_gpio_service(in_vec, out_vec);
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */
	default:
		return TFM_PLATFORM_ERR_NOT_SUPPORTED;
	}
}
