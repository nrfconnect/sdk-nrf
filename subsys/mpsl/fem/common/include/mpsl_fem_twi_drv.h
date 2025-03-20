/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_TWI_DRV__
#define MPSL_FEM_TWI_DRV__

#include <zephyr/devicetree.h>
#include <mpsl_fem_nrf22xx_twi_config_common.h>

#define MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_BEGIN(node_id) \
	(mpsl_fem_twi_if_t){ \
		.enabled = true, \
		.slave_address = ((uint8_t)DT_REG_ADDR(node_id)), \
		.p_instance = (void *)DEVICE_DT_GET(DT_BUS(node_id)),

#define MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_XFERS \
	.p_xfer_read = mpsl_fem_twi_drv_impl_xfer_read, \
	.p_xfer_write = mpsl_fem_twi_drv_impl_xfer_write,

#define MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_XFERS_ASYNC \
	.p_xfer_write_async = mpsl_fem_twi_drv_impl_xfer_write_async, \
	.p_xfer_write_async_time_get = mpsl_fem_twi_drv_impl_xfer_write_async_time_get,

#define MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_END \
	}

/**
 * @brief Initializer of structure @c mpsl_fem_twi_if_t allowing to use TWI interface.
 *
 * @note Initializes only the part related to the synchonous interface for boot-time
 * initialization of the FEM.
 *
 * @sa @ref @ref MPSL_FEM_TWI_DRV_IF_INITIALIZER_WASYNC
 *
 * @param node_id Devicetree node identifier for the TWI part for the FEM device.
 */
#define MPSL_FEM_TWI_DRV_IF_INITIALIZER(node_id) \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_BEGIN(node_id) \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_XFERS \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_END

/**
 * @brief Initializer of structure @c mpsl_fem_twi_if_t allowing to use TWI interface
 * with asynchronous extension.
 *
 * @sa @ref @ref MPSL_FEM_TWI_DRV_IF_INITIALIZER
 *
 * @param node_id Devicetree node identifier for the TWI part for the FEM device.
 */
#define MPSL_FEM_TWI_DRV_IF_INITIALIZER_WASYNC(node_id) \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_BEGIN(node_id) \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_XFERS \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_XFERS_ASYNC \
	MPSL_FEM_TWI_DRV_IF_PARTIAL_INITIALIZER_END


int32_t mpsl_fem_twi_drv_impl_xfer_read(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, uint8_t *p_data, uint8_t data_length);

int32_t mpsl_fem_twi_drv_impl_xfer_write(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, const uint8_t *p_data, uint8_t data_length);

int32_t mpsl_fem_twi_drv_impl_xfer_write_async(void *p_instance, uint8_t slave_address,
	const uint8_t *p_data, uint8_t data_length, mpsl_fem_twi_async_xfer_write_cb_t p_callback,
	void *p_context);

uint32_t mpsl_fem_twi_drv_impl_xfer_write_async_time_get(void *p_instance, uint8_t data_length);

#endif /* MPSL_FEM_TWI_DRV__ */
