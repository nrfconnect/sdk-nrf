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
#include <hal/nrf_vpr_csr_vtim.h>
#include <haly/nrfy_gpio.h>

#include <drivers/mspi/nrfe_mspi.h>

#include <stdint.h>

/*************************************************
 * 		   DEFINES
 *************************************************/
#define CE_PINS_MAX   9
#define DATA_PINS_MAX 8

#define HRT_IRQ_PRIORITY    2
#define HRT_VEVIF_IDX_WRITE 18

#define PINCTL_PIN_NUM_MASK 0x0fU

#define VEVIF_IRQN(vevif)   VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

/*************************************************
 * 	      TYPEDEFS, ENUMS, STRUCTS
 *************************************************/

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

enum base_io_mode {
	BASE_IO_MODE_SINGLE,
	BASE_IO_MODE_DUAL,
	BASE_IO_MODE_QUAD,
	BASE_IO_MODE_OCTAL
};

struct xfer_io_mode_cfg {
	enum base_io_mode command;
	enum base_io_mode address;
	enum base_io_mode data;
};

/*************************************************
 * 	      LOCAL VARIABLES
 *************************************************/

static const uint8_t pin_to_vio_map[] = {
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

static volatile uint8_t ce_vios_count;
static volatile uint8_t ce_vios[CE_PINS_MAX];
static volatile uint8_t data_vios_count;
static volatile uint8_t data_vios[DATA_PINS_MAX];
static volatile struct mspi_cfg nrfe_mspi_cfg;
static volatile struct mspi_dev_cfg nrfe_mspi_dev_cfg;
static volatile struct mspi_xfer nrfe_mspi_xfer;
static volatile struct mspi_xfer_packet nrfe_mspi_xfer_packet;
static volatile struct hrt_ll_xfer xfer_ll_params;

static struct ipc_ept ep;
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);

/*************************************************
 * 	    GLOBAL FUNCTION PROTOTYPES
 *************************************************/

__attribute__((interrupt)) void hrt_handler_write(void);

int main(void);

/*************************************************
 * 	    LOCAL FUNCTION PROTOTYPES
 *************************************************/

static struct xfer_io_mode_cfg get_io_modes(enum mspi_io_mode mode);

static void ep_bound(void *priv);

static void ep_recv(const void *data, size_t len, void *priv);

static void ll_prepare_transfer(enum base_io_mode xfer_mode, uint32_t data_length);

static void configure_clock(enum mspi_cpp_mode cpp_mode);

static void prepare_and_send_data();

static void process_packet(const void *data, size_t len);

static int backend_init(void);

/*************************************************
 * 	    GLOBAL FUNCTION IMPLEMENTATIONS
 *************************************************/

