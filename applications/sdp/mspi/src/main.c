/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "./backend/backend.h"
#include "./hrt/hrt.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <haly/nrfy_gpio.h>

#include <zephyr/mspi.h>

#define HRT_IRQ_PRIORITY          2
#define HRT_VEVIF_IDX_WRITE_SINGLE  17
#define HRT_VEVIF_IDX_WRITE_QUAD    18

#define VEVIF_IRQN(vevif) VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

struct mspi_config {
	uint8_t* data;
	uint8_t data_len;
	uint8_t word_size;
}


volatile uint16_t counter_top;
volatile uint16_t word_size;
volatile uint16_t data;
volatile uint16_t data_len;

void mspi_configure(void)
{
    nrf_vpr_csr_vio_dir_set( (0x1<<CS_BIT) | (0x1<<SCLK_BIT));
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT) | (0x0<<SCLK_BIT));

	nrf_vpr_csr_vio_config_t vio_config = {
		.clk_polarity = 0,
		.input_sel = 0,
		.stop_cnt = 0,
	};

	nrf_vpr_csr_vio_config_set(&vio_config);
}

void process_packet(const void *data, size_t len)
{
	(void)len;
	nrfe_mspi_flpr_response_t response;
	uint8_t opcode = *(uint8_t *)data;

	switch (opcode) {
		case NRFE_MSPI_CTRL_CONFIG: {
			response.opcode = NRFE_MSPI_CTRL_CONFIG_DONE;
			struct mspi_cfg *cfg = (struct mspi_cfg *)data;

			/* Not supported options. Some may be added later, for example, sw_multi_periph. */
			if (cfg->op_mode == MSPI_OP_MODE_PERIPHERAL |
				cfg->duplex == MSPI_FULL_DUPLEX |
				cfg->dqs_support |
				cfg->sw_multi_periph)
			{
				return;
			}

			for (uint8_t i=0; i < cfg->num_ce_gpios; i++)
			{
				//init cfg->ce_group[i]
			}

			/* Ignore freq_max and re_init for now. */
			break;
		}
		case NRFE_MSPI_DEV_CONFIG: {
			response.opcode = NRFE_MSPI_DEV_CONFIG_DONE;
			struct mspi_dev_cfg *cfg = (struct mspi_dev_cfg *)data;

			/* Not supported options. Some may be added later, for example, sw_multi_periph. */
			if (cfg->data_rate == MSPI_DATA_RATE_S_S_D |
				cfg->data_rate == MSPI_DATA_RATE_S_D_D |
				cfg->data_rate == MSPI_DATA_RATE_DUAL |
				cfg->dqs_enable)
			{
				return;
			}
			/* TODO: Process device config data */
			/* Ignore ce_num, freq, mem_boundary and time_to_break for now. */
			break;
		}
		case NRFE_MSPI_XFER_CONFIG: {
			response.opcode = NRFE_MSPI_XFER_CONFIG_DONE;
			struct mspi_xfer *cfg = (struct mspi_xfer *)data;
			/* TODO: Process cfer config data */
			break;
		}
		case NRFE_MSPI_XFER: {
			struct mspi_xfer_packet *cfg = (struct mspi_xfer_packet *)data;

			if (packet->packet.dir == MSPI_TX) {

				/* TODO: Send data */
				response.opcode = NRFE_MSPI_TX_DONE;
			}
			else if (packet->packet.dir == MSPI_RX) {
				response.opcode = NRFE_MSPI_RX_DONE;
			}

			break;
		}
		default:
			break;
	}

	int ret = ipc_service_send(&ep, (const void *)&response.opcode, sizeof(response));
	if (ret < 0) {
		// printf("IPC send error: %d\n", ret);
	}
}

#define HRT_CONNECT(vevif, handler)                                            \
	IRQ_DIRECT_CONNECT(vevif, HRT_IRQ_PRIORITY, handler, 0);               \
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(vevif), true)


__attribute__ ((interrupt)) void hrt_handler_write_single(void)
{
	// hrt_clear_bits();
}

__attribute__ ((interrupt)) void hrt_handler_write_quad(void)
{
	// hrt_set_bits();
}

int main(void)
{
	// int ret = 0;

	// ret = backend_init(process_packet);
	// if (ret < 0) {
	// 	return 0;
	// }

	HRT_CONNECT(HRT_VEVIF_IDX_WRITE_SINGLE, hrt_handler_clear_bits);
	HRT_CONNECT(HRT_VEVIF_IDX_WRITE_QUAD, hrt_handler_set_bits);

	nrf_vpr_csr_rtperiph_enable_set(true);

	// nrf_gpio_pin_dir_t dir = NRF_GPIO_PIN_DIR_OUTPUT;
	// nrf_gpio_pin_input_t input = NRF_GPIO_PIN_INPUT_DISCONNECT;
	// nrf_gpio_pin_pull_t pull = NRF_GPIO_PIN_NOPULL;
	// nrf_gpio_pin_drive_t drive = NRF_GPIO_PIN_S0S1;

	// nrfy_gpio_reconfigure(NRF_GPIO_PIN_MAP(2, CS_BIT), &dir, &input, &pull, &drive, NULL);

	nrfy_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, 0), NRF_GPIO_PIN_SEL_VPR);
	nrfy_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, 1), NRF_GPIO_PIN_SEL_VPR);
	nrfy_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, 2), NRF_GPIO_PIN_SEL_VPR);
	nrfy_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, 3), NRF_GPIO_PIN_SEL_VPR);
	nrfy_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, 4), NRF_GPIO_PIN_SEL_VPR);
	nrfy_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, CS_BIT), NRF_GPIO_PIN_SEL_VPR);

	mspi_configure();

	uint8_t len1 = 3;
	uint32_t to_send1[3] = {0x10, 0x25, 0x3278};

	uint8_t len = 5;
	uint32_t to_send[5] = {0x10, 0x93708965, 0x25, 0x5060, 0x3278};

	write_single_by_word(to_send1, len1, 4, 32);

	write_quad_by_word(to_send, len, 4, 32);

	k_msleep(1);

	uint8_t len2 = 3;
	uint32_t to_send2[3] = {0xff, 0x10, 0xf6};
	write_quad_by_word(to_send2, len2, 4, 16);

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
