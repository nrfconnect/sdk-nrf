/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_TWI_DRV__
#define MPSL_FEM_TWI_DRV__

#include <zephyr/devicetree.h>
#include <mpsl_fem_nrf22xx_twi_config_common.h>

/**
 * @brief Initializer of structure @c mpsl_fem_twi_if_t allowing to use TWI interface.
 *
 * @param node_id Devicetree node identifier for the TWI part for the FEM device.
 */
#define MPSL_FEM_TWI_DRV_IF_INITIALIZER(node_id) \
	(mpsl_fem_twi_if_t){ \
		.enabled = true, \
		.slave_address = ((uint8_t)DT_REG_ADDR(node_id)), \
		.p_instance = (void *)DEVICE_DT_GET(DT_BUS(node_id)), \
		.p_xfer_read = mpsl_fem_twi_drv_impl_xfer_read, \
		.p_xfer_write = mpsl_fem_twi_drv_impl_xfer_write, \
	}

int32_t mpsl_fem_twi_drv_impl_xfer_read(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, uint8_t *p_data, uint8_t data_length);

int32_t mpsl_fem_twi_drv_impl_xfer_write(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, const uint8_t *p_data, uint8_t data_length);

#endif /* MPSL_FEM_TWI_DRV__ */
