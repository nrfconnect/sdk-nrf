/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_NPMX_NPMX_GLUE_H__
#define ZEPHYR_DRIVERS_NPMX_NPMX_GLUE_H__

#include <npmx.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_glue nrfx_glue.h
 * @{
 * @ingroup npmx
 *
 * @brief This file contains functions declarations and macros that should be implemented
 *        according to the needs of the host environment into which @em npmx is integrated.
 */

/**
 * @brief Macro for placing a runtime assertion.
 *
 * @param expression Expression to be evaluated.
 */
#define NPMX_ASSERT(expression) __ASSERT_NO_MSG(expression)

/**
 * @brief Function for I2C write.
 *
 * @param[in] inst             Pointer to backend instance
 * @param[in] register_address Register address in npmx device to be modified
 * @param[in] p_data           Pointer to data to write
 * @param[in] num_of_bytes     Number of bytes of data to write
 */
int npmx_backend_i2c_write(npmx_backend_instance_t const *p_inst, uint16_t register_address,
			   uint8_t *p_data, uint32_t num_of_bytes);
/**
 * @brief Function for I2C read.
 *
 * @param[in] inst             Pointer to backend instance
 * @param[in] register_address Register address in npmx device to be read
 * @param[in] p_data           Pointer to buffer for read data
 * @param[in] num_of_bytes     Number of bytes to read
 */
int npmx_backend_i2c_read(npmx_backend_instance_t const *p_inst, uint16_t register_address,
			  uint8_t *p_data, uint32_t num_of_bytes);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // ZEPHYR_DRIVERS_NPMX_NPMX_GLUE_H__
