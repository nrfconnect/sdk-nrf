/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TEST_GPIO_H__
#define __TEST_GPIO_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, test_hpf_gpio), okay)

/* Execution of the test requires hardware configuration described in
 * devicetree. See the test, hpf_gpio_basic_api binding local to this test
 * for details.
 */
#define DEV_OUT DT_GPIO_CTLR(DT_INST(0, test_hpf_gpio), out_gpios)
#define DEV_IN DT_GPIO_CTLR(DT_INST(0, test_hpf_gpio), in_gpios)
#define PIN_OUT DT_GPIO_PIN(DT_INST(0, test_hpf_gpio), out_gpios)
#define PIN_OUT_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_hpf_gpio), out_gpios)
#define PIN_IN DT_GPIO_PIN(DT_INST(0, test_hpf_gpio), in_gpios)
#define PIN_IN_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_hpf_gpio), in_gpios)
#else
#error Unsupported board
#endif

#ifndef PIN_OUT
/* For build-only testing use fixed pins. */
#define PIN_OUT 10
#define PIN_IN 14
#endif

#endif /* __TEST_GPIO_H__ */
