/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TEST_EGPIO_H__
#define __TEST_EGPIO_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, test_egpio), okay)

/* Execution of the test requires hardware configuration described in
 * devicetree. See the test, egpio_basic_api binding local to this test
 * for details.
 */
#define DEV_OUT DT_GPIO_CTLR(DT_INST(0, test_egpio), out_gpios)
#define DEV_IN DT_GPIO_CTLR(DT_INST(0, test_egpio), in_gpios)
#define DEV DEV_OUT
#define PIN_OUT DT_GPIO_PIN(DT_INST(0, test_egpio), out_gpios)
#define PIN_OUT_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_egpio), out_gpios)
#define PIN_IN DT_GPIO_PIN(DT_INST(0, test_egpio), in_gpios)
#define PIN_IN_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_egpio), in_gpios)
#else
#error Unsupported board
#endif

#ifndef PIN_OUT
/* For build-only testing use fixed pins. */
#define PIN_OUT 10
#define PIN_IN 14
#endif

#define MAX_INT_CNT 3
struct drv_data {
	struct gpio_callback gpio_cb;
	gpio_flags_t mode;
	int index;
	int aux;
};

void test_egpio_pin_read_write(void);
void test_egpio_callback_add_remove(void);
void test_egpio_callback_self_remove(void);
void test_egpio_callback_enable_disable(void);
void test_egpio_callback_variants(void);

void test_egpio_port(void);

void test_egpio_deprecated(void);

#endif /* __TEST_EGPIO_H__ */
