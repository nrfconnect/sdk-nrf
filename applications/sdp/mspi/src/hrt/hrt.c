/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>
#include <zephyr/sys/byteorder.h>

/* Hardware requirement, to get n shifts SHIFTCNTB register has to be set to n-1*/
#define SHIFTCNTB_VALUE(shift_count) (shift_count - 1)

#define SPI_INPUT_PIN_NUM 2
#define CNT1_INIT_VALUE	  1
#define MSB_MASK	  (0xff000000)

#define BYTE_3_SHIFT 24
#define BYTE_2_SHIFT 16

#define CLK_VIO		    0
#define D0_VIO		    1
#define D1_VIO		    2
#define VIO_SINGLE_BUS_MASK 0x0004
#define VIO_QUAD_BUS_MASK   0x001E

#define IN_REG_EMPTY UINT16_MAX

/* Time in clock cycles required for RX transfer stop procedure in modes 1-3.
 * Which is min time between last two clock edges of transfer.
 */
#define LAST_WORD_PROCESSING_DELAY 35

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

NRF_STATIC_INLINE bool is_counter_running(uint8_t counter)
{
	/* This may not work for higher frequencies but
	 * there is no other way to check if timer is running.
	 * Tested it down to counter value 2 which is 21.333333MHz.
	 */
	uint16_t cnt = nrf_vpr_csr_vtim_simple_counter_get(counter);

	return cnt != nrf_vpr_csr_vtim_simple_counter_get(counter);
}

NRF_STATIC_INLINE uint32_t get_next_shift_count(volatile hrt_xfer_t *hrt_xfer_params,
						uint32_t word_count, uint32_t index)
{
	switch (word_count - index) {
	case 1: /* Last transfer */
		return SHIFTCNTB_VALUE(hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks);
	case 2: /* Last but one transfer */
		return SHIFTCNTB_VALUE(
			hrt_xfer_params->xfer_data[HRT_FE_DATA].penultimate_word_clocks);
	default:
		return SHIFTCNTB_VALUE(BITS_IN_WORD / hrt_xfer_params->bus_widths.data);
	}
}

