/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_TWI_DRV__
#define MPSL_FEM_TWI_DRV__

#include <zephyr/devicetree.h>
#include <mpsl_fem_nrf22xx_twi_config_common.h>
#include <nrfx_twim.h>

/** @brief Structure representing an I2C bus driver for a Front-End Module. */
typedef struct {
	const struct device *dev;
	nrfx_twim_event_handler_t nrfx_twim_callback_saved;
	void *nrfx_twim_callback_ctx_saved;
	mpsl_fem_twi_async_xfer_write_cb_t fem_twi_async_xfwr_write_cb;
	void *fem_twi_async_xfwr_write_cb_ctx;
} mpsl_fem_twi_drv_t;

#define MPSL_FEM_TWI_DRV_INITIALIZER(node_id)		\
	(mpsl_fem_twi_drv_t){				\
		.dev = DEVICE_DT_GET(DT_BUS(node_id)),	\
	}

/** @brief Prepare the FEM TWI interface structure.
 *
 * @note This function prepares just synchronous part of the interface.
 * If Front-End Module driver requires an asynchronous part of the interface call
 * the @ref mpsl_fem_twi_drv_fem_twi_if_prepare_add_async function afterwards.
 *
 * @param drv     Pointer to the FEM TWI driver instance object initialized with
 *                MPSL_FEM_TWI_DRV_INITIALIZER.
 * @param twi_if  Pointer to the FEM TWI interface structure to fill.
 * @param address The address of the FEM device on the TWI bus.
 */
void mpsl_fem_twi_drv_fem_twi_if_prepare(mpsl_fem_twi_drv_t *drv, mpsl_fem_twi_if_t *twi_if,
					 uint8_t address);

/** @brief Prepare the asynchronous part of the FEM TWI interface structure.
 *
 * @note Assumes that the @ref mpsl_fem_twi_drv_fem_twi_if_prepare function
 * has already been called.
 *
 * @param twi_if  Pointer to the interface structure to fill.
 */
void mpsl_fem_twi_drv_fem_twi_if_prepare_add_async(mpsl_fem_twi_if_t *twi_if);

#endif /* MPSL_FEM_TWI_DRV__ */
