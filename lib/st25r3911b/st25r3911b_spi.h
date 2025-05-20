/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ST25R3911B_SPI_H_
#define ST25R3911B_SPI_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

/**
 * @file
 * @defgroup st25r3911b_nfc_spi ST25R3911B NFC Reader SPI
 * @{
 *
 * @brief API for the ST25R3911B NFC Reader SPI communication.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** ST25R3911B FIFO maximum length. */
#define ST25R3911B_MAX_FIFO_LEN 96

/** @brief Initialize ST25R3911B NFC Reader SPI hardware interface.
 *
 *  @details This function initializes the hardware SPI interface
 *           according to ST25R3911B IC documentation chapter 1.2.12.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_spi_init(void);

/** @brief Write multiple NFC Reader registers.
 *
 *  @details The register address is auto-incrementing.
 *           Registers are written in the order ascending from
 *           the starting address.
 *
 *  @param[in] start_reg The address of the register from which the writing
 *                       will start. See @ref st25r3911b_reg for the
 *                       register addresses.
 *  @param[in] val Register values to write.
 *  @param[in] len Number of registers to write.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_multiple_reg_write(uint8_t start_reg, uint8_t *val,
				  size_t len);

/** @brief Write a single NFCA Reader register.
 *
 *  @param[in] reg Register address (see @ref st25r3911b_reg).
 *  @param[in] val Value to write.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
static inline int st25r3911b_reg_write(uint8_t reg, uint8_t val)
{
	return st25r3911b_multiple_reg_write(reg, &val, 1);
}

/** @brief Read multiple NFC Reader registers.
 *
 *  @details The register address is auto-incrementing.
 *           Registers are read in the order ascending from
 *           the starting address.
 *
 *  @param[in] start_reg The address of the register from which the
 *                       reading will start. See @ref st25r3911b_reg
 *                       for the register addresses.
 *  @param[out] val Read registers values.
 *  @param[in] len Number of registers to read.
 *
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_multiple_reg_read(uint8_t start_reg, uint8_t *val, size_t len);

/** @brief Read a single NFC Reader register.
 *
 *  @param[in] reg Register address (see @ref st25r3911b_reg).
 *  @param[out] val Read register value.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
static inline int st25r3911b_reg_read(uint8_t reg, uint8_t *val)
{
	return st25r3911b_multiple_reg_read(reg, val, 1);
}

/** @brief Execute an NFC Reader direct command.
 *
 *  @details Direct commands are used to automate certain activities.
 *
 *  @param[in] cmd Direct command (see @ref st25r3911b_direct_command).
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_cmd_execute(uint8_t cmd);

/** @brief Modify a single NFC Reader register.
 *
 *  @details The first register value is read,
 *           then the bits selected by the clear mask are cleared.
 *           Finally, the bits selected by the set mask are set.
 *
 *  @param[in] reg Register address (see @ref st25r3911b_reg).
 *  @param[in] clr_mask Bitmask that defines which register
 *                      bits to clear.
 *  @param[in] set_mask Bitmask that defines which register
 *                      bits to set.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_reg_modify(uint8_t reg, uint8_t clr_mask, uint8_t set_mask);

/** @brief Read NFC Reader FIFO data.
 *
 *  @details Data from FIFO is read by SPI. The data length to
 *           read can be in the range of 1 to 96 bytes.
 *
 *  @param[out] data Read FIFO data.
 *  @param[in] length Length of data to read.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_fifo_read(uint8_t *data, size_t length);

/** @brief Write NFC Reader FIFO data.
 *
 *  @details Data from FIFO is written by SPI.The  data length
 *           to write can be in the range of 1 to 96 bytes.
 *
 *  @param[in] data  FIFO data to write.
 *  @param[in] length Length of data to write.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_fifo_write(uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_SPI_H_ */
