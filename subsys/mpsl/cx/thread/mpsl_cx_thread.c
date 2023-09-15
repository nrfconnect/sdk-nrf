/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements generic Coexistence interface according to
 *   Thread Radio Coexistence whitepaper.
 *
 */


#if !defined(CONFIG_MPSL_CX_PIN_FORWARDER)
#include <mpsl_cx_abstract_interface.h>
#else
#include <string.h>
#include <soc_secure.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include "hal/nrf_gpio.h"
#include <nrfx_gpiote.h>

#if DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_coex))
#define CX_NODE DT_NODELABEL(nrf_radio_coex)
#else
#define CX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

#if !defined(CONFIG_MPSL_CX_PIN_FORWARDER)

/* Value from chapter 7. Logic Timing from Thread Radio Coexistence */
#define REQUEST_TO_GRANT_US 50U

#define GRANT_PIN_PORT_NO  DT_PROP(DT_GPIO_CTLR(CX_NODE, grant_gpios), port)
#define GRANT_PIN_PIN_NO   DT_GPIO_PIN(CX_NODE, grant_gpios)

static const struct gpio_dt_spec req_spec = GPIO_DT_SPEC_GET(CX_NODE, req_gpios);
static const struct gpio_dt_spec pri_spec = GPIO_DT_SPEC_GET(CX_NODE, pri_dir_gpios);
static const struct gpio_dt_spec gra_spec = GPIO_DT_SPEC_GET(CX_NODE, grant_gpios);

static mpsl_cx_cb_t callback;
static struct gpio_callback grant_cb;
static uint32_t grant_abs_pin;

static int32_t grant_pin_is_asserted(bool *is_asserted)
{
	int ret = gpio_pin_get_dt(&gra_spec);

	if (ret < 0) {
		return ret;
	}

	*is_asserted = (bool)ret;
	return 0;
}

static mpsl_cx_op_map_t granted_ops_map(bool grant_is_asserted)
{
	mpsl_cx_op_map_t granted_ops = MPSL_CX_OP_IDLE_LISTEN | MPSL_CX_OP_RX;

	if (grant_is_asserted) {
		granted_ops |= MPSL_CX_OP_TX;
	}

	return granted_ops;
}

static int32_t granted_ops_get(mpsl_cx_op_map_t *granted_ops)
{
	int  ret;
	bool grant_is_asserted;

	ret = grant_pin_is_asserted(&grant_is_asserted);
	if (ret < 0) {
		return ret;
	}

	*granted_ops = granted_ops_map(grant_is_asserted);
	return 0;
}

static void gpiote_irq_handler(const struct device *gpiob, struct gpio_callback *cb, uint32_t pins)
{
	(void)gpiob;
	(void)cb;
	(void)pins;

	static mpsl_cx_op_map_t last_notified;
	int32_t ret;
	mpsl_cx_op_map_t granted_ops;
	mpsl_cx_cb_t callback_copy = callback;

	if (callback_copy != NULL) {
		ret = granted_ops_get(&granted_ops);

		__ASSERT(ret == 0, "Getting grant pin state returned unexpected result: %d", ret);
		if (ret != 0) {
			/* nrfx gpio implementation cannot return failure for this call
			 * This condition is handled for fail-safe approach. It is assumed
			 * GRANT is not given, if cannot read its value
			 */
			granted_ops = granted_ops_map(false);
		}

		if (granted_ops != last_notified) {
			last_notified = granted_ops;
			callback_copy(granted_ops);
		}
	}
}

static int32_t request(const mpsl_cx_request_t *req_params)
{
	int ret;

	if (req_params == NULL) {
		return -NRF_EINVAL;
	}

	bool pri_active = req_params->prio > (UINT8_MAX / 2);

	ret = gpio_pin_set_dt(&pri_spec, pri_active);
	if (ret < 0) {
		return -NRF_EPERM;
	}

	bool req_active = req_params->ops & (MPSL_CX_OP_RX | MPSL_CX_OP_TX);

	ret = gpio_pin_set_dt(&req_spec, req_active);
	if (ret < 0) {
		return -NRF_EPERM;
	}

	return 0;
}

static int32_t release(void)
{
	int ret;

	ret = gpio_pin_set_dt(&req_spec, 0);
	if (ret < 0) {
		return -NRF_EPERM;
	}

	ret = gpio_pin_set_dt(&pri_spec, 0);
	if (ret < 0) {
		return -NRF_EPERM;
	}

	return 0;
}

static uint32_t req_grant_delay_get(void)
{
	return REQUEST_TO_GRANT_US;
}

static int32_t register_callback(mpsl_cx_cb_t cb)
{
	callback = cb;

	if (cb != NULL) {
		nrfx_gpiote_trigger_enable(grant_abs_pin, true);
	} else {
		nrfx_gpiote_trigger_disable(grant_abs_pin);
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

	ret = gpio_pin_configure_dt(&req_spec, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&pri_spec, GPIO_OUTPUT_INACTIVE);
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
	nrfx_gpiote_trigger_disable(grant_abs_pin);

	gpio_init_callback(&grant_cb, gpiote_irq_handler, BIT(gra_spec.pin));
	gpio_add_callback(gra_spec.port, &grant_cb);

	return 0;
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#else // !defined(CONFIG_MPSL_CX_PIN_FORWARDER)
static int mpsl_cx_init(void)
{
#if DT_NODE_HAS_PROP(CX_NODE, req_gpios)
	uint8_t req_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, req_gpios);

	soc_secure_gpio_pin_mcu_select(req_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif
#if DT_NODE_HAS_PROP(CX_NODE, pri_dir_gpios)
	uint8_t pri_dir_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, pri_dir_gpios);

	soc_secure_gpio_pin_mcu_select(pri_dir_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif
#if DT_NODE_HAS_PROP(CX_NODE, grant_gpios)
	uint8_t grant_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, grant_gpios);

	soc_secure_gpio_pin_mcu_select(grant_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

	return 0;
}

SYS_INIT(mpsl_cx_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif // !defined(CONFIG_MPSL_CX_PIN_FORWARDER)
