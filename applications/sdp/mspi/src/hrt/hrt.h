/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>
#include <stdbool.h>

#define SCLK_PIN 0
#define D0_PIN 1
#define D1_PIN 2
#define D2_PIN 3
#define D3_PIN 4
#define CS_PIN 5

/* Max word size. */
#define MAX_WORD_SIZE NRF_VPR_CSR_VIO_SHIFT_CNT_OUT_BUFFERED_MAX

/* Macro for getting direction mask for specified pin and direction. */
#define PIN_DIR_MASK(PIN_NUM, DIR) (VPRCSR_NORDIC_DIR_PIN##PIN_NUM##_##DIR << VPRCSR_NORDIC_DIR_PIN##PIN_NUM##_Pos)

/* Macro for getting output mask for specified pin. */
#define PIN_DIR_OUT_MASK(PIN_NUM) PIN_DIR_MASK(PIN_NUM, OUTPUT)

/* Macro for getting input mask for specified pin. */
#define PIN_DIR_IN_MASK(PIN_NUM) PIN_DIR_MASK(PIN_NUM, INPUT)

/* Macro for getting state mask for specified pin and state. */
#define PIN_OUT_MASK(PIN_NUM, STATE) (VPRCSR_NORDIC_OUT_PIN##PIN_NUM##_##STATE << VPRCSR_NORDIC_OUT_PIN##PIN_NUM##_Pos)

/* Macro for getting high state mask for specified pin. */
#define PIN_OUT_HIGH_MASK(PIN_NUM) PIN_OUT_MASK(PIN_NUM, HIGH)

/* Macro for getting low state mask for specified pin. */
#define PIN_OUT_LOW_MASK(PIN_NUM) PIN_OUT_MASK(PIN_NUM, LOW)

/** @brief Write on single line.
 *
 *  Function to be used to write data on single data line (SPI).
 *
 *  @param[in] data Data to be sent.
 *  @param[in] data_len Length of data in words.
 *  @param[in] counter_top Top value of VTIM. This will determine clock frequency (SPI_CLOCK ~= CPU_CLOCK / (2 * TOP)).
 *  @param[in] word_size Size of a word in bits.
 */
void write_single_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top, uint8_t word_size, bool ce_enable_state, bool hold_ce);

/** @brief Write on single line.
 *
 *  Function to be used to write data on single data line (SPI).
 *
 *  @param[in] data Data to be sent.
 *  @param[in] data_len Length of data in words.
 *  @param[in] counter_top Top value of VTIM. This will determine clock frequency (SPI_CLOCK ~= CPU_CLOCK / (2 * TOP)).
 *  @param[in] word_size Size of a word in bits.
 */
void write_quad_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top, uint8_t word_size, bool ce_enable_state, bool hold_ce);

#endif /* _HRT_H__ */
