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

#include <mpsl_cx_abstract_interface.h>
#include <mpsl/mpsl_cx_config_thread.h>

#include <stddef.h>
#include <stdint.h>

#include <device.h>
#include <drivers/gpio.h>

#include "hal/nrf_gpio.h"

/* Value from chapter 7. Logic Timing from Thread Radio Coexistence */
#define REQUEST_TO_GRANT_US 50U

static struct mpsl_cx_thread_interface_config config;
static mpsl_cx_cb_t callback;

static const struct device *req_dev;
static const struct device *pri_dev;
static const struct device *gra_dev;
static gpio_port_value_t    req_pin_mask;
static gpio_port_value_t    pri_pin_mask;
static gpio_port_value_t    gra_pin_mask;
static struct gpio_callback grant_cb;

static int32_t grant_pin_is_asserted(bool *is_asserted)
{
	int ret;
	gpio_port_value_t port_val;

	ret = gpio_port_get_raw(gra_dev, &port_val);

	if (ret != 0) {
		return -NRF_EPERM;
	}

	*is_asserted = (port_val & gra_pin_mask) ? true : false;
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

	if (callback != NULL) {
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
			callback(granted_ops);
		}
	}
}

static int32_t gpio_init(uint8_t pin_no, bool input, const struct device **port,
		gpio_port_value_t *pin_mask)
{
	gpio_flags_t flags;
	bool use_port_1 = (pin_no > P0_PIN_NUM);

	pin_no = use_port_1 ? pin_no - P0_PIN_NUM : pin_no;
	*port = device_get_binding(use_port_1 ? "GPIO_1" : "GPIO_0");

	if (*port == NULL) {
		return -NRF_EINVAL;
	}

	*pin_mask = 1U << pin_no;

	if (input) {
		flags = GPIO_INPUT | GPIO_PULL_UP;
	} else {
		flags = GPIO_OUTPUT_LOW;
	}

	gpio_pin_configure(*port, pin_no, flags);

	return 0;
}

static int32_t gpiote_init(void)
{
	int32_t ret;
	gpio_flags_t flags = GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_EDGE_BOTH;

	ret = gpio_init(config.granted_pin, true, &gra_dev, &gra_pin_mask);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure(gra_dev, config.granted_pin, flags);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&grant_cb, gpiote_irq_handler, gra_pin_mask);
	gpio_add_callback(gra_dev, &grant_cb);

	return 0;
}

static int32_t request(const mpsl_cx_request_t *req_params)
{
	int ret;

	if (req_params == NULL) {
		return -NRF_EINVAL;
	}

	if (req_params->prio > (UINT8_MAX / 2)) {
		ret = gpio_port_set_clr_bits_raw(pri_dev, pri_pin_mask, 0);
	} else {
		ret = gpio_port_set_clr_bits_raw(pri_dev, 0, pri_pin_mask);
	}

	if (ret < 0) {
		return -NRF_EPERM;
	}

	if (req_params->ops & (MPSL_CX_OP_RX | MPSL_CX_OP_TX)) {
		ret = gpio_port_set_clr_bits_raw(req_dev, req_pin_mask, 0);
	} else {
		ret = gpio_port_set_clr_bits_raw(req_dev, 0, req_pin_mask);
	}

	if (ret < 0) {
		return -NRF_EPERM;
	}

	return 0;
}

static int32_t release(void)
{
	int ret;

	ret = gpio_port_set_clr_bits_raw(req_dev, 0, req_pin_mask);
	if (ret < 0) {
		return -NRF_EPERM;
	}

	ret = gpio_port_set_clr_bits_raw(pri_dev, 0, pri_pin_mask);
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

	return 0;
}

static const mpsl_cx_interface_t m_mpsl_cx_methods = {
	.p_request             = request,
	.p_release             = release,
	.p_granted_ops_get     = granted_ops_get,
	.p_req_grant_delay_get = req_grant_delay_get,
	.p_register_callback   = register_callback,
};

int32_t mpsl_cx_thread_interface_config_set(
		struct mpsl_cx_thread_interface_config const * const new_config)
{
	int32_t ret_code;

	config = *new_config;
	callback = NULL;

	ret_code = mpsl_cx_interface_set(&m_mpsl_cx_methods);
	if (ret_code != 0) {
		return ret_code;
	}

	ret_code = gpio_init(config.request_pin, false, &req_dev, &req_pin_mask);
	if (ret_code != 0) {
		return ret_code;
	}

	ret_code = gpio_init(config.priority_pin, false, &pri_dev, &pri_pin_mask);
	if (ret_code != 0) {
		return ret_code;
	}

	ret_code = gpiote_init();
	if (ret_code != 0) {
		return ret_code;
	}

	return 0;
}