NRF_STATIC_INLINE void rx_end_procedure_mode_3(volatile hrt_xfer_t *hrt_xfer_params)
{
	uint32_t last_word;
	uint16_t in;
	uint16_t bus_mask = VIO_SINGLE_BUS_MASK;
	uint16_t bus_position = D1_VIO;

	/* Read last word that is missing data from 1 clock pulse. */
	last_word = nrf_vpr_csr_vio_in_buffered_reversed_byte_get();

	nrf_vpr_csr_vio_mode_in_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	if (hrt_xfer_params->counter_value > LAST_WORD_PROCESSING_DELAY) {
		nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
		nrf_vpr_csr_vtim_simple_counter_set(0, hrt_xfer_params->counter_value -
							       LAST_WORD_PROCESSING_DELAY);
	}
	/* Wait for the data to appear in the IN register. */
	while (nrf_vpr_csr_vio_in_get() == IN_REG_EMPTY) {
	}

	/* Wait for correct edge bit time. */
	if (hrt_xfer_params->counter_value > LAST_WORD_PROCESSING_DELAY) {
		nrf_vpr_csr_vtim_simple_wait_set(0, false, 0);
	}

	in = nrf_vpr_csr_vio_in_get();
	nrf_vpr_csr_vio_out_or_set(BIT(CLK_VIO));

	/* Insert data from IN register into last word.
	 * At this point last_word stores all data from last word apart from last clock cycle.
	 * "in" variable stores state of all VIOs for the last clock cycle, where:
	 *  - in SINGLE mode: the data is 1 bit in the position of BIT(D1_VIO)
	 *  - in QUAD mode: the data is 4 bits starting from BIT(D0_VIO)
	 */
	if (hrt_xfer_params->bus_widths.data == 4) {
		bus_mask = VIO_QUAD_BUS_MASK;
		bus_position = D0_VIO;
	}

	last_word = (BSWAP_32(last_word) << hrt_xfer_params->bus_widths.data) |
		    ((in & bus_mask) >> bus_position);
	last_word = last_word << (BITS_IN_WORD -
				  (hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks + 1) *
					  hrt_xfer_params->bus_widths.data);
	hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word = BSWAP_32(last_word);
}

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
	uint16_t prev_out = 0;

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

	prev_out = nrf_vpr_csr_vio_out_get();

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
			nrf_vpr_csr_vio_out_clear_set(BIT(CLK_VIO));
		} else if (hrt_xfer_params->cpp_mode == MSPI_CPP_MODE_3) {
			nrf_vpr_csr_vio_out_or_set(BIT(CLK_VIO));
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
	nrf_vpr_csr_vio_shift_ctrl_t rx_shift_ctrl = {
		.shift_count = SHIFTCNTB_VALUE(BITS_IN_WORD / hrt_xfer_params->bus_widths.data),
		.out_mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = hrt_xfer_params->bus_widths.data,
		.in_mode = NRF_VPR_CSR_VIO_MODE_IN_SHIFT,
	};
	nrf_vpr_csr_vio_mode_out_t rx_out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = hrt_xfer_params->bus_widths.data,
	};
	uint32_t word_count = hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count;
	bool hold = hrt_xfer_params->ce_hold;
	uint32_t *data = (uint32_t *)hrt_xfer_params->xfer_data[HRT_FE_DATA].data;
	uint32_t last_word;
	uint16_t prev_out;

	/* Workaround for hw issue: in modes 1-3 clock does 1 extra edge after being stopped.
	 * To fix it, rx transfer is set up to do 1 clock cycle less and last clock edge is done,
	 * by writing directly to out register and reading in register.
	 */
	if (hrt_xfer_params->cpp_mode == MSPI_CPP_MODE_3) {
		hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks -= 1;
	}

	/* Enable CE */
	if (hrt_xfer_params->ce_polarity == MSPI_CE_ACTIVE_LOW) {
		nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->ce_vio));
	} else {
		nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->ce_vio));
	}

	/* Get state of all VIO to reset it correctly after transfer. */
	prev_out = nrf_vpr_csr_vio_out_get();

	if ((hrt_xfer_params->xfer_data[HRT_FE_COMMAND].word_count != 0) ||
	    (hrt_xfer_params->xfer_data[HRT_FE_ADDRESS].word_count != 0) ||
	    (hrt_xfer_params->xfer_data[HRT_FE_DUMMY_CYCLES].word_count != 0)) {
		/* Write only command, address and dummy cycles and keep CS active. */
		hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count = 0;
		hrt_xfer_params->ce_hold = true;
		hrt_write(hrt_xfer_params);
	}

	/* Restore variables values for read phase. */
	hrt_xfer_params->xfer_data[HRT_FE_DATA].word_count = word_count;
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
	nrf_vpr_csr_vio_mode_out_set(&rx_out_mode);

	/* Value of 1 is set to SHIFTCNTOUT register to start MSPI clock running, 0 is not possible.
	 * This causes 1 to be written to SHIFTCNTOUT and SHIFTCNTIN.
	 * When starting timers with nrf_vpr_csr_vtim_combined_counter_set both should start
	 * simoultaneously but it does not happen.
	 * 1. First TIMER 0 starts.
	 * 2. When SHIFTCNTOUT reaches 0, TIMER 1 starts.
	 * 3. When SHIFTCNTIN reaches 0 SHIFTCTRLB register is loaded and both timers and
	 *    shift counters continue to run together.
	 * So writing n to SHIFTCNTOUT and starting timers causes 2*n clock pulses to be generated.
	 */
	nrf_vpr_csr_vio_shift_cnt_out_set(1);

	/* Counter settings */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_count_mode_set(1, NRF_VPR_CSR_VTIM_COUNT_RELOAD);

	/* Set top counters value. CNT1 - trigger data capture every two clock cycles */
	nrf_vpr_csr_vtim_simple_counter_top_set(0, hrt_xfer_params->counter_value);
	nrf_vpr_csr_vtim_simple_counter_top_set(1,
						CNT1_TOP_CALCULATE(hrt_xfer_params->counter_value));

	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&rx_shift_ctrl);

	/* Make sure that there is no data leftover in output register. */
	nrf_vpr_csr_vio_out_buffered_reversed_word_set(0x00);

	/* Special case when only 2 clocks are required. */
	if ((word_count == 1) && (hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks == 2)) {

		/* Start reception. */
		nrf_vpr_csr_vtim_combined_counter_set(NRF_VPR_CSR_VTIM_COUNTER_VAL(
			hrt_xfer_params->counter_value, CNT1_INIT_VALUE));

		/* Wait for reception to end.
		 * Here are the solutions that should work but in this case due to high clock
		 * frequency don't.
		 * 1. Setting SHIFTCTRLB cannot be used because with RTPERIPHCTRL:STOPCOUNTERS == 1,
		 *    counter stops 1 clock too early.
		 * 2. while loop with nrf_vpr_csr_vio_shift_cnt_in_get function inside cannot be
		 *    used because at higher frequencies shifting stops before it is called the
		 *    first time.
		 * Therefore the only solution that works is to wait for timer 0 to stop.
		 * Here also the defaut way of doing that does not work,
		 * which is to use WAIT0 register, because at higher frequencies function
		 * nrf_vpr_csr_vtim_simple_wait_set is called when CNT0 has already stopped,
		 * making code wait indefinitely.
		 */
		while (is_counter_running(0)) {
		}

		/* Reset VIO outputs. */
		nrf_vpr_csr_vio_out_buffered_reversed_word_set(prev_out << BYTE_3_SHIFT);

		/* Set out mode to none so that reading INB/INBRB register doesn't cause
		 * clock to continue running.
		 */
		rx_out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
		nrf_vpr_csr_vio_mode_out_set(&rx_out_mode);

		if (hrt_xfer_params->bus_widths.data == 8) {
			last_word = nrf_vpr_csr_vio_in_buffered_reversed_byte_get();
			hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word =
				(uint16_t)(last_word >> BYTE_2_SHIFT);
		} else {
			hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word =
				(uint8_t)(nrf_vpr_csr_vio_in_buffered_reversed_byte_get() >>
					  BYTE_3_SHIFT);
		}
	} else {
		/* Get shift count for index 0.
		 * Value of 1 is set to SHIFTCNTOUT register to start MSPI clock
		 * running, 0 is not possible. Due to hardware error it causes 2*1
		 * clock pulses to be generated. After that n-2 pulses have to be
		 * generated to receive total of n bits.
		 */
		rx_shift_ctrl.shift_count =
			get_next_shift_count(hrt_xfer_params, word_count, 0) - 2;
		nrf_vpr_csr_vio_shift_ctrl_buffered_set(&rx_shift_ctrl);

		/* Get shift count for index 1. */
		rx_shift_ctrl.shift_count = get_next_shift_count(hrt_xfer_params, word_count, 1);

		nrf_vpr_csr_vtim_combined_counter_set(NRF_VPR_CSR_VTIM_COUNTER_VAL(
			hrt_xfer_params->counter_value, CNT1_INIT_VALUE));
		/* Read INBRB to continue clock beyond first 2 pulses.
		 * For higher frequencies it is important that SHIFTCTRLB is written
		 * immediately after reading INB/INBRB.
		 */
		nrf_vpr_csr_vio_in_buffered_reversed_byte_get();
		nrf_vpr_csr_vio_shift_ctrl_buffered_set(&rx_shift_ctrl);

		for (uint32_t i = 0; i < word_count - 1; i++) {
			/* Shift count was already read for indexes 0 and 1. */
			rx_shift_ctrl.shift_count =
				get_next_shift_count(hrt_xfer_params, word_count, i + 2);
			/* For higher frequencies it is important that SHIFTCTRLB is written
			 * immediately after reading INB/INBRB.
			 */
			data[i] = nrf_vpr_csr_vio_in_buffered_reversed_byte_get();
			nrf_vpr_csr_vio_shift_ctrl_buffered_set(&rx_shift_ctrl);
		}

		/* Wait for reception to end.
		 * Here are the solutions that should work but in this case due to high clock
		 * frequency don't.
		 * 1. Setting SHIFTCTRLB cannot be used because with RTPERIPHCTRL:STOPCOUNTERS == 1,
		 *    counter stops 1 clock too early.
		 * 2. while loop with nrf_vpr_csr_vio_shift_cnt_in_get function inside cannot be
		 *    used because at higher frequencies shifting stops before it is called the
		 *    first time.
		 * Therefore the only solution that works is to wait for timer 0 to stop.
		 * Here also the defaut way of doing that does not work,
		 * which is to use WAIT0 register, because at higher frequencies function
		 * nrf_vpr_csr_vtim_simple_wait_set is called when CNT0 has already stopped,
		 * making code wait indefinitely.
		 */
		while (is_counter_running(0)) {
		}
		/* Reset VIO outputs. */
		nrf_vpr_csr_vio_out_buffered_reversed_word_set(prev_out << BYTE_3_SHIFT);

		/* Set out mode to none so that reading INB/INBRB register doesn't cause
		 * clock to continue running.
		 */
		rx_out_mode.mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
		nrf_vpr_csr_vio_mode_out_set(&rx_out_mode);

		/* Workaround for hw issue: in modes 1-3 clock does 1 extra edge after stopping.
		 * To fix it, rx transfer is set up to do 1 clock cycle less and last clock edge is
		 * done, by writing directly to out register and reading in register.
		 */
		if (hrt_xfer_params->cpp_mode == MSPI_CPP_MODE_3) {
			rx_end_procedure_mode_3(hrt_xfer_params);
		} else {
			hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word =
				nrf_vpr_csr_vio_in_buffered_reversed_byte_get() >>
				(BITS_IN_WORD -
				 hrt_xfer_params->xfer_data[HRT_FE_DATA].last_word_clocks *
					 hrt_xfer_params->bus_widths.data);
		}
	}

	/* Stop counters */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);
	nrf_vpr_csr_vtim_count_mode_set(1, NRF_VPR_CSR_VTIM_COUNT_STOP);

	/* Final configuration */
	rx_shift_ctrl.out_mode = NRF_VPR_CSR_VIO_SHIFT_NONE;
	rx_shift_ctrl.in_mode = NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS;
	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&rx_shift_ctrl);

	/* Disable CS */
	if (!hrt_xfer_params->ce_hold) {

		if (hrt_xfer_params->ce_polarity == MSPI_CE_ACTIVE_LOW) {
			nrf_vpr_csr_vio_out_or_set(BIT(hrt_xfer_params->ce_vio));
		} else {
			nrf_vpr_csr_vio_out_clear_set(BIT(hrt_xfer_params->ce_vio));
		}
	}

	/* Set DQ1 back as output. */
	if (hrt_xfer_params->bus_widths.data == 1) {
		WRITE_BIT(hrt_xfer_params->tx_direction_mask, SPI_INPUT_PIN_NUM,
			  VPRCSR_NORDIC_DIR_OUTPUT);
		nrf_vpr_csr_vio_dir_set(hrt_xfer_params->tx_direction_mask);
	}
}
