/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>
#include <stdbool.h>
#include <drivers/mspi/nrfe_mspi.h>
#include <zephyr/drivers/mspi.h>

#define VPRCSR_NORDIC_OUT_HIGH 1
#define VPRCSR_NORDIC_OUT_LOW  0

#define VPRCSR_NORDIC_DIR_OUTPUT 1
#define VPRCSR_NORDIC_DIR_INPUT	 0

#define BITS_IN_WORD 32
#define BITS_IN_BYTE 8

typedef enum {
	HRT_FE_COMMAND,
	HRT_FE_ADDRESS,
	HRT_FE_DATA,
	HRT_FE_MAX
} hrt_frame_element_t;

/** @brief Structure for holding bus width of different xfer parts */
typedef struct {
	uint8_t command;
	uint8_t address;
	uint8_t data;
} hrt_xfer_bus_widths_t;

typedef struct {
	/** @brief Buffer for RX/TX data */
	volatile uint8_t *data;

	/** @brief Data length in 4 byte words,
	 *         calculated as CEIL(buffer_length_bits/32).
	 */
	uint32_t word_count;

	/** @brief Amount of clock pulses for last word.
	 *         Due to hardware limitation, in case when last word clock pulse count is 1,
	 *         the penultimate word has to share its bits with last word,
	 *         for example:
	 *         buffer length = 36bits,
	 *         bus_width = QUAD,
	 *         last_word_clocks would be:(buffer_length%32)/QUAD = 1
	 *         so:
	 *                 penultimate_word_clocks = 32-BITS_IN_BYTE
	 *                 last_word_clocks = (buffer_length%32)/QUAD + BITS_IN_BYTE
	 *                 last_word = penultimate_word>>24 | last_word<<8
	 */
	uint8_t last_word_clocks;

	/** @brief  Amount of clock pulses for penultimate word.
	 *          For more info see last_word_clocks.
	 */
	uint8_t penultimate_word_clocks;

	/** @brief Value of last word.
	 *         For more info see last_word_clocks.
	 */
	uint32_t last_word;

	/** @brief Function for writing to buffered out register. */
	void (*vio_out_set)(uint32_t value);
} hrt_xfer_data_t;

/** @brief Hrt transfer parameters. */
typedef struct {

	/** @brief Data for all transfer parts */
	hrt_xfer_data_t xfer_data[HRT_FE_MAX];

	/** @brief Bus widths for different transfer parts (command, address, and data). */
	hrt_xfer_bus_widths_t bus_widths;

	/** @brief Timer value, used for setting clock frequency
	 */
	uint16_t counter_value;

	/** @brief Index of CE VIO pin */
	uint8_t ce_vio;

	/** @brief If true chip enable pin will be left active after transfer */
	bool ce_hold;

	/** @brief Chip enable pin polarity in enabled state. */
	enum mspi_ce_polarity ce_polarity;

	/** @brief When true clock signal makes 1 transition less.
	 *         It is required for spi modes 1 and 3 due to hardware issue.
	 */
	bool eliminate_last_pulse;

	/** @brief Tx mode mask for csr dir register  */
	uint16_t tx_direction_mask;

	/** @brief Rx mode mask for csr dir register  */
	uint16_t rx_direction_mask;

} hrt_xfer_t;

/** @brief Write.
 *
 *  Function to be used to write data on MSPI.
 *
 *  @param[in] hrt_xfer_params Hrt transfer parameters and data.
 */
void hrt_write(hrt_xfer_t *hrt_xfer_params);

#endif /* _HRT_H__ */
