/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hrt/hrt.h"

#include <zephyr/drivers/mspi.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>

#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>
#include <hal/nrf_timer.h>
#include <haly/nrfy_gpio.h>

#include <drivers/mspi/hpf_mspi.h>

#define SUPPORTED_IO_MODES_COUNT 7

#define DEVICES_MAX   5
#define DATA_PINS_MAX 8

/* Bellow this CNT0 period pin steering force has to be increased to produce correct waveform.
 * CNT0 value 1 generates 32MHz clock.
 */
#define STD_PAD_BIAS_CNT0_THRESHOLD 1

/* Max RX frequency is 21.333333MHz which corresponds to counter period value of 2. */
#define RX_CNT0_MIN_VALUE 2

#define MODE_3_RX_MIN_CLOCKS 3

#define PAD_BIAS_VALUE 1

#define MAX_SHIFT_COUNT 63

#define DATA_PIN_UNUSED UINT8_MAX
#define CE_PIN_UNUSED   UINT8_MAX

#define HRT_IRQ_PRIORITY    2
#define HRT_VEVIF_IDX_READ  17
#define HRT_VEVIF_IDX_WRITE 18

#define VEVIF_IRQN(vevif)   VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

#ifndef CONFIG_SOC_NRF54L15
#error "Unsupported SoC for HPF MSPI"
#endif

#define DATA_LINE_INDEX(pinctr_fun) (pinctr_fun - NRF_FUN_HPF_MSPI_DQ0)

BUILD_ASSERT(CONFIG_HPF_MSPI_MAX_RESPONSE_SIZE > 0, "Response max size should be greater that 0");

static const uint8_t pin_to_vio_map[HPF_MSPI_PINS_MAX] = {
	4,  /* Physical pin 0 */
	0,  /* Physical pin 1 */
	1,  /* Physical pin 2 */
	3,  /* Physical pin 3 */
	2,  /* Physical pin 4 */
	5,  /* Physical pin 5 */
	6,  /* Physical pin 6 */
	7,  /* Physical pin 7 */
	8,  /* Physical pin 8 */
	9,  /* Physical pin 9 */
	10, /* Physical pin 10 */
};

static const hrt_xfer_bus_widths_t io_modes[SUPPORTED_IO_MODES_COUNT] = {
	{1, 1, 1, 1}, /* MSPI_IO_MODE_SINGLE */
	{2, 2, 2, 2}, /* MSPI_IO_MODE_DUAL */
	{1, 1, 1, 2}, /* MSPI_IO_MODE_DUAL_1_1_2 */
	{1, 2, 2, 2}, /* MSPI_IO_MODE_DUAL_1_2_2 */
	{4, 4, 4, 4}, /* MSPI_IO_MODE_QUAD */
	{1, 1, 1, 4}, /* MSPI_IO_MODE_QUAD_1_1_4 */
	{1, 4, 4, 4}, /* MSPI_IO_MODE_QUAD_1_4_4 */
};

static volatile uint8_t ce_vios_count;
static volatile uint8_t ce_vios[DEVICES_MAX];
static volatile uint8_t data_vios_count;
static volatile uint8_t data_vios[DATA_PINS_MAX];
static volatile uint8_t clk_vio;
static volatile hpf_mspi_dev_config_t hpf_mspi_devices[DEVICES_MAX];
static volatile hpf_mspi_xfer_config_t hpf_mspi_xfer_config;
static volatile hpf_mspi_xfer_config_t *hpf_mspi_xfer_config_ptr = &hpf_mspi_xfer_config;

static volatile hrt_xfer_t xfer_params;

static volatile uint8_t response_buffer[CONFIG_HPF_MSPI_MAX_RESPONSE_SIZE];

static struct ipc_ept ep;
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);
#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
static NRF_TIMER_Type *fault_timer;
#endif
static volatile uint32_t *cpuflpr_error_ctx_ptr =
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(cpuflpr_error_code));

