/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_twi_drv.h>
#include <zephyr/drivers/i2c.h>


int32_t mpsl_fem_twi_drv_impl_xfer_read(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, uint8_t *p_data, uint8_t data_length)
{
	const struct device *dev = (const struct device *)p_instance;

	return i2c_burst_read(dev, slave_address, internal_address, p_data, data_length);
}

int32_t mpsl_fem_twi_drv_impl_xfer_write(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, const uint8_t *p_data, uint8_t data_length)
{
	const struct device *dev = (const struct device *)p_instance;

	return i2c_burst_write(dev, slave_address, internal_address, p_data, data_length);
}
