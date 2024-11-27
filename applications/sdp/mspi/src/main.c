/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hrt/hrt.h"

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/drivers/mspi.h>

#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <haly/nrfy_gpio.h>

#include <zephyr/drivers/mspi.h>

#define MAX_DATA_LEN 256

#define XFER_COMMAND_IDX (0)
#define XFER_ADDRESS_IDX (1)
#define XFER_DATA_IDX	 (2)

#define HRT_IRQ_PRIORITY	   2
#define HRT_VEVIF_IDX_WRITE_SINGLE 17
#define HRT_VEVIF_IDX_WRITE_QUAD   18

/* How many words are needed for given amount of bytes.*/
#define WORDS_FOR_BYTES(bytes) ((bytes - 1) / 4 + 1)

#define VEVIF_IRQN(vevif)   VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

typedef struct __packed {
	uint8_t opcode;
	struct mspi_cfg cfg;
} nrfe_mspi_cfg_t;

typedef struct __packed {
	uint8_t opcode;
	struct mspi_dev_cfg cfg;
} nrfe_mspi_dev_cfg_t;

typedef struct __packed {
	uint8_t opcode;
	struct mspi_xfer xfer;
} nrfe_mspi_xfer_t;

typedef struct __packed {
	uint8_t opcode;
	struct mspi_xfer_packet packet;
} nrfe_mspi_xfer_packet_t;

struct mspi_config {
	uint8_t *data;
	uint8_t data_len;
	uint8_t word_size;
};

struct mspi_dev_config {
	enum mspi_io_mode io_mode;
	enum mspi_ce_polarity ce_polarity;
	uint32_t read_cmd;
	uint32_t write_cmd;
	uint8_t cmd_length;  /* Command length in bits. */
	uint8_t addr_length; /* Address length in bits. */
};

static struct mspi_dev_config mspi_dev_configs;

uint32_t data_buffer[MAX_DATA_LEN + 2];

volatile struct hrt_ll_xfer xfer_ll_params = {
	.counter_top = 4,
	.word_size = 4,
	.data_to_send = NULL,
	.data_len = 0,
	.ce_hold = false,
	.ce_enable_state = false,
};

static struct ipc_ept ep;
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);

static void process_packet(const void *data, size_t len, void *priv);

static void ep_bound(void *priv)
{
	(void)priv;

	atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED);
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.received = process_packet,
	},
};

static void process_packet(const void *data, size_t len, void *priv)
{
	(void)priv;
	(void)len;
	nrfe_mspi_flpr_response_t response;
	uint8_t opcode = *(uint8_t *)data;

	response.opcode = opcode;

	switch (opcode) {
	case NRFE_MSPI_CONFIG_PINS: {
		/* TODO: Process pinctrl config data
		 * nrfe_mspi_pinctrl_soc_pin_t *pins_cfg = (nrfe_mspi_pinctrl_soc_pin_t *)data;
		 * response.opcode = pins_cfg->opcode;
		 *
		 * for (uint8_t i = 0; i < NRFE_MSPI_PINS_MAX; i++) {
		 *         uint32_t psel = NRF_GET_PIN(pins_cfg->pin[i]);
		 *         uint32_t fun = NRF_GET_FUN(pins_cfg->pin[i]);
		 *         NRF_GPIO_Type *reg = nrf_gpio_pin_port_decode(&psel);
		 * }
		 */
		break;
	}
	case NRFE_MSPI_CONFIG_CTRL: {
		/* TODO: Process controller config data
		 * nrfe_mspi_cfg_t *cfg = (nrfe_mspi_cfg_t *)data;
		 * response.opcode = cfg->opcode;
		 */
		break;
	}
	case NRFE_MSPI_CONFIG_DEV: {
		/* TODO: Process device config data
		 * nrfe_mspi_dev_cfg_t *cfg = (nrfe_mspi_dev_cfg_t *)data;
		 * response.opcode = cfg->opcode;
		 */
		break;
	}
	case NRFE_MSPI_CONFIG_XFER: {
		/* TODO: Process xfer config data
		 * nrfe_mspi_xfer_t *xfer = (nrfe_mspi_xfer_t *)data;
		 * response.opcode = xfer->opcode;
		 */
		break;
	}
	case NRFE_MSPI_TX:
	case NRFE_MSPI_TXRX: {
		nrfe_mspi_xfer_packet_t *packet = (nrfe_mspi_xfer_packet_t *)data;

		response.opcode = packet->opcode;

		if (packet->packet.dir == MSPI_RX) {
			/* TODO: Process received data */
		} else if (packet->packet.dir == MSPI_TX) {
			/* TODO: Send data */
		}
		break;
	}
	default:
		response.opcode = NRFE_MSPI_WRONG_OPCODE;
		break;
	}

	ipc_service_send(&ep, (const void *)&response.opcode, sizeof(response));
}

