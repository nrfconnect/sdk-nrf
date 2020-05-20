/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <spinlock.h>
#include <logging/log.h>

#include "st25r3911b_reg.h"
#include "st25r3911b_spi.h"
#include "st25r3911b_interrupt.h"
#include "st25r3911b_dt.h"

LOG_MODULE_DECLARE(st25r3911b);

#define IRQ_REG_CNT 3
#define IRQ_REG_READ_MAX_CNT 5

#define IRQ_PORT DT_GPIO_LABEL(ST25R3911B_NODE, irq_gpios)
#define IRQ_PIN DT_GPIO_PIN(ST25R3911B_NODE, irq_gpios)

static struct gpio_callback gpio_cb;
static struct device *gpio_dev;

static struct k_spinlock spinlock;
static struct k_sem *sem;

static u32_t irq_mask;

static void irq_pin_cb(struct device *gpiob, struct gpio_callback *cb,
		       u32_t pins)
{
	k_sem_give(sem);
}

static int gpio_init(void)
{
	int err;

	LOG_DBG("Setting up interrupts on %s pin %d", IRQ_PORT, IRQ_PIN);

	gpio_dev = device_get_binding(IRQ_PORT);
	if (!gpio_dev) {
		LOG_ERR("GPIO device binding error: can't find %s.", IRQ_PORT);
		return -ENXIO;
	}

	/* Configure IRQ pin */
	err = gpio_pin_configure(gpio_dev, IRQ_PIN, GPIO_INPUT);
	if (err) {
		return err;
	}

	gpio_init_callback(&gpio_cb, irq_pin_cb, BIT(IRQ_PIN));

	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		return err;
	};

	return gpio_pin_interrupt_configure(gpio_dev,
					    IRQ_PIN,
					    GPIO_INT_EDGE_RISING);
}

u32_t st25r3911b_irq_read(void)
{
	u8_t data[IRQ_REG_CNT] = {0};
	u32_t status = 0;
	int value = 0;
	u8_t cnt = 0;
	int err;

	/* In some cases, interrupts come so quickly that
	 * the state of the pin will not change
	 */
	do {
		cnt++;

		err = st25r3911b_multiple_reg_read(ST25R3911B_REG_MAIN_INT,
						   data, ARRAY_SIZE(data));
		if (err) {
			status = 0;
			break;
		}

		status = (u32_t)data[0];
		status |= ((u32_t)data[1] << 8);
		status |= ((u32_t)data[2] << 16);

		value = gpio_pin_get_raw(gpio_dev, IRQ_PIN);
		value = (value < 0) ? 0 : value;

		/* Limiting the number of interrupt reads,
		 * Protection against infinite loop.
		 */
		if (cnt >= IRQ_REG_READ_MAX_CNT) {
			break;
		}

	} while (value);

	LOG_DBG("Read interrupt, status: %u", status);

	status &= ~irq_mask;

	return status;
}

int st25r3911b_irq_modify(u32_t clr_mask, u32_t set_mask)
{
	int err = 0;
	u32_t mask;
	u32_t old_mask;
	struct k_spinlock_key key;

	key = k_spin_lock(&spinlock);

	old_mask = irq_mask;
	mask = (~old_mask & set_mask) | (old_mask & clr_mask);

	old_mask &= ~clr_mask;
	old_mask |= set_mask;

	irq_mask = old_mask;

	for (size_t i = 0; i < IRQ_REG_CNT; i++) {
		if (!((mask >> (8 * i)) & 0xFF)) {
			continue;
		}

		u8_t val = (u8_t)(old_mask >> (8 * i));

		err = st25r3911b_reg_write(ST25R3911B_REG_MASK_MAIN_INT + i,
					   val);
		if (err) {
			break;
		}
	}

	k_spin_unlock(&spinlock, key);

	LOG_DBG("Interrupts modified, current state %u", old_mask);

	return err;
}

int st25r3911b_irq_enable(u32_t mask)
{
	return st25r3911b_irq_modify(mask, 0);
}

int st25r3911b_irq_disable(u32_t mask)
{
	return st25r3911b_irq_modify(0, mask);
}

int st25r39_irq_clear(void)
{
	u8_t val[IRQ_REG_CNT] = {0};

	return st25r3911b_multiple_reg_read(ST25R3911B_REG_MAIN_INT,
					    val, ARRAY_SIZE(val));
}

u32_t st25r3911b_irq_wait_for_irq(u32_t mask, s32_t timeout)
{
	u32_t status;
	int err;

	err = k_sem_take(sem, K_MSEC(timeout));
	if (err) {
		LOG_DBG("Wait for interrupt %u, not received in time %u ms",
			mask, timeout);
		return 0;
	}

	status = st25r3911b_irq_read();

	status &= mask;

	return status;
}

int st25r3911b_irq_init(struct k_sem *irq_sem)
{
	sem = irq_sem;

	return gpio_init();
}