__attribute__((interrupt)) void hrt_handler_write(void)
{
	hrt_write(xfer_ll_params);
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

/*************************************************
 * 	    LOCAL FUNCTION IMPLEMENTATIONS
 *************************************************/

static struct xfer_io_mode_cfg get_io_modes(enum mspi_io_mode mode)
{
	struct xfer_io_mode_cfg xfer_mode;

	switch (nrfe_mspi_dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_SINGLE;
		xfer_mode.data = BASE_IO_MODE_SINGLE;
		break;
	case MSPI_IO_MODE_DUAL:
		xfer_mode.command = BASE_IO_MODE_DUAL;
		xfer_mode.address = BASE_IO_MODE_DUAL;
		xfer_mode.data = BASE_IO_MODE_DUAL;
		break;
	case MSPI_IO_MODE_DUAL_1_1_2:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_SINGLE;
		xfer_mode.data = BASE_IO_MODE_DUAL;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_DUAL;
		xfer_mode.data = BASE_IO_MODE_DUAL;
		break;
	case MSPI_IO_MODE_QUAD:
		xfer_mode.command = BASE_IO_MODE_QUAD;
		xfer_mode.address = BASE_IO_MODE_QUAD;
		xfer_mode.data = BASE_IO_MODE_QUAD;
		break;
	case MSPI_IO_MODE_QUAD_1_1_4:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_SINGLE;
		xfer_mode.data = BASE_IO_MODE_QUAD;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_QUAD;
		xfer_mode.data = BASE_IO_MODE_QUAD;
		break;
	case MSPI_IO_MODE_OCTAL:
		xfer_mode.command = BASE_IO_MODE_OCTAL;
		xfer_mode.address = BASE_IO_MODE_OCTAL;
		xfer_mode.data = BASE_IO_MODE_OCTAL;
		break;
	case MSPI_IO_MODE_OCTAL_1_1_8:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_SINGLE;
		xfer_mode.data = BASE_IO_MODE_OCTAL;
		break;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		xfer_mode.command = BASE_IO_MODE_SINGLE;
		xfer_mode.address = BASE_IO_MODE_OCTAL;
		xfer_mode.data = BASE_IO_MODE_OCTAL;
		break;
	case MSPI_IO_MODE_HEX:
	case MSPI_IO_MODE_HEX_8_8_16:
	case MSPI_IO_MODE_HEX_8_16_16:
	case MSPI_IO_MODE_MAX:
	default:
		/* TODO: Jira ticket: NRFX-6875 error*/
		break;
	}

	return xfer_mode;
}

static void ep_bound(void *priv)
{
	atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	(void)priv;

	process_packet(data, len);
}

static void ll_prepare_transfer(enum base_io_mode xfer_mode, uint32_t data_length)
{
	uint16_t dir;
	uint16_t out;
	nrf_vpr_csr_vio_config_t config;
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
		.frame_width = 1,
	};

	dir = nrf_vpr_csr_vio_dir_get();
	out = nrf_vpr_csr_vio_out_get();

	switch (xfer_mode) {
	case BASE_IO_MODE_SINGLE:
		out_mode.frame_width = 1;
		break;
	case BASE_IO_MODE_DUAL:
		out_mode.frame_width = 2;
		break;
	case BASE_IO_MODE_QUAD:
		out_mode.frame_width = 4;
		break;
	case BASE_IO_MODE_OCTAL:
		out_mode.frame_width = 8;
		break;
	}

	NRFX_ASSERT(data_vios_count >= out_mode.frame_width);
	NRFX_ASSERT(data_length % out_mode.frame_width == 0)

	uint16_t direction = (nrfe_mspi_xfer_packet.dir == MSPI_TX) ? VPRCSR_NORDIC_DIR_OUTPUT
								    : VPRCSR_NORDIC_DIR_INPUT;

	for (uint8_t i = 0; i < out_mode.frame_width; i++) {
		dir = BIT_SET_VALUE(dir, data_vios[i], direction);
		out = BIT_SET_VALUE(out, data_vios[i], VPRCSR_NORDIC_OUT_LOW);
	}

	dir = BIT_SET_VALUE(dir, xfer_ll_params.ce_vio, VPRCSR_NORDIC_DIR_OUTPUT);

	if (nrfe_mspi_dev_cfg.ce_polarity == MSPI_CE_ACTIVE_LOW) {
		out = BIT_SET_VALUE(out, xfer_ll_params.ce_vio, VPRCSR_NORDIC_OUT_HIGH);
	} else {
		out = BIT_SET_VALUE(out, xfer_ll_params.ce_vio, VPRCSR_NORDIC_OUT_LOW);
	}

	nrf_vpr_csr_vio_dir_set(dir);
	nrf_vpr_csr_vio_out_set(out);

	nrf_vpr_csr_vio_mode_out_set(&out_mode);
	nrf_vpr_csr_vio_mode_in_buffered_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	nrf_vpr_csr_vio_config_get(&config);
	config.input_sel = false;
	nrf_vpr_csr_vio_config_set(&config);

	/* Counter settings */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	/* TODO: Jira ticket: NRFX-6703
	 *       Top value of VTIM. This will determine clock frequency
	 *                         (SPI_CLOCK ~= CPU_CLOCK / (2 * TOP)).
	 *       Calculate this value based on frequency
	 */
	nrf_vpr_csr_vtim_simple_counter_top_set(0, 4);

	/* Set number of shifts before OUTB needs to be updated.
	 * First shift needs to be increased by 1.
	 */
	nrf_vpr_csr_vio_shift_cnt_out_set(BITS_IN_WORD / out_mode.frame_width);
	nrf_vpr_csr_vio_shift_cnt_out_buffered_set(BITS_IN_WORD / out_mode.frame_width - 1);

	uint8_t last_word_length = data_length % BITS_IN_WORD;
	uint8_t penultimate_word_length = BITS_IN_WORD;

	xfer_ll_params.words = NRFX_CEIL_DIV(data_length, BITS_IN_WORD);
	xfer_ll_params.last_word = ((uint32_t *)xfer_ll_params.data)[xfer_ll_params.words - 1];

	/* Due to hardware limitations it is not possible to send only 1 clock cycle.
	 * Therefore when data_length%32==1  last word is sent shorter (24bits)
	 * and the remaining byte and 1 bit is sent together.
	 */
	if (last_word_length == 0) {

		last_word_length = BITS_IN_WORD;
		xfer_ll_params.last_word =
			((uint32_t *)xfer_ll_params.data)[xfer_ll_params.words - 1];

	} else if ((last_word_length / out_mode.frame_width == 1) && (xfer_ll_params.words > 1)) {

		penultimate_word_length -= BITS_IN_BYTE;
		last_word_length += BITS_IN_BYTE;
		xfer_ll_params.last_word =
			((uint32_t *)xfer_ll_params.data)[xfer_ll_params.words - 2] >>
				(BITS_IN_WORD - BITS_IN_BYTE) |
			((uint32_t *)xfer_ll_params.data)[xfer_ll_params.words - 1] << BITS_IN_BYTE;
	}

	xfer_ll_params.last_word_clocks = last_word_length / out_mode.frame_width;
	xfer_ll_params.penultimate_word_clocks = penultimate_word_length / out_mode.frame_width;

	if (xfer_ll_params.words == 1) {
		nrf_vpr_csr_vio_shift_cnt_out_set(xfer_ll_params.last_word_clocks);
	} else if (xfer_ll_params.words == 2) {
		nrf_vpr_csr_vio_shift_cnt_out_set(xfer_ll_params.penultimate_word_clocks);
	}
}