void configure_clock(enum mspi_cpp_mode cpp_mode)
{
	nrf_vpr_csr_vio_config_t vio_config = {
		.input_sel = 0,
		.stop_cnt = 0,
	};

	nrf_vpr_csr_vio_dir_set(PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));

	switch (cpp_mode) {
	case MSPI_CPP_MODE_0: {
		vio_config.clk_polarity = 0;
		nrf_vpr_csr_vio_out_set(PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));
		break;
	}
	case MSPI_CPP_MODE_1: {
		vio_config.clk_polarity = 1;
		nrf_vpr_csr_vio_out_set(PIN_OUT_LOW_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));
		break;
	}
	case MSPI_CPP_MODE_2: {
		vio_config.clk_polarity = 1;
		nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));
		break;
	}
	case MSPI_CPP_MODE_3: {
		vio_config.clk_polarity = 0;
		nrf_vpr_csr_vio_out_set(PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_SCK_PIN_NUMBER)));
		break;
	}
	}

	nrf_vpr_csr_vio_config_set(&vio_config);
}

void prepare_and_send_data(uint8_t *data, uint8_t data_length)
{
	memcpy(&(data_buffer[2]), data, data_length);

	/* Send command */
	xfer_ll_params.ce_hold = true;
	xfer_ll_params.word_size = mspi_dev_configs.cmd_length;
	xfer_ll_params.data_len = 1;
	xfer_ll_params.data_to_send = data_buffer;

	if (mspi_dev_configs.io_mode == MSPI_IO_MODE_QUAD) {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
	} else {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
	}

	/* Send address */
	xfer_ll_params.word_size = mspi_dev_configs.addr_length;
	xfer_ll_params.data_to_send = data_buffer + XFER_ADDRESS_IDX;

	if (mspi_dev_configs.io_mode == MSPI_IO_MODE_SINGLE ||
	    mspi_dev_configs.io_mode == MSPI_IO_MODE_QUAD_1_1_4) {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
	} else {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
	}

	/* Send data */
	xfer_ll_params.ce_hold = false;
	xfer_ll_params.word_size = 32;
	/* TODO: this system needs to be fixed as for now,
	 * there are problems when (data_length%4) != 0
	 */
	xfer_ll_params.data_len = WORDS_FOR_BYTES(data_length);
	xfer_ll_params.data_to_send = data_buffer + XFER_DATA_IDX;

	if (mspi_dev_configs.io_mode == MSPI_IO_MODE_SINGLE) {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
	} else {
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
	}
}

__attribute__((interrupt)) void hrt_handler_write_single(void)
{
	xfer_ll_params.ce_enable_state = (mspi_dev_configs.ce_polarity == MSPI_CE_ACTIVE_HIGH);

	write_single_by_word(xfer_ll_params);
}

__attribute__((interrupt)) void hrt_handler_write_quad(void)
{
	xfer_ll_params.ce_enable_state = (mspi_dev_configs.ce_polarity == MSPI_CE_ACTIVE_HIGH);

	write_quad_by_word(xfer_ll_params);
}

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

int main(void)
{
	int ret = 0;
	uint16_t direction;
	uint16_t output;

	ret = backend_init();
	if (ret < 0) {
		return 0;
	}

	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_WRITE_SINGLE, HRT_IRQ_PRIORITY, hrt_handler_write_single,
			   0);
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE), true);

	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_WRITE_QUAD, HRT_IRQ_PRIORITY, hrt_handler_write_quad, 0);
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD), true);

	nrf_vpr_csr_rtperiph_enable_set(true);

	configure_clock(MSPI_CPP_MODE_0);

	direction = nrf_vpr_csr_vio_dir_get();
	nrf_vpr_csr_vio_dir_set(direction | PIN_DIR_OUT_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER)));

	output = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(output | PIN_OUT_HIGH_MASK(VIO(NRFE_MSPI_CS0_PIN_NUMBER)));

	/* This initialization is temporary until code is merged with APP core part */
	mspi_dev_configs.ce_polarity = MSPI_CE_ACTIVE_LOW;
	mspi_dev_configs.io_mode = MSPI_IO_MODE_SINGLE;
	mspi_dev_configs.cmd_length = 32;
	mspi_dev_configs.addr_length = 32;

	data_buffer[XFER_COMMAND_IDX] = 0xe5b326c1;
	data_buffer[XFER_ADDRESS_IDX] = 0xaabbccdd;

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
