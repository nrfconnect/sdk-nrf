/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>

#define SCLK_PIN 0
#define D0_PIN 1
#define D1_PIN 2
#define D2_PIN 3
#define D3_PIN 4
#define CS_PIN 5

/** @brief Write on single line.
 *
 *  Function to be used to write data on single data line (SPI).
 *
 *  @param[in] data Data to be sent.
 *  @param[in] data_len Length of data in words.
 *  @param[in] counter_top Top value of VTIM. This will determine clock frequency (SPI_CLOCK ~= CPU_CLOCK / (2 * TOP)).
 *  @param[in] word_size Size of a word in bits.
 */
void write_single_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top, uint8_t word_size);

/** @brief Write on single line.
 *
 *  Function to be used to write data on single data line (SPI).
 *
 *  @param[in] data Data to be sent.
 *  @param[in] data_len Length of data in words.
 *  @param[in] counter_top Top value of VTIM. This will determine clock frequency (SPI_CLOCK ~= CPU_CLOCK / (2 * TOP)).
 *  @param[in] word_size Size of a word in bits.
 */
void write_quad_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top, uint8_t word_size);

#endif /* _HRT_H__ */
