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

#define BIT_SET_VALUE(var, pos, value) ((var & (~(1 << pos))) | (value << pos))

#define VPRCSR_NORDIC_OUT_HIGH 1
#define VPRCSR_NORDIC_OUT_LOW  0

#define VPRCSR_NORDIC_DIR_OUTPUT 1
#define VPRCSR_NORDIC_DIR_INPUT	 0

#define BITS_IN_WORD 32
#define BITS_IN_BYTE 8

enum hrt_bit_order {
	HRT_BO_NORMAL,
	HRT_BO_REVERSED_BYTE,
	HRT_BO_REVERSED_WORD
};

/** @brief Low level transfer parameters. */
struct hrt_ll_xfer {

	/** @brief Timer initial value,
	 * 	   time which passes beetwen chip enable and first data clock
	 */
	uint16_t counter_initial_value;

	/** @brief Buffer for RX/TX data */
	uint8_t *data;

	/** @brief CEIL(buffer_length_bits/32)
	 */
	uint32_t words;

	/** @brief Amount of clock pulses for last word.
	 *         Due to hardware limitation, in case when last word clock pulse count is 1,
	 * 	   the penultimate word has to share its bits with last word,
	 * 	   for example:
	 * 		buffer length = 36bits,
	 * 		io_mode = QUAD,
	 * 		last_word_clocks would be:(buffer_length%32)/QUAD = 1
	 * 		so:
	 * 			penultimate_word_clocks = 32-BITS_IN_BYTE
	 * 			last_word_clocks = (buffer_length%32)/QUAD + BITS_IN_BYTE
	 * 			last_word = penultimate_word>>24 | last_word<<8
	 */
	uint8_t last_word_clocks;

	/** @brief  Amount of clock pulses for penultimate word.
	 * 	    For more info see last_word_clocks.
	 */
	uint8_t penultimate_word_clocks;

	/** @brief Value of last word.
	 *         For more info see last_word_clocks.
	 */
	uint32_t last_word;

	/** @brief Bit transfer order. */
	enum hrt_bit_order bit_order;

	/** @brief Number of CE VIO pin */
	uint8_t ce_vio;

	/** @brief If true chip enable pin will be left active after transfer */
	uint8_t ce_hold;

	/** @brief Chip enable pin polarity in enabled state. */
	enum mspi_ce_polarity ce_polarity;

	/** @brief When true clock signal makes 1 transition less.
	 *         It is required for spi modes 1 and 3 due to hardware issue.
	 */
	bool eliminate_last_pulse;
};

/** @brief Write.
 *
 *  Function to be used to write data on SPI.
 *
 *  @param[in] xfer_ll_params Low level transfer parameters.
 */
void hrt_write(volatile struct hrt_ll_xfer xfer_ll_params);

#endif /* _HRT_H__ */
