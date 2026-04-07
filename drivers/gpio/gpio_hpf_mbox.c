/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mbox.h>

#include "gpio_hpf.h"

static const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
static hpf_gpio_mbox_data_t *tx_data =
	(hpf_gpio_mbox_data_t *)((uint8_t *)(DT_REG_ADDR(DT_NODELABEL(sram_tx))));
#define MAX_MSG_SIZE (DT_REG_SIZE(DT_NODELABEL(sram_tx)))

int gpio_send(hpf_gpio_data_packet_t *msg)
{
	uint32_t notify_countdown = 1000000U;

	/* Wait for the access to the shared data structure */
	while (!atomic_cas(&tx_data->lock.locked, DATA_LOCK_STATE_READY, DATA_LOCK_STATE_BUSY)) {
		notify_countdown--;
		if (notify_countdown == 0U) {
			if (atomic_get(&tx_data->lock.locked) == DATA_LOCK_STATE_WITH_DATA) {
				/* If the remote side still has unread data, re-notify it. */
				(void)mbox_send_dt(&tx_channel, NULL);
			}
			notify_countdown = 1000000U;
		}
	}

	memcpy((void *)&tx_data->data, (void *)msg, sizeof(hpf_gpio_data_packet_t));
	tx_data->lock.data_size = sizeof(hpf_gpio_data_packet_t);

	/* Inform the consumer that new data is available */
	atomic_set(&tx_data->lock.locked, DATA_LOCK_STATE_WITH_DATA);

	return mbox_send_dt(&tx_channel, NULL);
}

int gpio_hpf_init(const struct device *port)
{
	return 0;
}
