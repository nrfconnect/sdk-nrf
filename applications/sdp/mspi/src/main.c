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
#include <haly/nrfy_gpio.h>

#include <drivers/mspi/nrfe_mspi.h>

#define CE_PINS_MAX   9
#define DATA_PINS_MAX 8
#define VIO_COUNT     11

#define SUPPORTED_IO_MODES_COUNT 7

#define HRT_IRQ_PRIORITY    2
#define HRT_VEVIF_IDX_WRITE 18

#define VEVIF_IRQN(vevif)   VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

/* In OCTAL mode 4 bytes for address + 32 bytes for up to 32 dummy cycles*/
#define ADDR_AND_CYCLES_MAX_SIZE 36

static const uint8_t pin_to_vio_map[VIO_COUNT] = {
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
	{1, 1, 1}, /* MSPI_IO_MODE_SINGLE */
	{2, 2, 2}, /* MSPI_IO_MODE_DUAL */
	{1, 1, 2}, /* MSPI_IO_MODE_DUAL_1_1_2 */
	{1, 2, 2}, /* MSPI_IO_MODE_DUAL_1_2_2 */
	{4, 4, 4}, /* MSPI_IO_MODE_QUAD */
	{1, 1, 4}, /* MSPI_IO_MODE_QUAD_1_1_4 */
	{1, 4, 4}, /* MSPI_IO_MODE_QUAD_1_4_4 */
};

static volatile uint8_t ce_vios_count;
static volatile uint8_t ce_vios[CE_PINS_MAX];
static volatile uint8_t data_vios_count;
static volatile uint8_t data_vios[DATA_PINS_MAX];
static volatile struct mspi_cfg nrfe_mspi_cfg;
static volatile struct mspi_dev_cfg nrfe_mspi_dev_cfg;
static volatile struct mspi_xfer nrfe_mspi_xfer;
static volatile hrt_xfer_t xfer_params;
static volatile uint8_t address_and_dummy_cycles[ADDR_AND_CYCLES_MAX_SIZE];

static struct ipc_ept ep;
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);

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
	xfer_data->last_word = ((uint32_t *)xfer_data->data)[xfer_data->word_count - 1];

	/* Due to hardware limitations it is not possible to send only 1
	 * clock cycle. Therefore when data_length%32==FRAME_WIDTH  last
	 * word is sent shorter (24bits) and the remaining byte and
	 * FRAME_WIDTH number of bits are bit is sent together.
	 */
	if (last_word_length == 0) {

		last_word_length = BITS_IN_WORD;
		xfer_data->last_word = ((uint32_t *)xfer_data->data)[xfer_data->word_count - 1];

	} else if ((last_word_length / frame_width == 1) && (xfer_data->word_count > 1)) {

		penultimate_word_length -= BITS_IN_BYTE;
		last_word_length += BITS_IN_BYTE;

		xfer_data->last_word = ((uint32_t *)xfer_data->data)[xfer_data->word_count - 2] >>
					       (BITS_IN_WORD - BITS_IN_BYTE) |
				       ((uint32_t *)xfer_data->data)[xfer_data->word_count - 1]
					       << BITS_IN_BYTE;
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
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_LOW);
		xfer_params.eliminate_last_pulse = false;
		break;
	}
	case MSPI_CPP_MODE_1: {
		vio_config.clk_polarity = 1;
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_LOW);
		xfer_params.eliminate_last_pulse = true;
		break;
	}
	case MSPI_CPP_MODE_2: {
		vio_config.clk_polarity = 1;
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_HIGH);
		xfer_params.eliminate_last_pulse = false;
		break;
	}
	case MSPI_CPP_MODE_3: {
		vio_config.clk_polarity = 0;
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_HIGH);
		xfer_params.eliminate_last_pulse = true;
		break;
	}
	}
	nrf_vpr_csr_vio_out_set(out);
	nrf_vpr_csr_vio_config_set(&vio_config);
}

