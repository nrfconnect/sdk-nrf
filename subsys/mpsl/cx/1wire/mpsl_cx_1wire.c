/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements a generic 1-wire coexistence interface.
 */

#if defined(CONFIG_MPSL_CX_PIN_FORWARDER)
#include <string.h>
#include <soc_secure.h>
#else
#include <mpsl_cx_abstract_interface.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <soc_nrf_common.h>

#include "hal/nrf_gpio.h"
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>

#if DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_coex))
#define CX_NODE DT_NODELABEL(nrf_radio_coex)
#else
#define CX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

#if DT_NODE_HAS_PROP(CX_NODE, concurrency_mode)
#define ALLOW_RX DT_PROP(CX_NODE, concurrency_mode)
#else
#define ALLOW_RX false
#endif

#if !(DT_NODE_HAS_COMPAT(CX_NODE, generic_radio_coex_one_wire) || \
		  DT_NODE_HAS_COMPAT(CX_NODE, sdc_radio_coex_one_wire))
#error Selected coex node is not compatible with generic-radio-coex-one-wire.
#endif

#if !defined(CONFIG_MPSL_CX_PIN_FORWARDER)

#define GRANT_PIN_PORT_NO  DT_PROP(DT_GPIO_CTLR(CX_NODE, grant_gpios), port)
#define GRANT_PIN_PIN_NO   DT_GPIO_PIN(CX_NODE, grant_gpios)

static nrfx_gpiote_t *gpiote =
	&GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(CX_NODE, grant_gpios));

static const struct gpio_dt_spec gra_spec = GPIO_DT_SPEC_GET(CX_NODE, grant_gpios);

static mpsl_cx_cb_t callback;
static struct gpio_callback grant_cb;
static uint32_t grant_abs_pin;

static bool grant_pin_is_asserted(void)
{
	int ret = gpio_pin_get_dt(&gra_spec);

	__ASSERT(ret >= 0, "Error while reading GPIO state.");

	return ret;
}

static mpsl_cx_op_map_t granted_ops_map_get(void)
{
#if ALLOW_RX
	mpsl_cx_op_map_t granted_ops = MPSL_CX_OP_IDLE_LISTEN | MPSL_CX_OP_RX;
#else
	mpsl_cx_op_map_t granted_ops = MPSL_CX_OP_IDLE_LISTEN;
#endif

	if (grant_pin_is_asserted()) {
		granted_ops |= MPSL_CX_OP_TX | MPSL_CX_OP_RX;
	}

	return granted_ops;
}

static int32_t granted_ops_get(mpsl_cx_op_map_t *granted_ops)
{
	*granted_ops = granted_ops_map_get();
	return 0;
}

static void gpiote_irq_handler(const struct device *gpiob, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(gpiob);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (callback) {
		mpsl_cx_op_map_t granted_ops = granted_ops_map_get();

		callback(granted_ops);
	}
}

static int32_t request(const mpsl_cx_request_t *req_params)
{
	ARG_UNUSED(req_params);
	return 0;
}

static int32_t release(void)
{
	return 0;
}

static uint32_t req_grant_delay_get(void)
{
	return 0;
}

static int32_t register_callback(mpsl_cx_cb_t cb)
{
	callback = cb;

	if (cb) {
		nrfx_gpiote_trigger_enable(gpiote, grant_abs_pin, true);
	} else {
		nrfx_gpiote_trigger_disable(gpiote, grant_abs_pin);
	}

	return 0;
}

static const mpsl_cx_interface_t m_mpsl_cx_methods = {
	.p_request             = request,
	.p_release             = release,
	.p_granted_ops_get     = granted_ops_get,
	.p_req_grant_delay_get = req_grant_delay_get,
	.p_register_callback   = register_callback,
};

static int mpsl_cx_init(void)
{
	int32_t ret;

	callback = NULL;

	ret = mpsl_cx_interface_set(&m_mpsl_cx_methods);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&gra_spec, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&gra_spec,
		GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		return ret;
	}

	grant_abs_pin = NRF_GPIO_PIN_MAP(GRANT_PIN_PORT_NO, GRANT_PIN_PIN_NO);
	nrfx_gpiote_trigger_disable(gpiote, grant_abs_pin);

	gpio_init_callback(&grant_cb, gpiote_irq_handler, BIT(gra_spec.pin));
	gpio_add_callback(gra_spec.port, &grant_cb);

	return 0;
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#else /* !defined(CONFIG_MPSL_CX_PIN_FORWARDER) */
static int mpsl_cx_init(void)
{
#if DT_NODE_HAS_PROP(CX_NODE, grant_gpios)
	uint8_t grant_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, grant_gpios);

	soc_secure_gpio_pin_mcu_select(grant_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

	return 0;
}

SYS_INIT(mpsl_cx_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* !defined(CONFIG_MPSL_CX_PIN_FORWARDER) */
