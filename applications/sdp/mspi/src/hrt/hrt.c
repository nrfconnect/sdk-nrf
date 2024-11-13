/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>

#define SCLK_PIN 0
#define D0_PIN 1
#define D1_PIN 2
#define D2_PIN 3
#define D3_PIN 4
#define CS_PIN 5

#define TOP 4

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


void write_single_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top, uint8_t word_size)
{
    NRFX_ASSERT(word_size > MAX_WORD_SIZE);

    /* Configuration step */
    nrf_vpr_csr_vio_dir_set(PIN_DIR_OUT_MASK(D0_PIN) | PIN_DIR_OUT_MASK(CS_PIN) | PIN_DIR_OUT_MASK(SCLK_PIN));
    nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(CS_PIN) | PIN_OUT_LOW_MASK(SCLK_PIN) | PIN_OUT_LOW_MASK(D0_PIN));

    nrf_vpr_csr_vio_mode_out_t out_mode = {
        .mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
        .frame_width = 1,
    };

    nrf_vpr_csr_vio_mode_out_set(&out_mode);

    nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);
    nrf_vpr_csr_vio_config_t config;
    nrf_vpr_csr_vio_config_get(&config);
    config.input_sel = false;
    nrf_vpr_csr_vio_config_set(&config);

    /* Fix position of data if word size < MAX_WORD_SIZE, so that leading zeros would not be printed instead of data. */
    if (word_size < MAX_WORD_SIZE)
    {
        for (uint8_t i = 0; i < data_len; i++)
        {
            data[i] = data[i] << (MAX_WORD_SIZE - word_size);
        }
    }

    /* Counter settings */
    nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
    nrf_vpr_csr_vtim_simple_counter_top_set(0, counter_top);

    /* Set number of shifts before OUTB needs to be updated. First shift needs to be increased by 1. */
    nrf_vpr_csr_vio_shift_cnt_out_set(word_size);
    nrf_vpr_csr_vio_shift_cnt_out_buffered_set(word_size - 1);

    /* Set all pins to low state, enable CS */
    nrf_vpr_csr_vio_out_set(0);

    /* Start counter */
    nrf_vpr_csr_vtim_simple_counter_set(0, 3 * counter_top);

    /* Send data */
    for (uint8_t i = 0; i < data_len; i++)
    {
        nrf_vpr_csr_vio_out_buffered_reversed_word_set(data[i]);
    }

    /* Clear all bits, wait until the last word is sent */
    nrf_vpr_csr_vio_out_buffered_set(0);

    /* Final configuration */
    out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
    nrf_vpr_csr_vio_mode_out_buffered_set(&out_mode);
    nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

    /* Deselect slave */
    nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(CS_PIN));

    /* Stop counter */
    nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
}

void write_quad_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top, uint8_t word_size)
{

}
