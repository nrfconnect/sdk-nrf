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

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Supported request types.
 */
enum tfm_platform_ioctl_reqest_types_t {
	TFM_PLATFORM_IOCTL_READ_SERVICE,
	TFM_PLATFORM_IOCTL_GPIO_SERVICE,
};

/** @brief Argument list for each platform read service.
 */
struct tfm_read_service_args_t {
	void *destination;
	uint32_t addr;
	size_t len;
};


/** @brief Output list for each read platform service
 */
struct tfm_read_service_out_t {
	uint32_t result;
};

enum tfm_gpio_service_type {
	/** Select which MCU / Subsystem controls the pin */
	TFM_GPIO_SERVICE_TYPE_PIN_MCU_SELECT = 0,
};

/** @brief Arguments for selecting the MCU to control a GPIO pin. */
struct tfm_gpio_service_args_mcu_select {
	uint32_t pin_number;
	uint32_t mcu;
};

/** @brief Argument list for each platform GPIO service */
struct tfm_gpio_service_args {
	uint32_t type;
	union {
		struct tfm_gpio_service_args_mcu_select mcu_select;
	};
};

/** @brief Output list for each GPIO platform service
 */
struct tfm_gpio_service_out {
	uint32_t result;
};

/**
 * @brief Perform a read operation.
 *
 * @param[out] destination   Pointer where read result is stored
 * @param[in]  addr          Address to read from
 * @param[in]  len           Number of bytes to read
 * @param[out] result        Result of operation
 *
 * @return Returns values as specified by the tfm_platform_err_t
 */
enum tfm_platform_err_t tfm_platform_mem_read(void *destination, uint32_t addr,
					      size_t len, uint32_t *result);

/** @brief Represents an accepted read range.
 */
struct tfm_read_service_range {
	uint32_t start;
	size_t size;
};

/**
 * @brief Perform a GPIO MCU select operation.
 *
 * @param pin_number         Pin_number.
 * @param mcu                MCU to control the pin, use nrf_gpio_pin_mcusel_t values.

 * @param[out] result        Result of operation
 *
 * @return Returns values as specified by the tfm_platform_err_t
 */
enum tfm_platform_err_t tfm_platform_gpio_pin_mcu_select(uint32_t pin_number, uint32_t mcu,
							 uint32_t *result);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* TFM_IOCTL_API_H__ */
