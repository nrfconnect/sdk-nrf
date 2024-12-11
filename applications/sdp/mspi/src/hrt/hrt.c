/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>
#include <drivers/mspi/nrfe_mspi.h>
#include <zephyr/drivers/mspi.h>

void hrt_write(volatile struct hrt_ll_xfer xfer_ll_params)
{
	uint8_t shift_count;
	uint16_t out;
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_NONE,
		.frame_width = 1,
	};

	NRFX_ASSERT((xfer_ll_params.last_word_clocks != 1) || (xfer_ll_params.words == 1))

	/* Enable CS */
	out = nrf_vpr_csr_vio_out_get();

	if (xfer_ll_params.ce_polarity == MSPI_CE_ACTIVE_LOW) {
		out = BIT_SET_VALUE(out, xfer_ll_params.ce_vio, VPRCSR_NORDIC_OUT_LOW);
	} else {
		out = BIT_SET_VALUE(out, xfer_ll_params.ce_vio, VPRCSR_NORDIC_OUT_HIGH);
	}
	nrf_vpr_csr_vio_out_set(out);

	for (uint32_t i = 0; i < xfer_ll_params.words; i++) {
		switch (xfer_ll_params.words - i) {
		case 1: /* Last transfer */
			nrf_vpr_csr_vio_shift_cnt_out_buffered_set(xfer_ll_params.last_word_clocks -
								   1);
			switch (xfer_ll_params.bit_order) {
			case HRT_BO_NORMAL:
				nrf_vpr_csr_vio_out_buffered_set(xfer_ll_params.last_word);
				break;
			case HRT_BO_REVERSED_BYTE:
				nrf_vpr_csr_vio_out_buffered_reversed_byte_set(
					xfer_ll_params.last_word);
				break;
			case HRT_BO_REVERSED_WORD:
				nrf_vpr_csr_vio_out_buffered_reversed_word_set(
					xfer_ll_params.last_word);
				break;
			}
			break;
		case 2: /* Last but one transfer.*/
			nrf_vpr_csr_vio_shift_cnt_out_buffered_set(
				xfer_ll_params.penultimate_word_clocks - 1);

			/* Intentional fallthrough. */
		default:
			switch (xfer_ll_params.bit_order) {
			case HRT_BO_NORMAL:
				nrf_vpr_csr_vio_out_buffered_set(
					((uint32_t *)xfer_ll_params.data)[i]);
				break;
			case HRT_BO_REVERSED_BYTE:
				nrf_vpr_csr_vio_out_buffered_reversed_byte_set(
					((uint32_t *)xfer_ll_params.data)[i]);
				break;
			case HRT_BO_REVERSED_WORD:
				nrf_vpr_csr_vio_out_buffered_reversed_word_set(
					((uint32_t *)xfer_ll_params.data)[i]);
				break;
			}
		}

		if (i == 0) {
			/* Start counter */
			nrf_vpr_csr_vtim_simple_counter_set(0,
							    xfer_ll_params.counter_initial_value);
			/* Wait for first clock cycle */
			shift_count = nrf_vpr_csr_vio_shift_cnt_out_get();
			while (nrf_vpr_csr_vio_shift_cnt_out_get() == shift_count) {
			}
		}
	}

	/* Clear all bits, wait until the last word is sent */
	nrf_vpr_csr_vio_out_buffered_set(0);

	/* Final configuration */
	nrf_vpr_csr_vio_mode_out_buffered_set(&out_mode);
	nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	/* Disable CS */
	if (!xfer_ll_params.ce_hold) {

		out = nrf_vpr_csr_vio_out_get();

		if (xfer_ll_params.ce_polarity == MSPI_CE_ACTIVE_LOW) {
			out = BIT_SET_VALUE(out, xfer_ll_params.ce_vio, VPRCSR_NORDIC_OUT_HIGH);
		} else {
			out = BIT_SET_VALUE(out, xfer_ll_params.ce_vio, VPRCSR_NORDIC_OUT_LOW);
		}
		nrf_vpr_csr_vio_out_set(out);
	}

	/* Stop counter */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
}