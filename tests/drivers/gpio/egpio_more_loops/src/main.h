/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TEST_EGPIO_H__
#define __TEST_EGPIO_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, test_egpio), okay)
#define DEV_OUT DT_GPIO_CTLR(DT_INST(0, test_egpio), out_gpios)
#define DEV_IN DT_GPIO_CTLR(DT_INST(0, test_egpio), in_gpios)
#define DEV DEV_OUT
#define PIN_OUT_1 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), out_gpios, 0)
#define PIN_OUT_2 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), out_gpios, 1)
#define PIN_OUT_3 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), out_gpios, 2)
#define PIN_OUT_4 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), out_gpios, 3)
#define PIN_OUT_5 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), out_gpios, 4)
#define PIN_IN_1 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), in_gpios, 0)
#define PIN_IN_2 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), in_gpios, 1)
#define PIN_IN_3 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), in_gpios, 2)
#define PIN_IN_4 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), in_gpios, 3)
#define PIN_IN_5 DT_GPIO_PIN_BY_IDX(DT_INST(0, test_egpio), in_gpios, 4)
#else
#error Unsupported board
#endif

#endif /* __TEST_EGPIO_H__ */
