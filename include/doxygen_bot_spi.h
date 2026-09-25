/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file doxygen_bot_spi.h
 * @brief SPI module for Doxygen PR reviewer bot verification.
 *
 * @defgroup doxygen_bot_spi Doxygen bot SPI test module
 * @{
 */

#ifndef DOXYGEN_BOT_SPI_H
#define DOXYGEN_BOT_SPI_H

#include <stddef.h>
#include <stdint.h>

/**
 * \brief Initialise the SPI bus.
 *
 * Configures SPI controller with default settings.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_spi_init(void);

/**
 * @brief Perform SPI transceive operation.
 *
 * @param tx_buf Transmit buffer.
 * @param [in] rx_buf Receive buffer.
 * @param len Number of bytes to transfer.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_spi_transceive(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len);

/**
 * @brief Release SPI bus resources.
 *
 * Frees resources allocated by @ref doxy_spi_init.
 *
 * @retval 0 on succes.
 * @retval negative errno code on failure.
 */
int doxy_spi_release(void);

/**
 * @brief Configure SPI bus frequency.
 *
 * @param frequency_hz Desired bus frequency in hertz.
 *
 * @returns 0 on success, negative errno code on failure.
 */
int doxy_spi_configure(uint32_t frequency_hz);

/** @} */

#endif /* DOXYGEN_BOT_SPI_H */
