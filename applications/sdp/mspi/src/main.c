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

#define SUPPORTED_IO_MODES_COUNT 7

#define DEVICES_MAX   5
#define DATA_PINS_MAX 8
#define VIO_COUNT     11

#define MAX_FREQUENCY 64000000

#define MAX_SHIFT_COUNT 63

#define CE_PIN_UNUSED UINT8_MAX

#define HRT_IRQ_PRIORITY    2
#define HRT_VEVIF_IDX_READ  17
#define HRT_VEVIF_IDX_WRITE 18

#define VEVIF_IRQN(vevif)   VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

BUILD_ASSERT(CONFIG_SDP_MSPI_MAX_RESPONSE_SIZE > 0, "Response max size should be greater that 0");

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
static volatile nrfe_mspi_dev_config_t nrfe_mspi_devices[DEVICES_MAX];
static volatile nrfe_mspi_xfer_config_t nrfe_mspi_xfer_config;
static volatile nrfe_mspi_xfer_config_t *nrfe_mspi_xfer_config_ptr = &nrfe_mspi_xfer_config;

static volatile hrt_xfer_t xfer_params;

static volatile uint8_t response_buffer[CONFIG_SDP_MSPI_MAX_RESPONSE_SIZE];

static struct ipc_ept ep;
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);

NRF_STATIC_INLINE void nrf_vpr_csr_vio_out_or_set(uint16_t value)
{
	nrf_csr_set_bits(VPRCSR_NORDIC_OUT, value);
}

NRF_STATIC_INLINE void nrf_vpr_csr_vio_out_clear_set(uint16_t value)
{
	nrf_csr_clear_bits(VPRCSR_NORDIC_OUT, value);
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
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_LOW);
		break;
	}
	case MSPI_CPP_MODE_1: {
		vio_config.clk_polarity = 1;
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_LOW);
		break;
	}
	case MSPI_CPP_MODE_2: {
		vio_config.clk_polarity = 1;
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_HIGH);
		break;
	}
	case MSPI_CPP_MODE_3: {
		vio_config.clk_polarity = 0;
		WRITE_BIT(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER], VPRCSR_NORDIC_OUT_HIGH);
		break;
	}
	}

	WRITE_BIT(out, 3, VPRCSR_NORDIC_OUT_HIGH);
	WRITE_BIT(out, 4, VPRCSR_NORDIC_OUT_HIGH);

	nrf_vpr_csr_vio_out_set(out);
	nrf_vpr_csr_vio_config_set(&vio_config);
}

