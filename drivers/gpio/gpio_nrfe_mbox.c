/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/printk.h>

#include "gpio_nrfe.h"

static const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
static nrfe_gpio_mbox_data_t *tx_data =
	(nrfe_gpio_mbox_data_t *)((uint8_t *)(DT_REG_ADDR(DT_NODELABEL(sram_tx))));
#define MAX_MSG_SIZE (DT_REG_SIZE(DT_NODELABEL(sram_tx)))

int gpio_send(nrfe_gpio_data_packet_t *msg)
{
	printk("Sending opcode: %d, pin %d, port %d, flag: %d\n", msg->opcode, msg->pin, msg->port,
	       msg->flags);
	/* Try and get lock */
	if (atomic_flag_test_and_set(&tx_data->lock.locked)) {
		/* Return -1 in case lock is not acquired (used by other core)*/
		return -1;
	}

	memcpy((void *)&tx_data->data, (void *)msg, sizeof(nrfe_gpio_data_packet_t));
	tx_data->lock.data_size = sizeof(nrfe_gpio_data_packet_t);

	/* Release lock */
	atomic_flag_clear(&tx_data->lock.locked);

	return mbox_send_dt(&tx_channel, NULL);
}

int gpio_nrfe_init(const struct device *port)
{
	return 0;
}