static void configure_clock(enum mspi_cpp_mode cpp_mode)
{
	nrf_vpr_csr_vio_config_t vio_config = {
		.input_sel = 0,
		.stop_cnt = 0,
	};
	uint16_t out = nrf_vpr_csr_vio_out_get();

	switch (cpp_mode) {
	case MSPI_CPP_MODE_0: {
		vio_config.clk_polarity = 0;
		out = BIT_SET_VALUE(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER],
				    VPRCSR_NORDIC_OUT_LOW);
		xfer_ll_params.eliminate_last_pulse = false;
		break;
	}
	case MSPI_CPP_MODE_1: {
		vio_config.clk_polarity = 1;
		out = BIT_SET_VALUE(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER],
				    VPRCSR_NORDIC_OUT_LOW);
		xfer_ll_params.eliminate_last_pulse = true;
		break;
	}
	case MSPI_CPP_MODE_2: {
		vio_config.clk_polarity = 1;
		out = BIT_SET_VALUE(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER],
				    VPRCSR_NORDIC_OUT_HIGH);
		xfer_ll_params.eliminate_last_pulse = false;
		break;
	}
	case MSPI_CPP_MODE_3: {
		vio_config.clk_polarity = 0;
		out = BIT_SET_VALUE(out, pin_to_vio_map[NRFE_MSPI_SCK_PIN_NUMBER],
				    VPRCSR_NORDIC_OUT_HIGH);
		xfer_ll_params.eliminate_last_pulse = true;
		break;
	}
	}
	nrf_vpr_csr_vio_out_set(out);
	nrf_vpr_csr_vio_config_set(&vio_config);
}

static void prepare_and_send_data()
{
	NRFX_ASSERT(nrfe_mspi_dev_cfg.ce_num < ce_vios_count);

	struct xfer_io_mode_cfg xfer_modes = get_io_modes(nrfe_mspi_dev_cfg.io_mode);
	uint32_t data;

	/* Send command */

	/* TODO: Jira ticket: NRFX-6703
	 * Device waits this time after setting CE and before sending
	 * first bit, make this value dependent on dummy cycles.
	 */
	xfer_ll_params.counter_initial_value = 120;
	data = nrfe_mspi_xfer_packet.cmd << (BITS_IN_WORD - nrfe_mspi_dev_cfg.cmd_length);
	xfer_ll_params.data = (uint8_t *)&data;
	xfer_ll_params.ce_vio = ce_vios[nrfe_mspi_dev_cfg.ce_num];
	xfer_ll_params.ce_hold = nrfe_mspi_xfer.hold_ce;
	xfer_ll_params.ce_polarity = nrfe_mspi_dev_cfg.ce_polarity;
	xfer_ll_params.bit_order = HRT_BO_REVERSED_WORD;

	ll_prepare_transfer(xfer_modes.command, nrfe_mspi_dev_cfg.cmd_length);

	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE));

	/* Send address */
	data = nrfe_mspi_xfer_packet.address << (BITS_IN_WORD - nrfe_mspi_dev_cfg.addr_length);
	xfer_ll_params.data = (uint8_t *)&data;

	ll_prepare_transfer(xfer_modes.address, nrfe_mspi_dev_cfg.addr_length);

	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE));

	/* Send data */
	xfer_ll_params.bit_order = HRT_BO_REVERSED_BYTE;
	/* TODO: Jira ticket: NRFX-6876 use read buffer that is appended to packet in NRFE_MSPI_TXRX,
	 * this is not possible now due to alignment problems
	 */
	xfer_ll_params.data = nrfe_mspi_xfer_packet.data_buf;

	ll_prepare_transfer(xfer_modes.data, nrfe_mspi_xfer_packet.num_bytes * BITS_IN_BYTE);

	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE));
}

