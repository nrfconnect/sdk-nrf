/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>

/* Hardware requirement, to get n shifts SHIFTCNTB register has to be set to n-1*/
#define SHIFTCNTB_VALUE(shift_count) (shift_count - 1)

#define SPI_INPUT_PIN_NUM 2
#define CNT1_INIT_VALUE	  1
#define MSB_MASK	  (0xff000000)

#define LAST_OUT_DATA_SHIFT 24

/*
 * Macro for calculating TOP value of CNT1. It should be twice as TOP value of CNT0
 * so that input was sampled on the other clock edge than sending.
 * Subtraction of 1 is needed, because value written to the register should be
 * equal to N - 1, where N is the desired value. For the same reason value of
 * cnt0_top needs to be increased by 1 first - it is already in form that should
 * be written to a CNT0 TOP register.
 */
#define CNT1_TOP_CALCULATE(cnt0_top) (2 * ((cnt0_top) + 1) - 1)

static const nrf_vpr_csr_vio_shift_ctrl_t write_final_shift_ctrl_cfg = {
	.shift_count = 1,
	.out_mode = NRF_VPR_CSR_VIO_SHIFT_NONE,
	.frame_width = 1,
	.in_mode = NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS,
};

static nrf_vpr_csr_vio_shift_ctrl_t xfer_shift_ctrl = {
	.shift_count = SHIFTCNTB_VALUE(BITS_IN_WORD),
	.out_mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
	.frame_width = 1,
	.in_mode = NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS,
};

static void hrt_tx(volatile hrt_xfer_data_t *xfer_data, uint8_t frame_width, bool *counter_running,
		   uint16_t counter_value)
{
	if (xfer_data->word_count == 0) {
		return;
	}

	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = frame_width,
	};
	uint8_t prev_frame_width = xfer_shift_ctrl.frame_width;
	uint32_t data;

	xfer_shift_ctrl.shift_count = SHIFTCNTB_VALUE(BITS_IN_WORD / frame_width);
	xfer_shift_ctrl.frame_width = frame_width;

	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&xfer_shift_ctrl);

	for (uint32_t i = 0; i < xfer_data->word_count; i++) {

		switch (xfer_data->word_count - i) {
		case 1: /* Last transfer */
			xfer_shift_ctrl.shift_count = SHIFTCNTB_VALUE(xfer_data->last_word_clocks);
			nrf_vpr_csr_vio_shift_ctrl_buffered_set(&xfer_shift_ctrl);

			data = xfer_data->last_word;
			break;
		case 2: /* Last but one transfer.*/
			xfer_shift_ctrl.shift_count =
				SHIFTCNTB_VALUE(xfer_data->penultimate_word_clocks);
			nrf_vpr_csr_vio_shift_ctrl_buffered_set(&xfer_shift_ctrl);
		default: /* Intentional fallthrough */
			if (xfer_data->data == NULL) {
				data = 0;
			} else {
				data = ((uint32_t *)xfer_data->data)[i];
			}
		}

		/* In case of bus width change device has to load
		 * new bus width before loading new out data.
		 */
		if (prev_frame_width != frame_width) {
			prev_frame_width = frame_width;
			while (nrf_vpr_csr_vio_shift_cnt_out_get() != 0) {
			}
			nrf_vpr_csr_vio_mode_out_set(&out_mode);
		}

		switch (xfer_data->fun_out) {
		case HRT_FUN_OUT_WORD:
			nrf_vpr_csr_vio_out_buffered_reversed_word_set(data);
			break;
		case HRT_FUN_OUT_BYTE:
			nrf_vpr_csr_vio_out_buffered_reversed_byte_set(data);
			break;
		default:
			break;
		}

		if ((i == 0) && (!*counter_running)) {
			/*
			 * Temporary fix for max frequency. Top value can be set to 0,
			 * but initial value cannot, because counter will not start.
			 */
			if (counter_value == 0) {
				counter_value = 1;
			}
			/* Start counter */
			nrf_vpr_csr_vtim_simple_counter_set(0, counter_value);
			*counter_running = true;
		}
	}
}

