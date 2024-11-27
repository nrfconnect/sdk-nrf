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

/* Max word size. */
#define MAX_WORD_SIZE NRF_VPR_CSR_VIO_SHIFT_CNT_OUT_BUFFERED_MAX

/* Macro for getting direction mask for specified pin and direction. */
#define PIN_DIR_MASK(PIN_NUM, DIR)                                                                 \
	(VPRCSR_NORDIC_DIR_PIN##PIN_NUM##_##DIR << VPRCSR_NORDIC_DIR_PIN##PIN_NUM##_Pos)

/* Macro for getting output mask for specified pin. */
#define PIN_DIR_OUT_MASK(PIN_NUM) PIN_DIR_MASK(PIN_NUM, OUTPUT)

/* Macro for getting input mask for specified pin. */
#define PIN_DIR_IN_MASK(PIN_NUM) PIN_DIR_MASK(PIN_NUM, INPUT)

/* Macro for getting state mask for specified pin and state. */
#define PIN_OUT_MASK(PIN_NUM, STATE)                                                               \
	(VPRCSR_NORDIC_OUT_PIN##PIN_NUM##_##STATE << VPRCSR_NORDIC_OUT_PIN##PIN_NUM##_Pos)

/* Macro for getting high state mask for specified pin. */
#define PIN_OUT_HIGH_MASK(PIN_NUM) PIN_OUT_MASK(PIN_NUM, HIGH)

/* Macro for getting low state mask for specified pin. */
#define PIN_OUT_LOW_MASK(PIN_NUM) PIN_OUT_MASK(PIN_NUM, LOW)

/** @brief Low level transfer parameters. */
struct hrt_ll_xfer {
	/** @brief Top value of VTIM. This will determine clock frequency
	 *                         (SPI_CLOCK ~= CPU_CLOCK / (2 * TOP)).
	 */
	volatile uint8_t counter_top;

	/** @brief Word size of passed data, bits. */
	volatile uint8_t word_size;

	/** @brief Data to send, under each index there is data of length word_size. */
	volatile uint32_t *data_to_send;

	/** @brief Data length. */
	volatile uint8_t data_len;

	/** @brief If true chip enable pin will be left active after transfer */
	volatile uint8_t ce_hold;

	/** @brief Chip enable pin polarity in enabled state. */
	volatile bool ce_enable_state;
};

/** @brief Write on single line.
 *
 *  Function to be used to write data on single data line (SPI).
 *
 *  @param[in] xfer_ll_params Low level transfer parameters.
 */
void write_single_by_word(volatile struct hrt_ll_xfer xfer_ll_params);

/** @brief Write on four lines.
 *
 *  Function to be used to write data on quad data line (SPI).
 *
 *  @param[in] xfer_ll_params Low level transfer parameters.
 */
void write_quad_by_word(volatile struct hrt_ll_xfer xfer_ll_params);

#endif /* _HRT_H__ */
