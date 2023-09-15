/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements the nRF700x Coexistence interface.
 *
 */

#if !defined(CONFIG_MPSL_CX_PIN_FORWARDER)
#include <mpsl_cx_abstract_interface.h>
#include <mpsl/mpsl_cx_nrf700x.h>
#else
#include <string.h>
#include <soc_secure.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include "hal/nrf_gpio.h"
#include <nrfx_gpiote.h>

/*
 * Typical part of device tree describing coex (sample port and pin numbers).
 *
 * / {
 *     nrf_radio_coex: nrf7002-coex {
 *         status = "okay";
 *         compatible = "nordic,nrf700x-coex";
 *         req-gpios =     <&gpio0 24 (GPIO_ACTIVE_HIGH)>;
 *         status0-gpios = <&gpio0 14 (GPIO_ACTIVE_HIGH)>;
 *         grant-gpios =   <&gpio0 25 (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
 *     };
 * };
 *
 */

#if DT_NODE_EXISTS(DT_NODELABEL(nrf_radio_coex))
#define CX_NODE DT_NODELABEL(nrf_radio_coex)
#else
#define CX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

#if !defined(CONFIG_MPSL_CX_PIN_FORWARDER)

#define REQUEST_LEAD_TIME 0U

#define GRANT_PIN_PORT_NO  DT_PROP(DT_GPIO_CTLR(CX_NODE, grant_gpios), port)
#define GRANT_PIN_PIN_NO   DT_GPIO_PIN(CX_NODE, grant_gpios)

static const struct gpio_dt_spec req_spec     = GPIO_DT_SPEC_GET(CX_NODE, req_gpios);
static const struct gpio_dt_spec status0_spec = GPIO_DT_SPEC_GET(CX_NODE, status0_gpios);
static const struct gpio_dt_spec grant_spec   = GPIO_DT_SPEC_GET(CX_NODE, grant_gpios);

static mpsl_cx_cb_t callback;
static struct gpio_callback grant_cb;
static uint32_t grant_abs_pin;

static bool enabled = true;

static int32_t grant_pin_is_asserted(bool *is_asserted)
{
	int ret;

	ret = gpio_pin_get_dt(&grant_spec);

	if (ret < 0) {
		return ret;
	}

	*is_asserted = (bool)ret;
	return 0;
}

static mpsl_cx_op_map_t granted_ops_map(bool grant_is_asserted)
{
	mpsl_cx_op_map_t granted_ops = MPSL_CX_OP_IDLE_LISTEN | MPSL_CX_OP_RX;

	if (grant_is_asserted || !enabled) {
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

/**
 * Calculates logic level of @c dir signal depending on operation
 *
 * @param   ops     Operations requested
 * @retval  0       @c dir signal should be inactive
 * @retval  other   @c dir signal should be active
 */
static int sig_dir_level_calc(mpsl_cx_op_map_t ops)
{
	int r = 1;

	if (ops & MPSL_CX_OP_TX) {
		r = 0;
	}

	return r;
}

/**
 * Drives @c pri-dir pin to value of @c dir signal according to requested operation.
 *
 * @param   ops Operations requested
 * @return      Result of gpio control
 */
static int32_t gpio_drive_status0_to_dir(mpsl_cx_op_map_t ops)
{
	return gpio_pin_set_dt(&status0_spec, sig_dir_level_calc(ops));
}

/**
 * Drives @c req pin to value according to @p active
 *
 * @param   active   0 drives to inactive level. Non-zero drives to active level
 * @return           Result of gpio control
 */
static int32_t gpio_drive_request(int active)
{
	return gpio_pin_set_dt(&req_spec, active);
}

static int32_t request(const mpsl_cx_request_t *req_params)
{
	int ret;

	if (req_params == NULL) {
		return -NRF_EINVAL;
	}

	ret = gpio_drive_status0_to_dir(req_params->ops);

	if (ret < 0) {
		return -NRF_EPERM;
	}

	ret = gpio_drive_request(req_params->ops & (MPSL_CX_OP_RX | MPSL_CX_OP_TX));

	if (ret < 0) {
		return -NRF_EPERM;
	}

	return 0;
}

static int32_t release(void)
{
	int ret;

	ret = gpio_drive_request(0);
	if (ret < 0) {
		return -NRF_EPERM;
	}

	ret = gpio_drive_status0_to_dir(0);

	if (ret < 0) {
		return -NRF_EPERM;
	}

	return 0;
}

static uint32_t req_grant_delay_get(void)
{
	return REQUEST_LEAD_TIME;
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

	ret = gpio_pin_configure_dt(&status0_spec, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&grant_spec, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&grant_spec,
			GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		return ret;
	}
	grant_abs_pin = NRF_GPIO_PIN_MAP(GRANT_PIN_PORT_NO, GRANT_PIN_PIN_NO);
	nrfx_gpiote_trigger_disable(grant_abs_pin);

	gpio_init_callback(&grant_cb, gpiote_irq_handler, BIT(grant_spec.pin));
	gpio_add_callback(grant_spec.port, &grant_cb);

	ret = gpio_drive_request(0);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_drive_status0_to_dir(0);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_MPSL_CX_INIT_PRIORITY);

void mpsl_cx_nrf700x_set_enabled(bool enable)
{
	enabled = enable;
}

#else // !defined(CONFIG_MPSL_CX_PIN_FORWARDER)
static int mpsl_cx_init(void)
{
#if DT_NODE_HAS_PROP(CX_NODE, req_gpios)
	uint8_t req_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, req_gpios);

	soc_secure_gpio_pin_mcu_select(req_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif
#if DT_NODE_HAS_PROP(CX_NODE, status0_gpios)
	uint8_t status0_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, status0_gpios);

	soc_secure_gpio_pin_mcu_select(status0_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif
#if DT_NODE_HAS_PROP(CX_NODE, grant_gpios)
	uint8_t grant_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, grant_gpios);

	soc_secure_gpio_pin_mcu_select(grant_pin, NRF_GPIO_PIN_SEL_NETWORK);
#endif

	return 0;
}

SYS_INIT(mpsl_cx_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif // !defined(CONFIG_MPSL_CX_PIN_FORWARDER)