void hrt_write(volatile hrt_xfer_t *hrt_xfer_params)
{
	hrt_frame_element_t first_element = HRT_FE_DATA;
	bool counter_running = false;

	nrf_vpr_csr_vio_mode_out_t out_mode = {.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE};

	/* Configure clock and pins */
	nrf_vpr_csr_vio_dir_set(hrt_xfer_params->tx_direction_mask);

	for (uint8_t i = 0; i < HRT_FE_MAX; i++) {

		if (hrt_xfer_params->xfer_data[i].word_count != 0) {
			first_element = i;
			break;
		}
	}

	switch (first_element) {
	case HRT_FE_COMMAND:
		out_mode.frame_width = hrt_xfer_params->bus_widths.command;
		break;
	case HRT_FE_ADDRESS:
		out_mode.frame_width = hrt_xfer_params->bus_widths.address;
		break;
	case HRT_FE_DATA:
		out_mode.frame_width = hrt_xfer_params->bus_widths.data;
		break;
	default:
		break;
	}
	xfer_shift_ctrl.frame_width = out_mode.frame_width;

	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_simple_counter_top_set(0, hrt_xfer_params->counter_value);
	nrf_vpr_csr_vio_mode_in_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	nrf_vpr_csr_vio_mode_out_set(&out_mode);

	switch (hrt_xfer_params->xfer_data[first_element].word_count) {
	case 1:
		nrf_vpr_csr_vio_shift_cnt_out_set(
			hrt_xfer_params->xfer_data[first_element].last_word_clocks);
		break;
	case 2:
		nrf_vpr_csr_vio_shift_cnt_out_set(
			hrt_xfer_params->xfer_data[first_element].penultimate_word_clocks);
		break;
	default:
		nrf_vpr_csr_vio_shift_cnt_out_set(BITS_IN_WORD / out_mode.frame_width);
	}

	/* Enable CE */
	if (hrt_xfer_params->ce_polarity == MSPI_CE_ACTIVE_LOW) {
		nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->ce_vio));
	} else {
		nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->ce_vio));
	}

	uint16_t prev_out = nrf_vpr_csr_vio_out_get();

	/* Transfer command */
	hrt_tx(&hrt_xfer_params->xfer_data[HRT_FE_COMMAND], hrt_xfer_params->bus_widths.command,
	       &counter_running, hrt_xfer_params->counter_value);
	/* Transfer address */
	hrt_tx(&hrt_xfer_params->xfer_data[HRT_FE_ADDRESS], hrt_xfer_params->bus_widths.address,
	       &counter_running, hrt_xfer_params->counter_value);
	/* Transfer dummy cycles */
	hrt_tx(&hrt_xfer_params->xfer_data[HRT_FE_DUMMY_CYCLES],
	       hrt_xfer_params->bus_widths.dummy_cycles, &counter_running,
	       hrt_xfer_params->counter_value);
	/* Transfer data */
	hrt_tx(&hrt_xfer_params->xfer_data[HRT_FE_DATA], hrt_xfer_params->bus_widths.data,
	       &counter_running, hrt_xfer_params->counter_value);

	/* Hardware issue workaround,
	 * additional clock edge when transmitting in modes other than MSPI_CPP_MODE_0.
	 * modes 1 and 3: Disable clock before the last pulse and perform last clock edge manualy.
	 * mode 2: Add one pulse more to the last word in message, and disable clock before the last
	 * pulse.
	 */
	if (hrt_xfer_params->cpp_mode == MSPI_CPP_MODE_0) {
		nrf_vpr_csr_vio_shift_ctrl_buffered_set(&write_final_shift_ctrl_cfg);
		/*
		 * Least significant bit is ignored when the whole OUTB is shifted to OUT at once
		 * after switching to no shifting mode, so the read value need to be shifted left
		 * by 1.
		 */
		nrf_vpr_csr_vio_out_buffered_set(prev_out << 1);
		nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
	} else {
		while (nrf_vpr_csr_vio_shift_cnt_out_get() != 0) {
		}
		nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);

		nrf_vpr_csr_vio_shift_ctrl_buffered_set(&write_final_shift_ctrl_cfg);

		if (hrt_xfer_params->cpp_mode == MSPI_CPP_MODE_1) {
			nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->clk_vio));
		} else if (hrt_xfer_params->cpp_mode == MSPI_CPP_MODE_3) {
			nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->clk_vio));
		}
	}

	/* Reset counter 0, Next message may be sent incorrectly if counter is not reset here. */
	nrf_vpr_csr_vtim_simple_counter_set(0, 0);

	/* Disable CE */
	if (!hrt_xfer_params->ce_hold) {

		if (hrt_xfer_params->ce_polarity == MSPI_CE_ACTIVE_LOW) {
			nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->ce_vio));
		} else {
			nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->ce_vio));
		}
	}
}