static void distribute_last_word_bits(void)
{
	uint32_t *rx_data = (uint32_t *)xfer_params.xfer_data[HRT_FE_DATA].data;
	uint32_t last_word = xfer_params.xfer_data[HRT_FE_DATA].last_word;
	uint32_t word_count = xfer_params.xfer_data[HRT_FE_DATA].word_count;
	uint32_t penultimate_word_bits =
		xfer_params.xfer_data[HRT_FE_DATA].penultimate_word_clocks *
		xfer_params.bus_widths.data;
	uint32_t last_word_bits =
		xfer_params.xfer_data[HRT_FE_DATA].last_word_clocks * xfer_params.bus_widths.data;
	uint32_t penultimate_word_shift = BITS_IN_WORD - penultimate_word_bits;
	/* In case when last word is too short, penultimate word has to give it 1 byte.
	 * this is here to pass this byte back to penultimate word to avoid holes.
	 */
	if ((penultimate_word_shift != 0) && (word_count > 1)) {
		rx_data[word_count - 2] = (rx_data[word_count - 2] >> penultimate_word_shift) |
					  (last_word << penultimate_word_bits);
		last_word = last_word >> penultimate_word_shift;
	}

	/* This is to avoid writing outside of data buffer in case when buffer_length%4 !=
	 * 0.
	 */
	for (uint8_t byte = 0;
	     byte < NRFX_CEIL_DIV(last_word_bits - penultimate_word_shift, BITS_IN_BYTE); byte++) {
		((uint8_t *)&(rx_data[word_count - 1]))[byte] = ((uint8_t *)&last_word)[byte];
	}
}

static void adjust_tail(volatile hrt_xfer_data_t *xfer_data, uint16_t frame_width,
			uint32_t data_length)
{
	if (data_length == 0) {
		return;
	}

	/* Due to hardware limitation, it is not possible to send only 1
	 * clock pulse.
	 */
	NRFX_ASSERT(data_length / frame_width >= 1);
	NRFX_ASSERT(data_vios_count >= frame_width);
	NRFX_ASSERT(data_length % frame_width == 0);

	uint8_t last_word_length = data_length % BITS_IN_WORD;
	uint8_t penultimate_word_length = BITS_IN_WORD;

	xfer_data->word_count = NRFX_CEIL_DIV(data_length, BITS_IN_WORD);

	/* Due to hardware limitations it is not possible to send only 1
	 * clock cycle. Therefore when data_length%32==FRAME_WIDTH  last
	 * word is sent shorter (24bits) and the remaining byte and
	 * FRAME_WIDTH number of bits are bit is sent together.
	 */
	if (last_word_length == 0) {

		last_word_length = BITS_IN_WORD;
		if (xfer_data->data != NULL) {
			xfer_data->last_word =
				((uint32_t *)xfer_data->data)[xfer_data->word_count - 1];
		}

	} else if ((last_word_length / frame_width == 1) && (xfer_data->word_count > 1)) {

		penultimate_word_length -= BITS_IN_BYTE;
		last_word_length += BITS_IN_BYTE;

		if (xfer_data->data != NULL) {
			xfer_data->last_word =
				((uint32_t *)xfer_data->data)[xfer_data->word_count - 2] >>
					(BITS_IN_WORD - BITS_IN_BYTE) |
				((uint32_t *)xfer_data->data)[xfer_data->word_count - 1]
					<< BITS_IN_BYTE;
		}
	} else if (xfer_data->data == NULL) {
		xfer_data->last_word = 0;
	} else {
		xfer_data->last_word = ((uint32_t *)xfer_data->data)[xfer_data->word_count - 1];
	}

	xfer_data->last_word_clocks = last_word_length / frame_width;
	xfer_data->penultimate_word_clocks = penultimate_word_length / frame_width;
}