static void xfer_execute(nrfe_mspi_xfer_packet_msg_t *xfer_packet)
{
	volatile nrfe_mspi_dev_config_t *device =
		&nrfe_mspi_devices[nrfe_mspi_xfer_config_ptr->device_index];

	xfer_params.counter_value = 4;
	xfer_params.ce_vio = ce_vios[device->ce_index];
	xfer_params.ce_hold = nrfe_mspi_xfer_config_ptr->hold_ce;
	xfer_params.cpp_mode = device->cpp;
	xfer_params.ce_polarity = device->ce_polarity;
	xfer_params.bus_widths = io_modes[device->io_mode];
	xfer_params.clk_vio = pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER];

	/* Fix position of command and address if command/address length is < BITS_IN_WORD,
	 * so that leading zeros would not be printed instead of data bits.
	 */
	xfer_packet->command =
		xfer_packet->command
		<< (BITS_IN_WORD - nrfe_mspi_xfer_config_ptr->command_length * BITS_IN_BYTE);
	xfer_packet->address =
		xfer_packet->address
		<< (BITS_IN_WORD - nrfe_mspi_xfer_config_ptr->address_length * BITS_IN_BYTE);

	xfer_params.xfer_data[HRT_FE_COMMAND].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_COMMAND].data = (uint8_t *)&xfer_packet->command;
	xfer_params.xfer_data[HRT_FE_COMMAND].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_COMMAND], xfer_params.bus_widths.command,
		    nrfe_mspi_xfer_config_ptr->command_length * BITS_IN_BYTE);

	xfer_params.xfer_data[HRT_FE_ADDRESS].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_ADDRESS].data = (uint8_t *)&xfer_packet->address;
	xfer_params.xfer_data[HRT_FE_ADDRESS].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_ADDRESS], xfer_params.bus_widths.address,
		    nrfe_mspi_xfer_config_ptr->address_length * BITS_IN_BYTE);

	xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES].data = NULL;
	xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES].word_count = 0;

	hrt_frame_element_t elem =
		nrfe_mspi_xfer_config_ptr->address_length != 0 ? HRT_FE_ADDRESS : HRT_FE_COMMAND;

	/* Up to 63 clock pulses (including data from previous part) can be sent by simply
	 * increasing shift count of last word in the previous part.
	 * Beyond that, dummy cycles have to be treated af different transfer part.
	 */
	if (xfer_params.xfer_data[elem].last_word_clocks + nrfe_mspi_xfer_config_ptr->tx_dummy <=
	    MAX_SHIFT_COUNT) {
		xfer_params.xfer_data[elem].last_word_clocks += nrfe_mspi_xfer_config_ptr->tx_dummy;
	} else {
		adjust_tail(&xfer_params.xfer_data[HRT_FE_DUMMY_CYCLES],
			    xfer_params.bus_widths.dummy_cycles,
			    nrfe_mspi_xfer_config_ptr->tx_dummy *
				    xfer_params.bus_widths.dummy_cycles);
	}

	xfer_params.xfer_data[HRT_FE_DATA].fun_out = HRT_FUN_OUT_BYTE;
	xfer_params.xfer_data[HRT_FE_DATA].data = xfer_packet->data;
	xfer_params.xfer_data[HRT_FE_DATA].word_count = 0;

	adjust_tail(&xfer_params.xfer_data[HRT_FE_DATA], xfer_params.bus_widths.data,
		    xfer_packet->num_bytes * BITS_IN_BYTE);

	/* Hardware issue: Additional clock edge when transmitting in modes other
	 *                 than MSPI_CPP_MODE_0.
	 * Here is first part workaround of that issue only for MSPI_CPP_MODE_2.
	 * Workaround: Add one pulse more to the last word in message,
	 *             and disable clock before the last pulse.
	 */
	if (device->cpp == MSPI_CPP_MODE_2) {
		for (uint8_t i = 0; i < HRT_FE_MAX; i++) {
			if (xfer_params.xfer_data[HRT_FE_MAX - 1 - i].word_count != 0) {
				xfer_params.xfer_data[HRT_FE_MAX - 1 - i].last_word_clocks++;
				break;
			}
		}
	}

	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE));
}

void prepare_and_read_data(nrfe_mspi_xfer_packet_msg_t *xfer_packet, volatile uint8_t *buffer)
{
	volatile nrfe_mspi_dev_config_t *device =
		&nrfe_mspi_devices[nrfe_mspi_xfer_config_ptr->device_index];
	nrf_vpr_csr_vio_config_t config;

	xfer_params.counter_value = 4;
	xfer_params.ce_vio = ce_vios[device->ce_index];
	xfer_params.ce_hold = nrfe_mspi_xfer_config_ptr->hold_ce;
	xfer_params.ce_polarity = device->ce_polarity;
	xfer_params.bus_widths = io_modes[device->io_mode];
	xfer_params.xfer_data[HRT_FE_DATA].data = buffer;

	nrf_vpr_csr_vio_config_get(&config);
	config.input_sel = true;
	nrf_vpr_csr_vio_config_set(&config);

	/*
	 * Fix position of command and address if command/address length is < BITS_IN_WORD,
	 * so that leading zeros would not be printed instead of data bits.
	 */
	xfer_packet->command =
		xfer_packet->command
		<< (BITS_IN_WORD - nrfe_mspi_xfer_config_ptr->command_length * BITS_IN_BYTE);
	xfer_packet->address =
		xfer_packet->address
		<< (BITS_IN_WORD - nrfe_mspi_xfer_config_ptr->address_length * BITS_IN_BYTE);

	/* Configure command phase. */
	xfer_params.xfer_data[HRT_FE_COMMAND].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_COMMAND].data = (uint8_t *)&xfer_packet->command;
	xfer_params.xfer_data[HRT_FE_COMMAND].word_count =
		nrfe_mspi_xfer_config_ptr->command_length;

	/* Configure address phase. */
	xfer_params.xfer_data[HRT_FE_ADDRESS].fun_out = HRT_FUN_OUT_WORD;
	xfer_params.xfer_data[HRT_FE_ADDRESS].data = (uint8_t *)&xfer_packet->address;
	xfer_params.xfer_data[HRT_FE_ADDRESS].word_count =
		nrfe_mspi_xfer_config_ptr->address_length;

	/* Configure data phase. */
	xfer_params.xfer_data[HRT_FE_DATA].word_count = xfer_packet->num_bytes;

	/* Read/write barrier to make sure that all configuration is done before jumping to HRT. */
	nrf_barrier_rw();

	/* Read data */
	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_READ));
}