static void process_packet(const void *data, size_t len)
{
	(void)len;
	uint16_t dir;
	uint16_t out;
	nrfe_mspi_flpr_response_t response;
	uint8_t opcode = *(uint8_t *)data;

	switch (opcode) {
	case NRFE_MSPI_CONFIG_PINS: {
		nrfe_mspi_pinctrl_soc_pin_t *pins_cfg = (nrfe_mspi_pinctrl_soc_pin_t *)data;

		response.opcode = pins_cfg->opcode;

		ce_vios_count = 0;
		data_vios_count = 0;
		dir = 0;
		out = 0;

		for (uint8_t i = 0; i < NRFE_MSPI_PINS_MAX; i++) {
			uint32_t psel = NRF_GET_PIN(pins_cfg->pin[i]);
			uint32_t fun = NRF_GET_FUN(pins_cfg->pin[i]);

			uint8_t pin_number = psel & PINCTL_PIN_NUM_MASK;

			if (pin_number >= sizeof(pin_to_vio_map)) {
				/* TODO: Jira ticket: NRFX-6875 error*/
				return;
			}

			dir = BIT_SET_VALUE(dir, pin_to_vio_map[pin_number],
					    VPRCSR_NORDIC_DIR_OUTPUT);

			if ((fun >= NRF_FUN_SDP_MSPI_CS0) && (fun <= NRF_FUN_SDP_MSPI_CS4)) {

				NRFX_ASSERT(pin_number < sizeof(pin_to_vio_map))
				ce_vios[ce_vios_count] = pin_to_vio_map[pin_number];
				ce_vios_count++;

				/* TODO: Jira ticket: NRFX-6876 Get CE disabled states and set them,
				 * they need to be passed from app
				 */
			} else if ((fun >= NRF_FUN_SDP_MSPI_DQ0) && (fun <= NRF_FUN_SDP_MSPI_DQ7)) {

				NRFX_ASSERT(pin_number < sizeof(pin_to_vio_map))
				data_vios[data_vios_count] = pin_to_vio_map[pin_number];
				data_vios_count++;
			}
		}
		nrf_vpr_csr_vio_dir_set(dir);
		nrf_vpr_csr_vio_out_set(0);
		break;
	}
	case NRFE_MSPI_CONFIG_CTRL: {
		nrfe_mspi_cfg_t *cfg = (nrfe_mspi_cfg_t *)data;

		response.opcode = cfg->opcode;
		nrfe_mspi_cfg = cfg->cfg;
		break;
	}
	case NRFE_MSPI_CONFIG_DEV: {
		nrfe_mspi_dev_cfg_t *cfg = (nrfe_mspi_dev_cfg_t *)data;
		response.opcode = cfg->opcode;
		nrfe_mspi_dev_cfg = cfg->cfg;

		configure_clock(nrfe_mspi_dev_cfg.cpp);
		break;
	}
	case NRFE_MSPI_CONFIG_XFER: {
		nrfe_mspi_xfer_t *xfer = (nrfe_mspi_xfer_t *)data;

		response.opcode = xfer->opcode;
		nrfe_mspi_xfer = xfer->xfer;
		break;
	}
	case NRFE_MSPI_TX:
	case NRFE_MSPI_TXRX: {
		nrfe_mspi_xfer_packet_t *packet = (nrfe_mspi_xfer_packet_t *)data;

		response.opcode = packet->opcode;
		nrfe_mspi_xfer_packet = packet->packet;

		if (packet->packet.dir == MSPI_RX) {
			/* TODO: Jira ticket: NRFX-6877 Process received data */
		} else if (packet->packet.dir == MSPI_TX) {
			prepare_and_send_data();
		}
		break;
	}
	default:
		response.opcode = NRFE_MSPI_WRONG_OPCODE;
		break;
	}

	ipc_service_send(&ep, (const void *)&response.opcode, sizeof(response));
}

static int backend_init(void)
{
	int ret = 0;
	const struct device *ipc0_instance;
	volatile uint32_t delay = 0;
	struct ipc_ept_cfg ep_cfg = {
		.cb =
			{
				.bound = ep_bound,
				.received = ep_recv,
			},
	};

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