void hrt_read(volatile hrt_xfer_t *hrt_xfer_params)
{
	nrf_vpr_csr_vio_shift_ctrl_t shift_ctrl = {
		.shift_count = SHIFTCNTB_VALUE(BITS_IN_WORD / hrt_xfer_params->bus_widths.data),
		.out_mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = hrt_xfer_params->bus_widths.data,
		.in_mode = NRF_VPR_CSR_VIO_MODE_IN_SHIFT,
	};
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = hrt_xfer_params->bus_widths.data,
	};
	uint32_t data_length = hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count;
	bool hold = hrt_xfer_params->ce_hold;
	uint32_t *data = (uint32_t *)hrt_xfer_params->xfer_data[HRT_FE_DATA].data;
	uint32_t penultimate_word_bits =
		hrt_xfer_params->xfer_data[HRT_FE_DATA].penultimate_word_clocks *
		hrt_xfer_params->bus_widths.data;
	uint32_t last_word_bits = hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks *
				  hrt_xfer_params->bus_widths.data;

	/* Enable CE */
	if (hrt_xfer_params->ce_polarity == MSPI_CE_ACTIVE_LOW) {
		nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->ce_vio));
	} else {
		nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->ce_vio));
	}

	/* Get state of all VIO to reset it correctly after transfer. */
	volatile uint16_t prev_out = nrf_vpr_csr_vio_out_get();

	/* Write only command address and dummy cycles and keep CS active. */
	hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count = 0;
	hrt_xfer_params->ce_hold = true;
	hrt_write(hrt_xfer_params);

	/* Restore variables values for read phase. */
	hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count = data_length;
	hrt_xfer_params->ce_hold = hold;

	/* Configure clock and pins */
	if (hrt_xfer_params->bus_widths.data == 1) {
		/* Set DQ1 as input */
		WRITE_BIT(hrt_xfer_params->tx_direction_mask, SPI_INPUT_PIN_NUM,
			  VPRCSR_NORDIC_DIR_INPUT);
		nrf_vpr_csr_vio_dir_set(hrt_xfer_params->tx_direction_mask);
	} else {
		nrf_vpr_csr_vio_dir_set(hrt_xfer_params->rx_direction_mask);
	}

	/* Initial configuration */
	nrf_vpr_csr_vio_mode_in_set(NRF_VPR_CSR_VIO_MODE_IN_SHIFT);
	nrf_vpr_csr_vio_mode_out_set(&out_mode);

	/* Value of 1 is set to SHIFTCNTOUT register to start MSPI clock running, 0 is not possible.
	 * Due to hardware error it causes 2*1 clock pulses to be generated.
	 */
	nrf_vpr_csr_vio_shift_cnt_out_set(1);

	/* Counter settings */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_count_mode_set(1, NRF_VPR_CSR_VTIM_COUNT_RELOAD);

	/* Set top counters value. CNT1 - trigger data capture every two clock cycles */
	nrf_vpr_csr_vtim_simple_counter_top_set(0, hrt_xfer_params->counter_value);
	nrf_vpr_csr_vtim_simple_counter_top_set(1,
						CNT1_TOP_CALCULATE(hrt_xfer_params->counter_value));

	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&shift_ctrl);

	/* Make sure that there is no data leftover in output register. */
	nrf_vpr_csr_vio_out_buffered_reversed_word_set(0x00);

	/* Special case when only 2 clocks are required. */
	if ((hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count == 1) &&
	    (hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks == 2)) {

		/* Start reception. */
		nrf_vpr_csr_vtim_combined_counter_set(
			(hrt_xfer_params->counter_value << VPRCSR_NORDIC_CNT_CNT0_Pos) +
			(CNT1_INIT_VALUE << VPRCSR_NORDIC_CNT_CNT1_Pos));

		/* Wait for reception to end, transfer has to be stopped "manualy".
		 * Setting SHIFTCTRLB cannot be used because with RTPERIPHCTRL:STOPCOUNTERS == 1,
		 * counter stops 1 clock to early.
		 */
		while (nrf_vpr_csr_vio_shift_cnt_in_get() > 0) {
		}
		/* Wait until timer 0 stops.
		 * WAIT0 cannot be used for this because in higher frequencies function
		 * nrf_vpr_csr_vtim_simple_wait_set is called when CNT0 has already stopped,
		 * making code wait indefinitely.
		 */
		uint16_t cnt0 = nrf_vpr_csr_vtim_simple_counter_get(0);
		uint16_t cnt0_prev = cnt0 + 1;

		while (cnt0 != cnt0_prev) {
			cnt0_prev = cnt0;
			cnt0 = nrf_vpr_csr_vtim_simple_counter_get(0);
		}

		/* Reset VIO outputs. */
		nrf_vpr_csr_vio_out_buffered_reversed_word_set(prev_out << LAST_OUT_DATA_SHIFT);

		/* Set out mode to none so that reading INB/INBRB register doesn't cause
		 * clock to continue running.
		 */
		out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
		nrf_vpr_csr_vio_mode_out_set(&out_mode);

		hrt_xfer_params->xfer_data[HRT_FE_DATA].data[0] =
			(uint8_t)(nrf_vpr_csr_vio_in_buffered_reversed_byte_get() >>
				  (sizeof(uint16_t) * BITS_IN_BYTE + last_word_bits));

	} else {
		uint32_t i = 0;

		for (; i < hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count; i++) {

			switch (hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count - i) {
			case 1: /* Last transfer */
				shift_ctrl.shift_count = SHIFTCNTB_VALUE(
					hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks);
				break;
			case 2: /* Last but one transfer */
				shift_ctrl.shift_count =
					SHIFTCNTB_VALUE(hrt_xfer_params->xfer_data[HRT_FE_DATA]
								.penultimate_word_clocks);
				break;
			default:
				shift_ctrl.shift_count = SHIFTCNTB_VALUE(
					BITS_IN_WORD / hrt_xfer_params->bus_widths.data);
			}

			if (i == 0) {
				/* Value of 1 is set to SHIFTCNTOUT register to start MSPI clock
				 * running, 0 is not possible. Due to hardware error it causes 2*1
				 * clock pulses to be generated. After that n-2 pulses have to be
				 * generated to receive total of n bits
				 */
				shift_ctrl.shift_count -= 2;
				nrf_vpr_csr_vio_shift_ctrl_buffered_set(&shift_ctrl);

				nrf_vpr_csr_vtim_combined_counter_set(
					(hrt_xfer_params->counter_value
					 << VPRCSR_NORDIC_CNT_CNT0_Pos) +
					(CNT1_INIT_VALUE << VPRCSR_NORDIC_CNT_CNT1_Pos));
				/* Read INBRB to continue clock beyond first 2 pulses. */
				nrf_vpr_csr_vio_in_buffered_reversed_byte_get();

			} else {
				nrf_vpr_csr_vio_shift_ctrl_buffered_set(&shift_ctrl);

				data[i - 1] = nrf_vpr_csr_vio_in_buffered_reversed_byte_get();
			}
		}

		/* Wait for reception to end, transfer has to be stopped "manualy".
		 * Setting SHIFTCTRLB cannot be used because with RTPERIPHCTRL:STOPCOUNTERS == 1,
		 * counter stops 1 clock to early.
		 */
		while (nrf_vpr_csr_vio_shift_cnt_in_get() > 0) {
		}
		/* Wait until timer 0 stops.
		 * WAIT0 cannot be used for this because in higher frequencies function
		 * nrf_vpr_csr_vtim_simple_wait_set is called when CNT0 has already stopped,
		 * making code wait indefinitely.
		 */
		uint16_t cnt0 = nrf_vpr_csr_vtim_simple_counter_get(0);
		uint16_t cnt0_prev = cnt0 + 1;

		while (cnt0 != cnt0_prev) {
			cnt0_prev = cnt0;
			cnt0 = nrf_vpr_csr_vtim_simple_counter_get(0);
		}

		/* Reset VIO outputs. */
		nrf_vpr_csr_vio_out_buffered_reversed_word_set(prev_out << LAST_OUT_DATA_SHIFT);

		/* Set out mode to none so that reading INB/INBRB register doesn't cause
		 * clock to continue running.
		 */
		out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
		nrf_vpr_csr_vio_mode_out_set(&out_mode);

		uint32_t last_word = nrf_vpr_csr_vio_in_buffered_reversed_byte_get() >>
				     (BITS_IN_WORD - last_word_bits);
		uint32_t penultimate_word_shift = BITS_IN_WORD - penultimate_word_bits;

		/* In case when last word is too short, penultimate word has to give it 1 byte.
		 * this is here to pass this byte back to penultimate word to avoid holes.
		 */
		if ((penultimate_word_shift != 0) && (i > 1)) {
			uint32_t penultimate_word = data[i - 2];

			penultimate_word = (penultimate_word >> (penultimate_word_shift)) |
					   (last_word << penultimate_word_bits);
			last_word = last_word >> penultimate_word_shift;
			data[i - 2] = penultimate_word;
		}

		/* This is to avoid writing outside of data buffer in case when buffer_length%4 !=
		 * 0.
		 */
		switch (NRFX_CEIL_DIV(last_word_bits - penultimate_word_shift, BITS_IN_BYTE)) {
		case 4:
			data[i - 1] = last_word;
			break;
		case 3:
			((uint8_t *)&(data[i - 1]))[sizeof(uint16_t)] =
				(uint8_t)(last_word >> (sizeof(uint16_t) * BITS_IN_BYTE));
		case 2: /* Intentional fallthrough. */
			*((uint16_t *)&(data[i - 1])) = (uint16_t)last_word;
			break;
		case 1:
			*((uint8_t *)&(data[i - 1])) = (uint8_t)last_word;
		}
	}

	/* Stop counters */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
	nrf_vpr_csr_vtim_count_mode_set(1, NRF_VPR_CSR_VTIM_COUNT_STOP);

	/* Final configuration */
	shift_ctrl.out_mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
	shift_ctrl.in_mode = NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS;
	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&shift_ctrl);

	/* Disable CS */
	if (!hrt_xfer_params->ce_hold) {

		if (hrt_xfer_params->ce_polarity == MSPI_CE_ACTIVE_LOW) {
			nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->ce_vio));
		} else {
			nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->ce_vio));
		}
	}

	/* Set DQ1 back as output. */
	WRITE_BIT(hrt_xfer_params->tx_direction_mask, SPI_INPUT_PIN_NUM, VPRCSR_NORDIC_DIR_OUTPUT);
	nrf_vpr_csr_vio_dir_set(hrt_xfer_params->tx_direction_mask);
}
