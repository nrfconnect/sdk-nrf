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
 * @brief Configure reset pin. It is mandatory when reset pin overlaps with any data pin.
 *
 * @param dev pointer to the mspi device structure.
 * @param psel pin number
 *
 * @retval non-negative the observed state of the on-off service associated
 *                      with the clock machine at the time the request was
 *                      processed (see onoff_release()), if successful.
 * @retval -EIO if service has recorded an error.
 * @retval -ENOTSUP if the service is not in a state that permits release.
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
 * @brief Set reset pin state.
 *
 * @param dev pointer to the mspi device structure.
 * @param psel pin number
 *
 * @retval non-negative the observed state of the on-off service associated
 *                      with the clock machine at the time the request was
 *                      processed (see onoff_release()), if successful.
 * @retval -EIO if service has recorded an error.
 * @retval -ENOTSUP if the service is not in a state that permits release.
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
