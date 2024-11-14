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

#define HRT_IRQ_PRIORITY          2
#define HRT_VEVIF_IDX_WRITE_SINGLE  17
#define HRT_VEVIF_IDX_WRITE_QUAD    18

#define VEVIF_IRQN(vevif) VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn


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

void process_packet(nrfe_gpio_data_packet_t *packet)
{
	if (packet->port != 2) {
		return;
	}

	switch (packet->opcode) {
	case NRFE_GPIO_PIN_CONFIGURE: {
		gpio_nrfe_pin_configure(packet->port, packet->pin, packet->flags);
		break;
	}
	case NRFE_GPIO_PIN_CLEAR: {
		irq_arg = packet->pin;
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_SINGLE));
		break;
	}
	case NRFE_GPIO_PIN_SET: {
		irq_arg = packet->pin;
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE_QUAD));
		break;
	}
	default: {
		break;
	}
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
