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
#define TEST_NODE DT_INST(0, test_hpf_gpio)

BUILD_ASSERT(DT_PROP_LEN(TEST_NODE, out_gpios) == DT_PROP_LEN(TEST_NODE, in_gpios),
	     "out-gpios and in-gpios must have the same number of entries");
#else
#error Unsupported board
#endif

#endif /* __TEST_GPIO_H__ */
