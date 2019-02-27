/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ST25R3911B_SPI_H_
#define ST25R3911B_SPI_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <misc/util.h>

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

/** @brief Initialize ST25R3911B NFC reader SPI hardware interface.
 *
 *  @details Function initialize hardware SPI interface according to ST25R3911B
 *           IC documentation chapter 1.2.12.
 *
 *  @return Returns 0 if initialization was successful,
 *          otherwise negative value.
 */
int st25r3911b_spi_init(void);

/** @brief Write multiple NFC reader registers.
 *
 *  @details The register address is auto-incrementing.
 *           Registers are written in the order ascending from
 *           the starting address.
 *
 *  @param[in] start_reg The address of the register from which the writing
 *                       will start. Register addresses @ref st25r3911b_reg.
 *  @param[in] val Register values to write.
 *  @param[in] len Number of registers to write.
 *
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_multiple_reg_write(u8_t start_reg, u8_t *val,
				  size_t len);

/** @brief Write single NFCA reader register.
 *
 *  @param[in] reg Register address @ref st25r3911b_reg.
 *  @param[in] val Value to write.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
static inline int st25r3911b_reg_write(u8_t reg, u8_t val)
{
	return st25r3911b_multiple_reg_write(reg, &val, 1);
}

/** @brief Read multiple NFC reader registers.
 *
 *  @details The register address is auto-incrementing.
 *           Registers are read in the order ascending from
 *           the starting address.
 *
 *  @param[in] start_reg The address of the register from which the
 *                       reading will start.
 *                       Register addresses @ref st25r3911b_reg.
 *  @param[out] val Read registers values.
 *  @param[in] len Number of registers to read.
 *
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_multiple_reg_read(u8_t start_reg, u8_t *val, size_t len);

/** @brief Read single NFC reader register.
 *
 *  @param[in] reg Register address @ref st25r3911b_reg.
 *  @param[out] val Read register value.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
static inline int st25r3911b_reg_read(u8_t reg, u8_t *val)
{
	return st25r3911b_multiple_reg_read(reg, val, 1);
}

/** @brief Execute NFC reader direct command.
 *
 *  @details Direct commands are used to automate certain activities.
 *
 *  @param[in] cmd Direct command @ref st25r3911b_direct_command.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_cmd_execute(u8_t cmd);

/** @brief Modify single NFC reader register.
 *
 *  @details The first register value is read,
 *           then the bits choose by clear mask are cleared.
 *           Finally, bits choose by set mask are set.
 *
 *  @param[in] reg Register address @ref st25r3911b_reg.
 *  @param[in] clr_mask Bitmask that defines which register
 *                      bits will be cleared.
 *  @param[in] set_mask Bitmask that defines which register
 *                      bits will be set.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_reg_modify(u8_t reg, u8_t clr_mask, u8_t set_mask);

/** @brief Read NFC reader FIFO data.
 *
 *  @details Data from FIFO is read by SPI.
 *           Data length to read can be in (1 to 96 bytes) range.
 *
 *  @param[out] data Read FIFO data.
 *  @param[in] length Length of data to read
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_fifo_read(u8_t *data, size_t length);

/** @brief Write NFC reader FIFO data.
 *
 *  @details Data from FIFO is written by SPI.
 *           Data length to write can be in (1 to 96 bytes) range.
 *
 *  @param[in] data  FIFO data to write.
 *  @param[in] length Length of data to write
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_fifo_write(u8_t *data, size_t length);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_SPI_H_ */
