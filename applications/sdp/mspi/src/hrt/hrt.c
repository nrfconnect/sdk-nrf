/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>

#define CLK_FIRST_CYCLE_MULTIPLICATOR (3)

void write_single_by_word(volatile struct hrt_ll_xfer xfer_ll_params)
{
	uint16_t dir;
	uint16_t out;
	nrf_vpr_csr_vio_config_t config;
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = 1,
	};

	NRFX_ASSERT(xfer_ll_params.word_size <= MAX_WORD_SIZE);
	/* Configuration step */
	dir = nrf_vpr_csr_vio_dir_get();
	nrf_vpr_csr_vio_dir_set(dir | PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_DQ0_PIN_NUMBER)));

	out = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(out | PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_DQ0_PIN_NUMBER)));

	nrf_vpr_csr_vio_mode_out_set(&out_mode);
	nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	nrf_vpr_csr_vio_config_get(&config);
	config.input_sel = false;
	nrf_vpr_csr_vio_config_set(&config);

	/* Fix position of data if word size < MAX_WORD_SIZE,
	 * so that leading zeros would not be printed instead of data bits.
	 */
	if (xfer_ll_params.word_size < MAX_WORD_SIZE) {
		for (uint8_t i = 0; i < xfer_ll_params.data_len; i++) {
			xfer_ll_params.data_to_send[i] =
				xfer_ll_params.data_to_send[i]
				<< (MAX_WORD_SIZE - xfer_ll_params.word_size);
		}
	}

	/* Counter settings */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_simple_counter_top_set(0, xfer_ll_params.counter_top);

	/* Set number of shifts before OUTB needs to be updated.
	 * First shift needs to be increased by 1.
	 */
	nrf_vpr_csr_vio_shift_cnt_out_set(xfer_ll_params.word_size);
	nrf_vpr_csr_vio_shift_cnt_out_buffered_set(xfer_ll_params.word_size - 1);

	/* Enable CS */
	out = nrf_vpr_csr_vio_out_get();
	out &= ~PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER));
	out |= xfer_ll_params.ce_enable_state ? PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER))
					      : PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER));
	nrf_vpr_csr_vio_out_set(out);

	/* Start counter */
	nrf_vpr_csr_vtim_simple_counter_set(0, CLK_FIRST_CYCLE_MULTIPLICATOR *
						       xfer_ll_params.counter_top);

	/* Send data */
	for (uint8_t i = 0; i < xfer_ll_params.data_len; i++) {
		nrf_vpr_csr_vio_out_buffered_reversed_byte_set(xfer_ll_params.data_to_send[i]);
	}

	/* Clear all bits, wait until the last word is sent */
	nrf_vpr_csr_vio_out_buffered_set(0);

	/* Final configuration */
	out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
	nrf_vpr_csr_vio_mode_out_buffered_set(&out_mode);
	nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	/* Disable CS */
	if (!xfer_ll_params.ce_hold) {
		out = nrf_vpr_csr_vio_out_get();
		out &= ~(PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER)) |
			 PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));
		out |= xfer_ll_params.ce_enable_state
			       ? PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER))
			       : PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER));
		nrf_vpr_csr_vio_out_set(out);
	}

	/* Stop counter */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
}

void write_quad_by_word(volatile struct hrt_ll_xfer xfer_ll_params)
{
	uint16_t dir;
	uint16_t out;
	nrf_vpr_csr_vio_config_t config;
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = 4,
	};

	NRFX_ASSERT(xfer_ll_params.word_size % 4 == 0);
	NRFX_ASSERT(xfer_ll_params.word_size <= MAX_WORD_SIZE);
	/* Configuration step */
	dir = nrf_vpr_csr_vio_dir_get();

	nrf_vpr_csr_vio_dir_set(dir | PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_DQ0_PIN_NUMBER)) |
				PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_DQ1_PIN_NUMBER)) |
				PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_DQ2_PIN_NUMBER)) |
				PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_DQ3_PIN_NUMBER)));

	out = nrf_vpr_csr_vio_out_get();

	nrf_vpr_csr_vio_out_set(out | PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_DQ0_PIN_NUMBER)) |
				PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_DQ1_PIN_NUMBER)) |
				PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_DQ2_PIN_NUMBER)) |
				PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_DQ3_PIN_NUMBER)));

	nrf_vpr_csr_vio_mode_out_set(&out_mode);
	nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	nrf_vpr_csr_vio_config_get(&config);
	config.input_sel = false;
	nrf_vpr_csr_vio_config_set(&config);

	/* Fix position of data if word size < MAX_WORD_SIZE,
	 * so that leading zeros would not be printed instead of data.
	 */
	if (xfer_ll_params.word_size < MAX_WORD_SIZE) {
		for (uint8_t i = 0; i < xfer_ll_params.data_len; i++) {
			xfer_ll_params.data_to_send[i] =
				xfer_ll_params.data_to_send[i]
				<< (MAX_WORD_SIZE - xfer_ll_params.word_size);
		}
	}

	/* Counter settings */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_simple_counter_top_set(0, xfer_ll_params.counter_top);

	/* Set number of shifts before OUTB needs to be updated.
	 * First shift needs to be increased by 1.
	 */
	nrf_vpr_csr_vio_shift_cnt_out_set(xfer_ll_params.word_size / 4);
	nrf_vpr_csr_vio_shift_cnt_out_buffered_set(xfer_ll_params.word_size / 4 - 1);

	/* Enable CS */
	out = nrf_vpr_csr_vio_out_get();
	out &= ~PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER));
	out |= xfer_ll_params.ce_enable_state ? PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER))
					      : PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER));
	nrf_vpr_csr_vio_out_set(out);

	/* Start counter */
	nrf_vpr_csr_vtim_simple_counter_set(0, 3 * xfer_ll_params.counter_top);

	/* Send data */
	for (uint8_t i = 0; i < xfer_ll_params.data_len; i++) {
		nrf_vpr_csr_vio_out_buffered_reversed_byte_set(xfer_ll_params.data_to_send[i]);
	}

	/* Clear all bits, wait until the last word is sent */
	nrf_vpr_csr_vio_out_buffered_set(0);

	/* Final configuration */
	out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
	nrf_vpr_csr_vio_mode_out_buffered_set(&out_mode);
	nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	/* Disable CS */
	if (!xfer_ll_params.ce_hold) {
		out = nrf_vpr_csr_vio_out_get();
		out &= ~(PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER)) |
			 PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));
		out |= xfer_ll_params.ce_enable_state
			       ? PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER))
			       : PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER));
		nrf_vpr_csr_vio_out_set(out);
	}

	/* Stop counter */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
}