static void configure_clock(enum mspi_cpp_mode cpp_mode)
{
	nrf_vpr_csr_vio_config_t vio_config = {
		.input_sel = false,
		.stop_cnt = true,
	};
	uint16_t out = nrf_vpr_csr_vio_out_get();

	switch (cpp_mode) {
	case MSPI_CPP_MODE_0: {
		vio_config.clk_polarity = 0;
		WRITE_BIT(out, clk_vio, VPRCSR_NORDIC_OUT_LOW);
		break;
	}
	case MSPI_CPP_MODE_1: {
		vio_config.clk_polarity = 1;
		WRITE_BIT(out, clk_vio, VPRCSR_NORDIC_OUT_LOW);
		break;
	}
	case MSPI_CPP_MODE_2: {
		vio_config.clk_polarity = 1;
		WRITE_BIT(out, clk_vio, VPRCSR_NORDIC_OUT_HIGH);
		break;
	}
	case MSPI_CPP_MODE_3: {
		vio_config.clk_polarity = 0;
		WRITE_BIT(out, clk_vio, VPRCSR_NORDIC_OUT_HIGH);
		break;
	}
	}

	nrf_vpr_csr_vio_out_set(out);
	nrf_vpr_csr_vio_config_set(&vio_config);
}

static void xfer_execute(hpf_mspi_xfer_packet_msg_t *xfer_packet, volatile uint8_t *rx_buffer)
{
	nrf_vpr_csr_vio_config_t config;
	volatile hpf_mspi_dev_config_t *device =
		&hpf_mspi_devices[hpf_mspi_xfer_config_ptr->device_index];
	uint16_t dummy_cycles = 0;

	if (xfer_packet->opcode == HPF_MSPI_TXRX) {
		NRFX_ASSERT(device->cnt0_value >= RX_CNT0_MIN_VALUE);

		nrf_vpr_csr_vio_config_get(&config);
		config.input_sel = true;
		nrf_vpr_csr_vio_config_set(&config);

		dummy_cycles = hpf_mspi_xfer_config_ptr->rx_dummy;
	} else {
		dummy_cycles = hpf_mspi_xfer_config_ptr->tx_dummy;
	}

	xfer_params.counter_value = device->cnt0_value;
	xfer_params.ce_vio = ce_vios[device->ce_index];
	xfer_params.ce_hold = hpf_mspi_xfer_config_ptr->hold_ce;
	xfer_params.cpp_mode = device->cpp;
	xfer_params.ce_polarity = device->ce_polarity;
	xfer_params.bus_widths = io_modes[device->io_mode];

	/* Fix position of command and address if command/address length is < BITS_IN_WORD,
	 * so that leading zeros would not be printed instead of data bits.
	 */
	xfer_packet->command =
		xfer_packet->command
		<< (BITS_IN_WORD - hpf_mspi_xfer_config_ptr->command_length * BITS_IN_BYTE);
	xfer_packet->address =
		xfer_packet->address
		<< (BITS_IN_WORD - hpf_mspi_xfer_config_ptr->address_length * BITS_IN_BYTE);

	/* Configure command phase. */
	xfer_params.xfer_data[HRT_FE_COMMAND].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_COMMAND].data = (uint8_t *)&xfer_packet->command;
	xfer_params.xfer_data[HRT_FE_COMMAND].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_COMMAND], xfer_params.bus_widths.command,
		    hpf_mspi_xfer_config_ptr->command_length * BITS_IN_BYTE);

	/* Configure address phase. */
	xfer_params.xfer_data[HRT_FE_ADDRESS].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_ADDRESS].data = (uint8_t *)&xfer_packet->address;
	xfer_params.xfer_data[HRT_FE_ADDRESS].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_ADDRESS], xfer_params.bus_widths.address,
		    hpf_mspi_xfer_config_ptr->address_length * BITS_IN_BYTE);

	/* Configure dummy_cycles phase. */
	xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES].data = NULL;
	xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES].word_count = 0;

	hrt_frame_element_t elem =
		hpf_mspi_xfer_config_ptr->address_length != 0 ? HRT_FE_ADDRESS : HRT_FE_COMMAND;

	/* Up to 63 clock pulses (including data from previous part) can be sent by simply
	 * increasing shift count of last word in the previous part.
	 * Beyond that, dummy cycles have to be treated af different transfer part.
	 */
	if (xfer_params.xfer_data[elem].last_word_clocks + dummy_cycles <= MAX_SHIFT_COUNT) {
		xfer_params.xfer_data[elem].last_word_clocks += dummy_cycles;
	} else {
		adjust_tail(&xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES],
			    xfer_params.bus_widths.dummy_cycles,
			    dummy_cycles * xfer_params.bus_widths.dummy_cycles);
	}

	/* Configure data phase. */
	xfer_params.xfer_data[HRT_FE_DATA].fun_out = HRT_FUN_OUT_BYTE;
	xfer_params.xfer_data[HRT_FE_DATA].word_count = 0;
	if (xfer_packet->opcode == HPF_MSPI_TXRX) {
		xfer_params.xfer_data[HRT_FE_DATA].data = NULL;

		adjust_tail(&xfer_params.xfer_data[HRT_FE_DATA], xfer_params.bus_widths.data,
			    xfer_packet->num_bytes * BITS_IN_BYTE);

		xfer_params.xfer_data[HRT_FE_DATA].data = rx_buffer;
	} else {
		xfer_params.xfer_data[HRT_FE_DATA].data = xfer_packet->data;
		adjust_tail(&xfer_params.xfer_data[HRT_FE_DATA], xfer_params.bus_widths.data,
			    xfer_packet->num_bytes * BITS_IN_BYTE);
	}

	/* Hardware issue: Additional clock edge when transmitting in modes other
	 *                 than MSPI_CPP_MODE_0.
	 * Here is first part workaround of that issue only for MSPI_CPP_MODE_2.
	 * Workaround: Add one pulse more to the last word in message,
	 *             and disable clock before the last pulse.
	 */
	if (device->cpp == MSPI_CPP_MODE_2) {
		/* Ommit data phase in rx mode */
		for (uint8_t phase = xfer_packet->opcode == HPF_MSPI_TXRX ? 1 : 0;
		     phase < HRT_FE_MAX; phase++) {
			if (xfer_params.xfer_data[HRT_FE_MAX - 1 - phase].word_count != 0) {
				xfer_params.xfer_data[HRT_FE_MAX - 1 - phase].last_word_clocks++;
				break;
			}
		}
	}

	if (device->cpp == MSPI_CPP_MODE_3) {
		NRFX_ASSERT(xfer_params.xfer_data[HRT_FE_DATA].last_word_clocks >=
			    MODE_3_RX_MIN_CLOCKS);
	}

	/* Read/write barrier to make sure that all configuration is done before jumping to HRT. */
	nrf_barrier_rw();

	if (xfer_packet->opcode == HPF_MSPI_TXRX) {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_READ));

		distribute_last_word_bits();

	} else {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE));
	}
}

