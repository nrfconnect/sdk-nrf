/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "./backend/backend.h"
#include "./hrt/hrt.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <drivers/gpio/nrfe_gpio.h>
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

void mspi_init()
{

    nrf_vpr_csr_vio_dir_set( (0x1<<CS_BIT)+ (0x1<<SCLK_BIT));
    nrf_vpr_csr_vio_out_set( (0x1<<CS_BIT)+ (0x0<<SCLK_BIT));
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
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_CLEAR));
		break;
	}
	case NRFE_GPIO_PIN_SET: {
		irq_arg = packet->pin;
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_SET));
		break;
	}
	case NRFE_GPIO_PIN_TOGGLE: {
		irq_arg = packet->pin;
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_TOGGLE));
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


int main(void)
{
	int ret = 0;

	nrf_vpr_csr_rtperiph_enable_set(true);

	ret = backend_init(process_packet);
	if (ret < 0) {
		return 0;
	}

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