static void xfer_execute(struct mspi_xfer_packet xfer_packet)
{
	NRFX_ASSERT(nrfe_mspi_dev_cfg.ce_num < ce_vios_count);
	NRFX_ASSERT(nrfe_mspi_dev_cfg.io_mode < SUPPORTED_IO_MODES_COUNT);

	xfer_params.counter_value = 4;
	xfer_params.ce_vio = ce_vios[nrfe_mspi_dev_cfg.ce_num];
	xfer_params.ce_hold = nrfe_mspi_xfer.hold_ce;
	xfer_params.ce_polarity = nrfe_mspi_dev_cfg.ce_polarity;
	xfer_params.bus_widths = io_modes[nrfe_mspi_dev_cfg.io_mode];

	/* Fix position of command if command length is < BITS_IN_WORD,
	 * so that leading zeros would not be printed instead of data bits.
	 */
	xfer_packet.cmd = xfer_packet.cmd
			  << (BITS_IN_WORD - nrfe_mspi_xfer.cmd_length * BITS_IN_BYTE);

	xfer_params.xfer_data[HRT_FE_COMMAND].vio_out_set =
		&nrf_vpr_csr_vio_out_buffered_reversed_word_set;
	xfer_params.xfer_data[HRT_FE_COMMAND].data = (uint8_t *)&xfer_packet.cmd;
	xfer_params.xfer_data[HRT_FE_COMMAND].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_COMMAND], xfer_params.bus_widths.command,
		    nrfe_mspi_xfer.cmd_length * BITS_IN_BYTE);

	/* Reverse address byte order so that address values are sent instead of zeros */
	for (uint8_t i = 0; i < nrfe_mspi_xfer.addr_length; i++) {
		address_and_dummy_cycles[i] =
			*(((uint8_t *)&xfer_packet.address) + nrfe_mspi_xfer.addr_length - i - 1);
	}

	for (uint8_t i = nrfe_mspi_xfer.addr_length; i < ADDR_AND_CYCLES_MAX_SIZE; i++) {
		address_and_dummy_cycles[i] = 0;
	}

	xfer_params.xfer_data[HRT_FE_ADDRESS].vio_out_set =
		&nrf_vpr_csr_vio_out_buffered_reversed_byte_set;
	xfer_params.xfer_data[HRT_FE_ADDRESS].data = address_and_dummy_cycles;
	xfer_params.xfer_data[HRT_FE_ADDRESS].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_ADDRESS], xfer_params.bus_widths.address,
		    nrfe_mspi_xfer.addr_length * BITS_IN_BYTE +
			    nrfe_mspi_xfer.tx_dummy * xfer_params.bus_widths.address);

	xfer_params.xfer_data[HRT_FE_DATA].vio_out_set =
		&nrf_vpr_csr_vio_out_buffered_reversed_byte_set;
	xfer_params.xfer_data[HRT_FE_DATA].data = xfer_packet.data_buf;
	xfer_params.xfer_data[HRT_FE_DATA].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_DATA], xfer_params.bus_widths.data,
		    xfer_packet.num_bytes * BITS_IN_BYTE);

	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE));
}