static void config_pins(hpf_mspi_pinctrl_soc_pin_msg_t *pins_cfg)
{
	ce_vios_count = 0;
	data_vios_count = 0;
	xfer_params.tx_direction_mask = 0;
	xfer_params.rx_direction_mask = 0;

	for (uint8_t i = 0; i < DATA_PINS_MAX; i++) {
		data_vios[i] = DATA_PIN_UNUSED;
	}

	for (uint8_t i = 0; i < pins_cfg->pins_count; i++) {
		uint32_t psel = NRF_GET_PIN(pins_cfg->pin[i]);
		uint32_t fun = NRF_GET_FUN(pins_cfg->pin[i]);

		if ((psel == NRF_PIN_DISCONNECTED) || (pins_cfg->pin[i] == 0)) {
			continue;
		}

		uint8_t pin_number = NRF_PIN_NUMBER_TO_PIN(psel);

		NRFX_ASSERT(pin_number < HPF_MSPI_PINS_MAX);

		if ((fun >= NRF_FUN_HPF_MSPI_CS0) && (fun <= NRF_FUN_HPF_MSPI_CS4)) {

			ce_vios[ce_vios_count] = pin_to_vio_map[pin_number];
			WRITE_BIT(xfer_params.tx_direction_mask, ce_vios[ce_vios_count],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			WRITE_BIT(xfer_params.rx_direction_mask, ce_vios[ce_vios_count],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			ce_vios_count++;

		} else if ((fun >= NRF_FUN_HPF_MSPI_DQ0) && (fun <= NRF_FUN_HPF_MSPI_DQ7)) {

			NRFX_ASSERT(DATA_LINE_INDEX(fun) < DATA_PINS_MAX);
			NRFX_ASSERT(data_vios[DATA_LINE_INDEX(fun)] == DATA_PIN_UNUSED);

			data_vios[DATA_LINE_INDEX(fun)] = pin_to_vio_map[pin_number];
			WRITE_BIT(xfer_params.tx_direction_mask, data_vios[DATA_LINE_INDEX(fun)],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			WRITE_BIT(xfer_params.rx_direction_mask, data_vios[DATA_LINE_INDEX(fun)],
				  VPRCSR_NORDIC_DIR_INPUT);
			data_vios_count++;
		} else if (fun == NRF_FUN_HPF_MSPI_SCK) {
			clk_vio = pin_to_vio_map[pin_number];
			WRITE_BIT(xfer_params.tx_direction_mask, clk_vio, VPRCSR_NORDIC_DIR_OUTPUT);
			WRITE_BIT(xfer_params.rx_direction_mask, clk_vio, VPRCSR_NORDIC_DIR_OUTPUT);
		}
	}
	nrf_vpr_csr_vio_dir_set(xfer_params.tx_direction_mask);

	/* Set all devices as undefined. */
	for (uint8_t i = 0; i < DEVICES_MAX; i++) {
		hpf_mspi_devices[i].ce_index = CE_PIN_UNUSED;
	}
}

static void ep_bound(void *priv)
{
	atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
#ifdef CONFIG_HPF_MSPI_IPC_NO_COPY
	data = *(void **)data;
#endif

	(void)priv;
	(void)len;
	uint8_t opcode = *(uint8_t *)data;
	uint32_t num_bytes = 0;

#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
	if (fault_timer != NULL) {
		nrf_timer_task_trigger(fault_timer, NRF_TIMER_TASK_START);
	}
#endif

	switch (opcode) {
#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
	case HPF_MSPI_CONFIG_TIMER_PTR: {
		const hpf_mspi_flpr_timer_msg_t *timer_data =
			(const hpf_mspi_flpr_timer_msg_t *)data;

		fault_timer = timer_data->timer_ptr;
		break;
	}
#endif
	case HPF_MSPI_CONFIG_PINS: {
		hpf_mspi_pinctrl_soc_pin_msg_t *pins_cfg = (hpf_mspi_pinctrl_soc_pin_msg_t *)data;

		config_pins(pins_cfg);
		break;
	}
	case HPF_MSPI_CONFIG_DEV: {
		hpf_mspi_dev_config_msg_t *dev_config = (hpf_mspi_dev_config_msg_t *)data;

		NRFX_ASSERT(dev_config->device_index < DEVICES_MAX);
		NRFX_ASSERT(dev_config->dev_config.io_mode < SUPPORTED_IO_MODES_COUNT);
		NRFX_ASSERT((dev_config->dev_config.cpp == MSPI_CPP_MODE_0) ||
			    (dev_config->dev_config.cpp == MSPI_CPP_MODE_3));
		NRFX_ASSERT(dev_config->dev_config.ce_index < ce_vios_count);
		NRFX_ASSERT(dev_config->dev_config.ce_polarity <= MSPI_CE_ACTIVE_HIGH);

		hpf_mspi_devices[dev_config->device_index] = dev_config->dev_config;

		/* Configure CE pin. */
		if (hpf_mspi_devices[dev_config->device_index].ce_polarity == MSPI_CE_ACTIVE_LOW) {
			nrf_vpr_csr_vio_out_or_set(
				BIT(ce_vios[hpf_mspi_devices[dev_config->device_index].ce_index]));
		} else {
			nrf_vpr_csr_vio_out_clear_set(
				BIT(ce_vios[hpf_mspi_devices[dev_config->device_index].ce_index]));
		}

		if (dev_config->dev_config.io_mode == MSPI_IO_MODE_SINGLE) {
			if (data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ2)] != DATA_PIN_UNUSED &&
			    data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ3)] != DATA_PIN_UNUSED) {
				nrf_vpr_csr_vio_out_or_set(
					BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ2)]));
				nrf_vpr_csr_vio_out_or_set(
					BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ3)]));
			}
		} else {
			nrf_vpr_csr_vio_out_clear_set(
				BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ2)]));
			nrf_vpr_csr_vio_out_clear_set(
				BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ3)]));
		}

		break;
	}
	case HPF_MSPI_CONFIG_XFER: {
		hpf_mspi_xfer_config_msg_t *xfer_config = (hpf_mspi_xfer_config_msg_t *)data;

		NRFX_ASSERT(xfer_config->xfer_config.device_index < DEVICES_MAX);
		/* Check if device was configured. */
		NRFX_ASSERT(hpf_mspi_devices[xfer_config->xfer_config.device_index].ce_index <
			    ce_vios_count);
		NRFX_ASSERT(xfer_config->xfer_config.command_length <= sizeof(uint32_t));
		NRFX_ASSERT(xfer_config->xfer_config.address_length <= sizeof(uint32_t));
		NRFX_ASSERT(xfer_config->xfer_config.tx_dummy == 0 ||
			    xfer_config->xfer_config.command_length != 0 ||
			    xfer_config->xfer_config.address_length != 0);

#ifdef CONFIG_HPF_MSPI_IPC_NO_COPY
		hpf_mspi_xfer_config_ptr = &xfer_config->xfer_config;
#else
		hpf_mspi_xfer_config = xfer_config->xfer_config;
#endif
		configure_clock(hpf_mspi_devices[hpf_mspi_xfer_config_ptr->device_index].cpp);

		/* Tune up pad bias for frequencies above 32MHz */
		if (hpf_mspi_devices[hpf_mspi_xfer_config_ptr->device_index].cnt0_value <=
		    STD_PAD_BIAS_CNT0_THRESHOLD) {
			NRF_GPIOHSPADCTRL->BIAS = PAD_BIAS_VALUE;
		}

		break;
	}
	case HPF_MSPI_TX:
		hpf_mspi_xfer_packet_msg_t *packet = (hpf_mspi_xfer_packet_msg_t *)data;

		xfer_execute(packet, NULL);
		break;
	case HPF_MSPI_TXRX: {
		hpf_mspi_xfer_packet_msg_t *packet = (hpf_mspi_xfer_packet_msg_t *)data;
		if (packet->num_bytes > 0) {
#ifdef CONFIG_HPF_MSPI_IPC_NO_COPY
			xfer_execute(packet, packet->data);
#else
			NRFX_ASSERT(packet->num_bytes <=
				    CONFIG_HPF_MSPI_MAX_RESPONSE_SIZE - sizeof(hpf_mspi_opcode_t));
			num_bytes = packet->num_bytes;
			xfer_execute(packet, response_buffer + sizeof(hpf_mspi_opcode_t));
#endif
		}
		break;
	}
	default:
		opcode = HPF_MSPI_WRONG_OPCODE;
		break;
	}

	response_buffer[0] = opcode;
	ipc_service_send(&ep, (const void *)response_buffer,
			 sizeof(hpf_mspi_opcode_t) + num_bytes);

