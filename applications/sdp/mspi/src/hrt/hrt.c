/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>

extern volatile uint16_t irq_arg;

#define CS_BIT 5
#define SCLK_BIT 0
#define D0_BIT 1
#define D1_BIT 2
#define D2_BIT 3
#define D3_BIT 4

#define PDN_BIT 12

#define TOP 4

void write_single_by_word(uint32_t* data, uint32_t data_len, uint32_t counter_top)
{
    nrf_vpr_csr_vio_dir_set( VPRCSR_NORDIC_DIR_PIN0_OUTPUT | (0x1<<D0_BIT)+ (0x1<<CS_BIT)+ (0x1<<SCLK_BIT) ); // (sclk, D0 output);
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT)+ (0x0<<SCLK_BIT));

    nrf_vpr_csr_vio_mode_out_t out_mode = {
        .mode = NRF_VPR_CSR_VIO_SHIFT_OUTB,
        .frame_width = 1,
    };

    nrf_vpr_csr_vio_mode_out_set(&out_mode);

    nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);

    csr_write(VIO_CTRLB, ((32-1)<<CTRL_SHIFT_Pos)+ (1<<CTRL_OUTNUM_Pos) + (OUTMODE_MODE_MODE2<<CTRL_OUTMODE_Pos) + (INMODE_MODE_NORMAL<<CTRL_INMODE_Pos));

    for (uint8_t i = 0; i < data_len - 1; i++)
    {
        nrf_vpr_csr_vio_out_buffered_reversed_word_set(data[i]);
    }

    nrf_vpr_csr_vio_out_buffered_reversed_word_set(data[len - 1]);

    // VPRCSR_NORDIC_INMODE_MODE_CONTINUOUS
    out_mode.mode = VPRCSR_NORDIC_SHIFTCTRLB_OUTMODEB_MODE_NoShifting;
    nrf_vpr_csr_vio_mode_out_buffered_set(&out_mode);

    csr_write(VIO_CTRLB, (OUTMODE_MODE_NORMAL<<CTRL_OUTMODE_Pos) + (INMODE_MODE_NORMAL<<CTRL_INMODE_Pos));

    /* Clear all bits */
    nrf_vpr_csr_vio_out_buffered_set(0);

    /* Deselect slave */
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT));

    /* Stop counter */
    nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP)

}
