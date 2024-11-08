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
#define HRT_VEVIF_IDX_GPIO_CLEAR  17
#define HRT_VEVIF_IDX_GPIO_SET    18
#define HRT_VEVIF_IDX_GPIO_TOGGLE 19

#define VEVIF_IRQN(vevif) VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

volatile uint16_t irq_arg;

#define CS_BIT 5
#define SCLK_BIT 0

void mspi_init(void)
{
    nrf_vpr_csr_vio_dir_set( (0x1<<CS_BIT)+ (0x1<<SCLK_BIT) + (0x1 << 1));
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT)+ (0x0<<SCLK_BIT) + (0x1 << 1));

	nrf_vpr_csr_vio_config_t vio_config = {
		.clk_polarity = 0,
		.input_sel = 0,
		.stop_cnt = 1,
	};

	nrf_vpr_csr_vio_config_set(&vio_config);
}

volatile int i = 0;

int main(void)
{
	// printk("Boootttinnggg\n");
	// k_msleep(100);

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

	mspi_init();

	// printk("Initialized\n");

	uint8_t len = 3;
	uint32_t to_send[3] = {0x10, 0x30708965, 0x25};

	// for (uint8_t i=0; i< len; i++)
	// {
	// 	printk("Word %u: %u\n", i, to_send[i]);
	// }

	write_single_by_word(to_send, len, 4);

	// k_msleep(100);
	// printk("Written\n");

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