#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
	if (fault_timer != NULL) {
		nrf_timer_task_trigger(fault_timer, NRF_TIMER_TASK_CLEAR);
		nrf_timer_task_trigger(fault_timer, NRF_TIMER_TASK_STOP);
	}
#endif
}

static const struct ipc_ept_cfg ep_cfg = {
	.cb = {.bound = ep_bound, .received = ep_recv},
};

static int backend_init(void)
{
	int ret = 0;
	const struct device *ipc0_instance;
	volatile uint32_t delay = 0;

#if !defined(CONFIG_SYS_CLOCK_EXISTS)
	/* Wait a little bit for IPC service to be ready on APP side. */
	while (delay < 6000) {
		delay++;
	}
#endif

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Wait for endpoint to be bound. */
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED)) {
	}

	return 0;
}

__attribute__((interrupt)) void hrt_handler_read(void)
{
	hrt_read(&xfer_params);
}

__attribute__((interrupt)) void hrt_handler_write(void)
{
	hrt_write(&xfer_params);
}

/**
 * @brief Trap handler for HPF application.
 *
 * @details
 * This function is called on unhandled CPU exceptions. It's a good place to
 * handle critical errors and notify the core that the HPF application has
 * crashed.
 *
 * @param mcause  - cause of the exception (from mcause register)
 * @param mepc    - address of the instruction that caused the exception (from mepc register)
 * @param mtval   - additional value (e.g. bad address)
 * @param context - pointer to the saved context (only some registers are saved - ra, t0, t1, t2)
 */
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval, void *context)
{
	const uint8_t fault_opcode = HPF_MSPI_HPF_APP_HARD_FAULT;

	/* It can be distinguish whether the exception was caused by an interrupt or an error:
	 * On RV32, bit 31 of the mcause register indicates whether the event is an interrupt.
	 */
	if (mcause & 0x80000000) {
		/* Interrupt – can be handled or ignored */
	} else {
		/* Exception – critical error */
	}

	cpuflpr_error_ctx_ptr[0] = mcause;
	cpuflpr_error_ctx_ptr[1] = mepc;
	cpuflpr_error_ctx_ptr[2] = mtval;
	cpuflpr_error_ctx_ptr[3] = (uint32_t)context;

	ipc_service_send(&ep, &fault_opcode, sizeof(fault_opcode));

	while (1) {
		/* Loop forever */
	}
}

