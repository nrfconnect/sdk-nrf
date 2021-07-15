/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_cx_config_thread.h
 *
 * @defgroup mpsl_cx_thread MPSL Coexistence configuration according to Thread Radio Coexistence
 * @ingroup  mpsl_cx
 *
 * @{
 */

#ifndef MPSL_CX_CONFIG_THREAD_H__
#define MPSL_CX_CONFIG_THREAD_H__

#include <stdbool.h>
#include <stdint.h>

/** @brief Configuration parameters for the Thread Radio Coexistence variant. */
struct mpsl_cx_thread_interface_config {
	/** GPIO pin number of REQUEST pin. */
	uint8_t request_pin;
	/** GPIO pin number of PRIORITY pin. */
	uint8_t priority_pin;
	/** GPIO pin number of GRANTED pin. */
	uint8_t granted_pin;
};

/** @brief Configures the Thread Radio Coexistence interface.
 *
 * This function sets device interface parameters for the Coexistence module.
 * The module is used to control PTA interface through the given pins and resources.
 *
 * @param[in] config Pointer to the interface parameters.
 *
 * @retval   0             Coexistence interface successfully configured.
 * @retval   -NRF_EPERM    Coexistence interface is not available.
 *
 */
int32_t mpsl_cx_thread_interface_config_set(
		struct mpsl_cx_thread_interface_config const * const config);

#endif // MPSL_CX_CONFIG_THREAD_H__

/**@} */