static void config_pins(nrfe_mspi_pinctrl_soc_pin_t *pins_cfg)
{
	ce_vios_count = 0;
	data_vios_count = 0;
	xfer_params.tx_direction_mask = 0;
	xfer_params.rx_direction_mask = 0;

	for (uint8_t i = 0; i < NRFE_MSPI_PINS_MAX; i++) {
		uint32_t psel = NRF_GET_PIN(pins_cfg->pin[i]);
		uint32_t fun = NRF_GET_FUN(pins_cfg->pin[i]);

		uint8_t pin_number = NRF_PIN_NUMBER_TO_PIN(psel);

		NRFX_ASSERT(pin_number < VIO_COUNT)

		if ((fun >= NRF_FUN_SDP_MSPI_CS0) && (fun <= NRF_FUN_SDP_MSPI_CS4)) {

			ce_vios[ce_vios_count] = pin_to_vio_map[pin_number];
			WRITE_BIT(xfer_params.tx_direction_mask, ce_vios[ce_vios_count],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			WRITE_BIT(xfer_params.rx_direction_mask, ce_vios[ce_vios_count],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			ce_vios_count++;

		} else if ((fun >= NRF_FUN_SDP_MSPI_DQ0) && (fun <= NRF_FUN_SDP_MSPI_DQ7)) {

			data_vios[data_vios_count] = pin_to_vio_map[pin_number];
			WRITE_BIT(xfer_params.tx_direction_mask, data_vios[data_vios_count],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			WRITE_BIT(xfer_params.rx_direction_mask, data_vios[data_vios_count],
				  VPRCSR_NORDIC_DIR_INPUT);
			data_vios_count++;
		} else if (fun == NRF_FUN_SDP_MSPI_SCK) {
			WRITE_BIT(xfer_params.tx_direction_mask,
				  pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER],
				  VPRCSR_NORDIC_DIR_OUTPUT);
			WRITE_BIT(xfer_params.rx_direction_mask,
				  pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER],
				  VPRCSR_NORDIC_DIR_OUTPUT);
		}
	}
	nrf_vpr_csr_vio_dir_set(xfer_params.tx_direction_mask);
	nrf_vpr_csr_vio_out_set(VPRCSR_NORDIC_OUT_HIGH << pin_to_vio_map[NRFE_MSPI_CS0_PIN_NUMBER]);
}

static void ep_bound(void *priv)
{
	atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	(void)priv;
	(void)len;
	nrfe_mspi_flpr_response_t response;
	uint8_t opcode = *(uint8_t *)data;

	response.opcode = opcode;

	switch (opcode) {
	case NRFE_MSPI_CONFIG_PINS: {
		nrfe_mspi_pinctrl_soc_pin_t *pins_cfg = (nrfe_mspi_pinctrl_soc_pin_t *)data;

		config_pins(pins_cfg);
		break;
	}
	case NRFE_MSPI_CONFIG_CTRL: {
		nrfe_mspi_cfg_t *cfg = (nrfe_mspi_cfg_t *)data;

		nrfe_mspi_cfg = cfg->cfg;
		break;
	}
	case NRFE_MSPI_CONFIG_DEV: {
		nrfe_mspi_dev_cfg_t *cfg = (nrfe_mspi_dev_cfg_t *)data;

		nrfe_mspi_dev_cfg = cfg->cfg;
		configure_clock(nrfe_mspi_dev_cfg.cpp);
		break;
	}
	case NRFE_MSPI_CONFIG_XFER: {
		nrfe_mspi_xfer_t *xfer = (nrfe_mspi_xfer_t *)data;

		nrfe_mspi_xfer = xfer->xfer;
		break;
	}
	case NRFE_MSPI_TX:
	case NRFE_MSPI_TXRX: {
		nrfe_mspi_xfer_packet_t *packet = (nrfe_mspi_xfer_packet_t *)data;

		if (packet->packet.dir == MSPI_RX) {
			/* TODO: Process received data */
		} else if (packet->packet.dir == MSPI_TX) {
			xfer_execute(packet->packet);
		}
		break;
	}
	default:
		response.opcode = NRFE_MSPI_WRONG_OPCODE;
		break;
	}

	ipc_service_send(&ep, (const void *)&response.opcode, sizeof(response));
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
	/* Wait a little bit for IPC service to be ready on APP side */
	while (delay < 1000) {
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

	/* Wait for endpoint to be bound */
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED)) {
	}

	return 0;
}

__attribute__((interrupt)) void hrt_handler_write(void)
{
	hrt_write((hrt_xfer_t *)&xfer_params);
}

int main(void)
{
	int ret = backend_init();

	if (ret < 0) {
		return 0;
	}

	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_WRITE, HRT_IRQ_PRIORITY, hrt_handler_write, 0);
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE), true);

	nrf_vpr_csr_rtperiph_enable_set(true);

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