/* The trap_entry function is the entry point for exception handling.
 * The naked attribute prevents the compiler from generating an automatic prologue/epilogue.
 */
__attribute__((naked)) void trap_entry(void)
{
	__asm__ volatile(
		/* Reserve space on the stack:
		 * 16 bytes for 4 registers context (ra, t0, t1, t2).
		 */
		"addi sp, sp, -16\n"
		"sw   ra, 12(sp)\n"
		"sw   t0, 8(sp)\n"
		"sw   t1, 4(sp)\n"
		"sw   t2, 0(sp)\n"

		/* Read CSR: mcause, mepc, mtval */
		"csrr t0, mcause\n" /* t0 = mcause */
		"csrr t1, mepc\n"   /* t1 = mepc   */
		"csrr t2, mtval\n"  /* t2 = mtval  */

		/* Prepare arguments for trap_handler function:
		 * a0 = mcause (t0), a1 = mepc (t1), a2 = mtval (t2), a3 = sp (pointer on context).
		 */
		"mv   a0, t0\n"
		"mv   a1, t1\n"
		"mv   a2, t2\n"
		"mv   a3, sp\n"

		"call trap_handler\n"

		/* Restore registers values */
		"lw   ra, 12(sp)\n"
		"lw   t0, 8(sp)\n"
		"lw   t1, 4(sp)\n"
		"lw   t2, 0(sp)\n"
		"addi sp, sp, 16\n"

		"mret\n");
}

void init_trap_handler(void)
{
	/* Write the address of the trap_entry function into the mtvec CSR.
	 * Note: On RV32, the address must be aligned to 4 bytes.
	 */
	uintptr_t trap_entry_addr = (uintptr_t)&trap_entry;

	__asm__ volatile("csrw mtvec, %0\n"
			 : /* no outs */
			 : "r"(trap_entry_addr));
}

#if defined(CONFIG_ASSERT)
#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
	ARG_UNUSED(file);
	ARG_UNUSED(line);
#endif

	/* force send trap_handler signal */
	trap_entry();
}
#endif

int main(void)
{
	int ret = 0;

	init_trap_handler();

	ret = backend_init();
	if (ret < 0) {
		return 0;
	}

	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_READ, HRT_IRQ_PRIORITY, hrt_handler_read, 0);
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_READ), true);

	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_WRITE, HRT_IRQ_PRIORITY, hrt_handler_write, 0);
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE), true);

	nrf_vpr_csr_rtperiph_enable_set(true);

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
