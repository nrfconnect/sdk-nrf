/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PERIPHERAL_STRESS_TEST_H
#define PERIPHERAL_STRESS_TEST_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/* Thread status */
extern atomic_t started_threads;
extern atomic_t completed_threads;

/* Accelerator readout via SPI: */
#define ACCEL_THREAD_COUNT_MAX		(40)
#define ACCEL_THREAD_OVERSAMPLING	(3)
#define ACCEL_THREAD_STACKSIZE		(1024)
#define ACCEL_THREAD_PRIORITY		(5)
#define ACCEL_THREAD_SLEEP			(200)

/* ADC thread: */
#define ADC_THREAD_COUNT_MAX	(40)
#define ADC_THREAD_OVERSAMPLING (3)
#define ADC_THREAD_STACKSIZE	(1024)
#define ADC_THREAD_PRIORITY		(5)
#define ADC_THREAD_SLEEP		(200)

/* BME680 environment sensor readout via I2C: */
#define BME680_THREAD_COUNT_MAX		(40)
#define BME680_THREAD_OVERSAMPLING	(2)
#define BME680_THREAD_STACKSIZE		(1024)
#define BME680_THREAD_PRIORITY		(5)
#define BME680_THREAD_SLEEP			(200)

/* CAN thread: */
#define CAN_THREAD_COUNT_MAX	(40)
#define CAN_THREAD_STACKSIZE	(1024)
#define CAN_THREAD_PRIORITY		(5)
#define CAN_THREAD_SLEEP		(200)

/* Counter thread: */
#define COUNTER_THREAD_COUNT_MAX	(40)
#define COUNTER_THREAD_STACKSIZE	(512)
#define COUNTER_THREAD_PRIORITY		(5)
#define COUNTER_THREAD_PERIOD		(200)

/* FLASH thread: */
#define FLASH_THREAD_COUNT_MAX	(40)
#define FLASH_THREAD_STACKSIZE	(2048)
#define FLASH_THREAD_PRIORITY	(5)
#define FLASH_THREAD_SLEEP		(200)

/* GPIO thread: */
#define GPIO_THREAD_COUNT_MAX	(40)
#define GPIO_THREAD_STACKSIZE	(512)
#define GPIO_THREAD_PRIORITY	(5)
#define GPIO_THREAD_SLEEP		(200)

/* I2S thread: */
#define I2S_THREAD_COUNT_MAX	(40)
#define I2S_THREAD_STACKSIZE	(1024)
#define I2S_THREAD_PRIORITY		(5)
#define I2S_THREAD_SLEEP		(200)

/* PWM thread: */
#define PWM_THREAD_COUNT_MAX	(40)
#define PWM_THREAD_OVERSAMPLING	(10)
#define PWM_THREAD_STACKSIZE	(512)
#define PWM_THREAD_PRIORITY		(5)
#define PWM_THREAD_SLEEP		(200)

/* TEMP thread: */
#define TEMP_THREAD_COUNT_MAX	(40)
#define TEMP_THREAD_STACKSIZE	(512)
#define TEMP_THREAD_PRIORITY	(5)
#define TEMP_THREAD_PERIOD		(200)

/* Timer thread: */
#define TIMER_THREAD_COUNT_MAX	(40)
#define TIMER_THREAD_STACKSIZE	(512)
#define TIMER_THREAD_PRIORITY	(5)
#define TIMER_THREAD_PERIOD		(200)

/* Watchdog thread: */
#define WDT_THREAD_COUNT_MAX	(40)
#define WDT_THREAD_STACKSIZE	(512)
#define WDT_THREAD_PRIORITY		(4)
#define WDT_THREAD_PERIOD		(200)
#define WDT_WINDOW_MAX			(2 * WDT_THREAD_PERIOD)
#define WDT_TAG					(0x12345678U)

/* Clock thread: */
#define CLOCK_THREAD_STACKSIZE	(512)
#define CLOCK_THREAD_PRIORITY	(4)


/* Busy load thread: */
#define LOAD_THREAD_STACKSIZE	(1024)
#define LOAD_THREAD_PRIORITY	(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#define LOAD_THREAD_DURATION	(ACCEL_THREAD_SLEEP * (ACCEL_THREAD_COUNT_MAX + 2))


#endif
