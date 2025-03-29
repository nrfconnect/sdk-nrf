/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_NRF_MSPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_NRF_MSPI_H_

#include <zephyr/device.h>
#include <zephyr/drivers/mspi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*mspi_api_reset_pin_config)(const struct device *dev, const struct mspi_dev_id *dev_id,
					 const struct gpio_dt_spec *spec, uint8_t gpio_port_num,
					 gpio_flags_t extra_flags);
typedef int (*mspi_api_reset_pin_set)(const struct device *dev, const struct gpio_dt_spec *spec,
				      const struct mspi_dev_id *dev_id, int value);

__subsystem struct nrf_mspi_driver_api {
	struct mspi_driver_api std_api;

	mspi_api_reset_pin_config reset_pin_config;
	mspi_api_reset_pin_set reset_pin_set;
};

/**
 * @brief Configure reset pin. It should be used if there is a chance that reset pin overlaps with
 * any data pin.
 *
 * Function checks if given reset pin overlap with any pin used by MSPI. If it does and can be used
 * as reset in SINGLE mode (for example, ovelaps with data line DQ3), then it is saved in MSPI
 * driver. If it does not overlap, then gpio_pin_configure_dt() is called.
 *
 * @param dev Pointer to the MSPI device structure.
 * @param dev_id Pointer to the device ID of the MSPI device.
 * @param spec GPIO specification from devicetree.
 * @param gpio_port_num GPIO port number of the selected reset pin.
 * @param extra_flags Additional GPIO flags.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Return value from gpio_pin_configure_dt().
 * @retval -EINVAL If reset pin overlaps with MSPI pin that cannot be used as reset (e.g. CLK) or a
 * return value from gpio_pin_configure_dt().
 * @retval -EIO Return value from gpio_pin_configure_dt().
 * @retval -EWOULDBLOCK Return value from gpio_pin_configure_dt().
 */
static inline int nrf_mspi_reset_pin_config(const struct device *dev,
					    const struct mspi_dev_id *dev_id,
					    const struct gpio_dt_spec *spec, uint8_t gpio_port_num,
					    gpio_flags_t extra_flags)
{
	const struct nrf_mspi_driver_api *api = (const struct nrf_mspi_driver_api *)dev->api;

	return api->reset_pin_config(dev, dev_id, spec, gpio_port_num, extra_flags);
}

/**
 * @brief Set reset pin state. It should be used if there is a chance that reset pin overlaps with
 * any data pin.
 *
 * If reset pin does not overlap with any MSPI pin, gpio_pin_set_dt() is called.
 * Assumes that nrf_mspi_reset_pin_config() was called before.
 *
 * @param dev Pointer to the MSPI device structure.
 * @param spec GPIO specification from devicetree.
 * @param dev_id Pointer to the device ID of the MSPI device.
 * @param value Value assigned to the pin.
 *
 * @retval 0 If successful.
 * @retval -EIO Return value from gpio_pin_set_dt().
 * @retval -EWOULDBLOCK Return value from gpio_pin_set_dt().
 */
static inline int nrf_mspi_reset_pin_set(const struct device *dev, const struct gpio_dt_spec *spec,
					 const struct mspi_dev_id *dev_id, int value)
{
	const struct nrf_mspi_driver_api *api = (const struct nrf_mspi_driver_api *)dev->api;

	return api->reset_pin_set(dev, spec, dev_id, value);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_NRF_MSPI_H_ */
