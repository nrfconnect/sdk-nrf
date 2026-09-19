/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file doxygen_bot_i2c.h
 * @brief I2C module for Doxygen PR reviewer bot verification.
 *
 * @defgroup doxygen_bot_i2c Doxygen bot I2C test module
 * @{
 */

#ifndef DOXYGEN_BOT_I2C_H
#define DOXYGEN_BOT_I2C_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialise the I2C bus.
 *
 * Configures the I2C controller for master mode operation
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_i2c_init(void);

/**
 * @brief Write data to an I2C slave device.
 *
 * Sends data bytes to the specified slav address.
 *
 * @param addr 7-bit I2C slave address.
 * @param [in] data Data buffer to write.
 * @param len Number of bytes to write.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_i2c_write(uint8_t addr, const uint8_t *data, size_t len);

/**
 * @brief Read data from an I2C slave device.
 *
 * @param addr 7-bit I2C slave address.
 * @param [inout] dst Buffer to store read data.
 * @param len Number of bytes to read.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_i2c_read(uint8_t addr, uint8_t *dst, size_t len);

/**
 * @brief Recover a stuck I2C bus.
 *
 * Attempts to recover the bus by clocking SCL.
 *
 * @return 0 on success.
 * @return negative errno code on failure.
 */
int doxy_i2c_recover(void);

/** @} */

#endif /* DOXYGEN_BOT_I2C_H */