static void config_pins(nrfe_mspi_pinctrl_soc_pin_msg_t *pins_cfg)
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

	/* Set all devices as undefined. */
	for (uint8_t i = 0; i < DEVICES_MAX; i++) {
		nrfe_mspi_devices[i].ce_index = CE_PIN_UNUSED;
	}
}

static void ep_bound(void *priv)
{
	atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
#ifdef CONFIG_SDP_MSPI_IPC_NO_COPY
	data = *(void **)data;
#endif

	(void)priv;
	(void)len;
	nrfe_mspi_flpr_response_msg_t response;
	uint8_t opcode = *(uint8_t *)data;
	uint32_t num_bytes = 0;

	response.opcode = opcode;

	switch (opcode) {
	case NRFE_MSPI_CONFIG_PINS: {
		nrfe_mspi_pinctrl_soc_pin_msg_t *pins_cfg = (nrfe_mspi_pinctrl_soc_pin_msg_t *)data;

		config_pins(pins_cfg);
		break;
	}
	case NRFE_MSPI_CONFIG_DEV: {
		nrfe_mspi_dev_config_msg_t *dev_config = (nrfe_mspi_dev_config_msg_t *)data;

		NRFX_ASSERT(dev_config->device_index < DEVICES_MAX);
		NRFX_ASSERT(dev_config->dev_config.io_mode < SUPPORTED_IO_MODES_COUNT);
		NRFX_ASSERT(dev_config->dev_config.cpp <= MSPI_CPP_MODE_3);
		NRFX_ASSERT(dev_config->dev_config.ce_index < ce_vios_count);
		NRFX_ASSERT(dev_config->dev_config.ce_polarity <= MSPI_CE_ACTIVE_HIGH);
		NRFX_ASSERT(dev_config->dev_config.freq <= MAX_FREQUENCY);

		nrfe_mspi_devices[dev_config->device_index] = dev_config->dev_config;

		/* Configure CE pin. */
		if (nrfe_mspi_devices[dev_config->device_index].ce_polarity == MSPI_CE_ACTIVE_LOW) {
			nrf_vpr_csr_vio_out_or_set(
				BIT(ce_vios[nrfe_mspi_devices[dev_config->device_index].ce_index]));
		} else {
			nrf_vpr_csr_vio_out_clear_set(
				BIT(ce_vios[nrfe_mspi_devices[dev_config->device_index].ce_index]));
		}

		break;
	}
	case NRFE_MSPI_CONFIG_XFER: {
		nrfe_mspi_xfer_config_msg_t *xfer_config = (nrfe_mspi_xfer_config_msg_t *)data;

		NRFX_ASSERT(xfer_config->xfer_config.device_index < DEVICES_MAX);
		/* Check if device was configured. */
		NRFX_ASSERT(nrfe_mspi_devices[xfer_config->xfer_config.device_index].ce_index <
			    ce_vios_count);
		NRFX_ASSERT(xfer_config->xfer_config.command_length <= sizeof(uint32_t));
		NRFX_ASSERT(xfer_config->xfer_config.address_length <= sizeof(uint32_t));
		NRFX_ASSERT(xfer_config->xfer_config.tx_dummy == 0 ||
			    xfer_config->xfer_config.command_length != 0 ||
			    xfer_config->xfer_config.address_length != 0);

#ifdef CONFIG_SDP_MSPI_IPC_NO_COPY
		nrfe_mspi_xfer_config_ptr = &xfer_config->xfer_config;
#else
		nrfe_mspi_xfer_config = xfer_config->xfer_config;
#endif
		configure_clock(nrfe_mspi_devices[nrfe_mspi_xfer_config_ptr->device_index].cpp);
		break;
	}
	case NRFE_MSPI_TX:
		nrfe_mspi_xfer_packet_msg_t *packet = (nrfe_mspi_xfer_packet_msg_t *)data;

		xfer_execute(packet);
		break;
	case NRFE_MSPI_TXRX: {
		nrfe_mspi_xfer_packet_msg_t *packet = (nrfe_mspi_xfer_packet_msg_t *)data;
		num_bytes = packet->num_bytes;

		if (num_bytes > 0) {
			prepare_and_read_data(packet, response_buffer + 1);
		}
		break;
	}
	default:
		opcode = NRFE_MSPI_WRONG_OPCODE;
		break;
	}

	response_buffer[0] = opcode;
	ipc_service_send(&ep, (const void *)response_buffer, sizeof(opcode) + num_bytes);
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

	/* Wait for endpoint to be bound. */
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED)) {
	}

	return 0;
}

__attribute__((interrupt)) void hrt_handler_read(void)
{
	hrt_read(&xfer_params);
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
