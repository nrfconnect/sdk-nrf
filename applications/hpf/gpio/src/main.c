/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "./backend/backend.h"
#include "./hrt/hrt.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
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

#if defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54LM20A) || defined(CONFIG_SOC_NRF54LM20B)
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

#define VIO_PORT 2
#define VIO_PIN_OFFSET 0

#elif defined(CONFIG_SOC_NRF54LV10A)
static const uint8_t pin_to_vio_map[] = {
	4,  /* Physical pin 15 */
	0,  /* Physical pin 16 */
	1,  /* Physical pin 17 */
	3,  /* Physical pin 18 */
	2,  /* Physical pin 19 */
	5,  /* Physical pin 20 */
	6,  /* Physical pin 21 */
	7,  /* Physical pin 22 */
	8,  /* Physical pin 23 */
	9,  /* Physical pin 24 */
};

#define VIO_PORT 1
#define VIO_PIN_OFFSET 15

#else
#error "Unsupported target"
#endif

#define VIO_INDEX_INVALID UINT8_MAX
#define VIO_PIN_MASK_INVALID UINT16_MAX

#define VIO_PIN_COUNT ARRAY_SIZE(pin_to_vio_map)
#define VIO_VALID_PIN_MASK ((BIT(VIO_PIN_COUNT) - 1) << VIO_PIN_OFFSET)

volatile uint16_t irq_arg;

static uint8_t gpio_pin_port_to_vio_index(uint8_t port, uint16_t pin)
{
	/* Check if the pin and the port can be accessed by VIO. */
	if ((port != VIO_PORT) || (pin < VIO_PIN_OFFSET) ||
	    (pin >= (VIO_PIN_OFFSET + VIO_PIN_COUNT))) {
		return VIO_INDEX_INVALID;
	}
	return pin_to_vio_map[pin - VIO_PIN_OFFSET];
}

static uint16_t gpio_pin_mask_to_vio_mask(uint32_t gpio_pin_mask)
{
	/* Check if all of the pins specified by the mask can be accessed by VIO. */
	if ((gpio_pin_mask & (~VIO_VALID_PIN_MASK)) != 0) {
		return VIO_PIN_MASK_INVALID;
	}
	uint16_t vio_mask = 0;

	for (int i = 0; i < VIO_PIN_COUNT; i++) {
		if (gpio_pin_mask & BIT(i + VIO_PIN_OFFSET)) {
			vio_mask |= BIT(pin_to_vio_map[i]);
		}
	}
	return vio_mask;
}

static nrf_gpio_pin_pull_t get_pull(gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		return NRF_GPIO_PIN_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		return NRF_GPIO_PIN_PULLDOWN;
	}

	return NRF_GPIO_PIN_NOPULL;
}

static int gpio_hpf_pin_configure(uint8_t port, uint16_t pin, uint32_t flags)
{
	uint8_t pin_vio_index = gpio_pin_port_to_vio_index(port, pin);

	if (pin_vio_index == VIO_INDEX_INVALID) {
		return -EINVAL;
	}
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(port, pin);
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

		nrf_vpr_csr_vio_out_set(outs | (BIT(pin_vio_index)));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs & ~(BIT(pin_vio_index)));
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
		nrf_vpr_csr_vio_dir_set(nrf_vpr_csr_vio_dir_get() | (BIT(pin_vio_index)));
	}

	/* Take control of the pin */
	nrfy_gpio_pin_control_select(abs_pin, NRF_GPIO_PIN_SEL_VPR);

	return 0;
}

void process_packet(hpf_gpio_data_packet_t *packet)
{
	if (packet->opcode == HPF_GPIO_PIN_CONFIGURE) {
		int ret = gpio_hpf_pin_configure(packet->port, packet->pin, packet->flags);

		/* Prevent warnings when asserts are disabled. */
		(void)ret;
		__ASSERT_NO_MSG(ret == 0);
	} else {
		uint16_t vio_mask = gpio_pin_mask_to_vio_mask(packet->pin);

		__ASSERT_NO_MSG((packet->port == VIO_PORT) && vio_mask != VIO_PIN_MASK_INVALID);

		irq_arg = vio_mask;
		switch (packet->opcode) {
		case HPF_GPIO_PIN_CLEAR:
			nrf_vpr_clic_int_pending_set(NRF_VPRCLIC,
						     VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_CLEAR));
			break;
		case HPF_GPIO_PIN_SET:
			nrf_vpr_clic_int_pending_set(NRF_VPRCLIC,
						     VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_SET));
			break;
		case HPF_GPIO_PIN_TOGGLE:
			nrf_vpr_clic_int_pending_set(NRF_VPRCLIC,
						     VEVIF_IRQN(HRT_VEVIF_IDX_GPIO_TOGGLE));
			break;
		default:
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

	HRT_CONNECT(HRT_VEVIF_IDX_GPIO_CLEAR, hrt_handler_clear_bits);
	HRT_CONNECT(HRT_VEVIF_IDX_GPIO_SET, hrt_handler_set_bits);
	HRT_CONNECT(HRT_VEVIF_IDX_GPIO_TOGGLE, hrt_handler_toggle_bits);

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
