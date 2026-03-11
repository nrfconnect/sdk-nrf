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
#include <drivers/gpio/hpf_gpio.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <haly/nrfy_gpio.h>

#define HRT_IRQ_PRIORITY          2
#define HRT_VEVIF_IDX_GPIO_CLEAR  17
#define HRT_VEVIF_IDX_GPIO_SET    18
#define HRT_VEVIF_IDX_GPIO_TOGGLE 19

#define VEVIF_IRQN(vevif) VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

#define VIO_PIN_NOT_CONNECTED 0xFF

#if defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54LM20A)
static const uint8_t vio_index_to_pin_map[] = {
	[0]  = 1,  /* Physical pin 2.01 */
	[1]  = 2,  /* Physical pin 2.02 */
	[2]  = 4,  /* Physical pin 2.04 */
	[3]  = 3,  /* Physical pin 2.03 */
	[4]  = 0,  /* Physical pin 2.00 */
	[5]  = 5,  /* Physical pin 2.05 */
	[6]  = 6,  /* Physical pin 2.06 */
	[7]  = 7,  /* Physical pin 2.07 */
	[8]  = 8,  /* Physical pin 2.08 */
	[9]  = 9,  /* Physical pin 2.09 */
	[10] = 10, /* Physical pin 2.10 */
};

#define VIO_GPIO_PORT  2
#define VIO_PIN_COUNT ARRAY_SIZE(vio_index_to_pin_map)

#elif defined(CONFIG_SOC_NRF54LV10A)
static const uint8_t vio_index_to_pin_map[] = {
	[0] = 16, /* Physical pin 1.16 */
	[1] = 17, /* Physical pin 1.17 */
	[2] = 19, /* Physical pin 1.19 */
	[3] = 18, /* Physical pin 1.18 */
	[4] = 15, /* Physical pin 1.15 */
	[5] = 20, /* Physical pin 1.20 */
	[6] = 21, /* Physical pin 1.21 */
	[7] = 22, /* Physical pin 1.22 */
	[8] = 23, /* Physical pin 1.23 */
	[9] = 24, /* Physical pin 1.24 */
};

#define VIO_GPIO_PORT  1
#define VIO_PIN_COUNT ARRAY_SIZE(vio_index_to_pin_map)

#else
#error "Unsupported target"
#endif

volatile uint16_t irq_arg;

static nrf_gpio_pin_pull_t get_pull(gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		return NRF_GPIO_PIN_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		return NRF_GPIO_PIN_PULLDOWN;
	}

	return NRF_GPIO_PIN_NOPULL;
}

static int gpio_hpf_pin_configure(uint16_t pin, uint32_t flags)
{
	if (pin >= ARRAY_SIZE(vio_index_to_pin_map)) {
		return -EINVAL;
	}

	uint32_t abs_pin = NRF_GPIO_PIN_MAP(VIO_GPIO_PORT, vio_index_to_pin_map[pin]);
	nrf_gpio_pin_pull_t pull = get_pull(flags);
	nrf_gpio_pin_drive_t drive;

	switch (flags & (NRF_GPIO_DRIVE_MSK | GPIO_OPEN_DRAIN)) {
	case NRF_GPIO_DRIVE_S0S1:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case NRF_GPIO_DRIVE_S0H1:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case NRF_GPIO_DRIVE_H0S1:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case NRF_GPIO_DRIVE_H0H1:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case NRF_GPIO_DRIVE_S0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_S0D1;
		break;
	case NRF_GPIO_DRIVE_H0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_H0D1;
		break;
	case NRF_GPIO_DRIVE_S1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case NRF_GPIO_DRIVE_H1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0H1;
		break;
	default:
		return -EINVAL;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs | (BIT(pin)));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs & ~(BIT(pin)));
	}

	nrf_gpio_pin_dir_t dir =
		(flags & GPIO_OUTPUT) ? NRF_GPIO_PIN_DIR_OUTPUT : NRF_GPIO_PIN_DIR_INPUT;
	nrf_gpio_pin_input_t input =
		(flags & GPIO_INPUT) ? NRF_GPIO_PIN_INPUT_CONNECT : NRF_GPIO_PIN_INPUT_DISCONNECT;

	/* Reconfigure the GPIO pin with the specified pull-up/pull-down configuration and drive
	 * strength.
	 */
	nrfy_gpio_reconfigure(abs_pin, &dir, &input, &pull, &drive, NULL);

	if (dir == NRF_GPIO_PIN_DIR_OUTPUT) {
		nrf_vpr_csr_vio_dir_set(nrf_vpr_csr_vio_dir_get() | (BIT(pin)));
	}

	/* Take control of the pin */
	nrfy_gpio_pin_control_select(abs_pin, NRF_GPIO_PIN_SEL_VPR);

	return 0;
}

void process_packet(hpf_gpio_data_packet_t *packet)
{
	switch (packet->opcode) {
	case HPF_GPIO_PIN_CONFIGURE: {
		gpio_hpf_pin_configure(packet->pin, packet->flags);
		break;
	}
	case HPF_GPIO_PIN_CLEAR: {
		irq_arg = packet->pin;
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_CLEAR));
		break;
	}
	case HPF_GPIO_PIN_SET: {
		irq_arg = packet->pin;
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_SET));
		break;
	}
	case HPF_GPIO_PIN_TOGGLE: {
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


__attribute__ ((interrupt)) void hrt_handler_clear_bits(void)
{
	hrt_clear_bits();
}

__attribute__ ((interrupt)) void hrt_handler_set_bits(void)
{
	hrt_set_bits();
}

__attribute__ ((interrupt)) void hrt_handler_toggle_bits(void)
{
	hrt_toggle_bits();
}

int main(void)
{
	int ret = 0;

	ret = backend_init(process_packet);
	if (ret < 0) {
		return 0;
	}

	HRT_CONNECT(HRT_VEVIF_IDX_GPIO_CLEAR, hrt_handler_clear_bits);
	HRT_CONNECT(HRT_VEVIF_IDX_GPIO_SET, hrt_handler_set_bits);
	HRT_CONNECT(HRT_VEVIF_IDX_GPIO_TOGGLE, hrt_handler_toggle_bits);

	nrf_vpr_csr_rtperiph_enable_set(true);

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
