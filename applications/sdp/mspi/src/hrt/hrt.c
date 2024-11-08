/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>

extern volatile uint16_t irq_arg;

#define CS_BIT 5
#define SCLK_BIT 0
#define D0_BIT 1
#define D1_BIT 2
#define D2_BIT 3
#define D3_BIT 4

#define PDN_BIT 12

#define TOP 4

void write_single_by_word(uint32_t* data, uint8_t data_len, uint32_t counter_top)
{
    /* Configuration step */
    // VPRCSR_NORDIC_DIR_PIN0_OUTPUT
    nrf_vpr_csr_vio_dir_set( (0x1<<D0_BIT)+ (0x1<<CS_BIT)+ (0x1<<SCLK_BIT) ); // (sclk, D0 output);
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT)+ (0x0<<SCLK_BIT) + (0 << D0_BIT) );

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

    /* Conter settings */
    nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
    nrf_vpr_csr_vtim_simple_counter_top_set(0, TOP);

    /* Set number of shifts before OUTB needs to be updated */
    nrf_vpr_csr_vio_shift_cnt_out_buffered_set(31);

    /* set all pins to low state, enable CS */
    nrf_vpr_csr_vio_out_set(0);

    /* Start counter */
    nrf_vpr_csr_vtim_combined_counter_set(2 * TOP);
    // nrf_vpr_csr_vtim_simple_counter_set(0, TOP);

    /* Send data */
    for (uint8_t i = 0; i < data_len - 1; i++)
    {
        nrf_vpr_csr_vio_out_buffered_reversed_byte_set(data[i]);
    }

    nrf_vpr_csr_vio_shift_cnt_out_buffered_set(31);
    nrf_vpr_csr_vio_out_buffered_reversed_byte_set(data[data_len - 1]);

    /* Final configuration */
    out_mode.mode = VPRCSR_NORDIC_SHIFTCTRLB_OUTMODEB_MODE_NoShifting;
    nrf_vpr_csr_vio_mode_out_buffered_set(&out_mode);
    nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

    /* Clear all bits */
    nrf_vpr_csr_vio_out_buffered_set(0);

    /* Deselect slave */
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT));

    /* Stop counter */
    nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);

}
