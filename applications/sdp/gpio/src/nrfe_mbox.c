/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/main.h"
#include <zephyr/drivers/mbox.h>

static const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

/**
 * @brief Callback function for when a message is received from the mailbox
 * @param instance Pointer to the mailbox device instance
 * @param channel Unused, but required by the mailbox API
 * @param user_data Pointer to the received message
 * @param msg_data Unused, but required by the mailbox API
 *
 * This function is called when a message is received from the mailbox.
 * It is responsible for handling the received data and logging any errors.
 */
static void mbox_callback(const struct device *instance, uint32_t channel, void *user_data,
			  struct mbox_msg *msg_data)
{
	/* Unused parameters */
	(void)msg_data;

	/* Check for invalid arguments */
	if (user_data == NULL) {
		return;
	}

	nrfe_gpio_mbox_data_t *rx_data = (nrfe_gpio_mbox_data_t *)user_data;

	/* Try and get lock for the shared data structure */
	if (atomic_flag_test_and_set(&rx_data->lock.locked)) {
		/* Return in case lock is not acquired (used by other core)*/
		return;
	}

	nrfe_gpio_data_packet_t *packet = (nrfe_gpio_data_packet_t *)&rx_data->data;

	process_packet(packet);

	/* Clear shared_data.buffer_size (there is no more data available)
	 * This is necessary so that the other core knows that the data has been read
	 */
	rx_data->lock.data_size = 0;

	/* We are finished with the shared data structure, so we can release the lock */
	atomic_flag_clear(&rx_data->lock.locked);
}

/**
 * @brief Initialize the mailbox driver.
 *
 * This function sets up the mailbox receive channel with the callback
 * function specified in the DT node.
 *
 * @return 0 on success, negative error code on failure.
 */
static int mbox_init(void *shared_data)
{
	int ret;

	/* Register the callback function for the mailbox receive channel */
	ret = mbox_register_callback_dt(&rx_channel, mbox_callback, shared_data);
	if (ret < 0) {
		return ret;
	}

	/* Enable the mailbox receive channel */
	return mbox_set_enabled_dt(&rx_channel, true);
}

int backend_init(void)
{
	int ret = 0;

	static nrfe_gpio_mbox_data_t *rx_data =
		(nrfe_gpio_mbox_data_t *)((uint8_t *)(DT_REG_ADDR(DT_NODELABEL(sram_rx))));

	ret = mbox_init((void *)rx_data);
	if (ret < 0) {
		return ret;
	}

	/* clear the buffer locks and their size holders */
	atomic_flag_clear(&rx_data->lock.locked);
	rx_data->lock.data_size = 0;

	return 0;
}
