/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_usb.h"

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_usb, CONFIG_MODULE_AUDIO_USB_LOG_LEVEL);

static struct k_msgq *audio_q_tx;
static struct k_msgq *audio_q_rx;

NET_BUF_POOL_FIXED_DEFINE(pool_in, CONFIG_FIFO_FRAME_SPLIT_NUM, USB_BLOCK_SIZE_STEREO,
			  sizeof(struct audio_metadata), NULL);

static uint32_t rx_num_overruns;
static bool rx_first_data;
static bool tx_first_data;

#if (CONFIG_STREAM_BIDIRECTIONAL)
static uint32_t tx_num_underruns;

static void data_write(const struct device *dev)
{
	int ret;

	if (audio_q_tx == NULL) {
		LOG_WRN("USB TX queue not initialized, dropping data.");
		return;
	}

	struct net_buf *data_out;

	ret = k_msgq_get(audio_q_tx, &data_out, K_NO_WAIT);
	if (ret) {
		tx_num_underruns++;
		if ((tx_num_underruns % 100) == 1) {
			LOG_WRN("USB TX underrun. Num: %d", tx_num_underruns);
		}

		return;
	}

	if (data_out->len != usb_audio_get_in_frame_size(dev)) {
		LOG_WRN("Wrong size write: %d", data_out->len);
		net_buf_unref(data_out);
		return;
	}

	/* The net buf will also be freed in usb_audio_send */
	ret = usb_audio_send(dev, data_out, data_out->len);
	if (ret) {
		LOG_WRN("USB TX failed, ret: %d", ret);
		net_buf_unref(data_out);
		return;
	}

	if (!tx_first_data) {
		LOG_INF("USB TX first data sent.");
		tx_first_data = true;
	}
}
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */

static void data_received(const struct device *dev, struct net_buf *buffer, size_t size)
{
	int ret;
	struct net_buf *audio_block;

	if (audio_q_rx == NULL) {
		/* Throwing away data */
		net_buf_unref(buffer);
		return;
	}

	if (buffer == NULL || size == 0 || buffer->data == NULL) {
		/* This should never happen */
		ERR_CHK(-EINVAL);
	}

	/* Receive data from USB */
	if (size != USB_BLOCK_SIZE_STEREO) {
		LOG_WRN("Wrong length: %d", size);
		net_buf_unref(buffer);
		return;
	}

	/* RX FIFO can fill up due to re-transmissions or disconnect */
	/* Also check the availability in the pool, as the message queue might become
	 * available before the net_buf is unreferenced due to thread execution timing
	 */
	if (k_msgq_num_free_get(audio_q_rx) == 0 || pool_in.avail_count == 0) {
		struct net_buf *stale_usb_data;

		rx_num_overruns++;
		if ((rx_num_overruns % 100) == 1) {
			LOG_WRN("USB RX overrun. Num: %d", rx_num_overruns);
		}

		ret = k_msgq_get(audio_q_rx, (void *)&stale_usb_data, K_NO_WAIT);
		ERR_CHK(ret);

		net_buf_unref(stale_usb_data);
	}

	audio_block = net_buf_alloc(&pool_in, K_NO_WAIT);
	if (audio_block == NULL) {
		LOG_WRN("Out of RX buffers");
		net_buf_unref(buffer);
		return;
	}

	/* Copy data to RX buffer */
	net_buf_add_mem(audio_block, buffer->data, size);

	/* Send USB buffer back to USB stack */
	net_buf_unref(buffer);

	/* Add meta data */
	struct audio_metadata *meta = net_buf_user_data(audio_block);

	meta->data_coding = PCM;
	meta->data_len_us = 1000;
	meta->sample_rate_hz = CONFIG_AUDIO_SAMPLE_RATE_HZ;
	meta->bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
	meta->carried_bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
	meta->locations = BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT;
	meta->bad_data = false;

	/* Put the block into RX queue */
	ret = k_msgq_put(audio_q_rx, (void *)&audio_block, K_NO_WAIT);
	ERR_CHK_MSG(ret, "RX failed to store block");

	if (!rx_first_data) {
		LOG_INF("USB RX first data received.");
		rx_first_data = true;
	}
}

static void feature_update(const struct device *dev, const struct usb_audio_fu_evt *evt)
{
	LOG_DBG("Control selector %d for channel %d updated", evt->cs, evt->channel);
	switch (evt->cs) {
	case USB_AUDIO_FU_MUTE_CONTROL:
		/* Fall through */
	default:
		break;
	}
}

static const struct usb_audio_ops ops = {
	.data_received_cb = data_received,
	.feature_update_cb = feature_update,
#if (CONFIG_STREAM_BIDIRECTIONAL)
	.data_request_cb = data_write,
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */
};

int audio_usb_start(struct k_msgq *audio_q_tx_in, struct k_msgq *audio_q_rx_in)
{
	if (audio_q_tx_in == NULL || audio_q_rx_in == NULL) {
		return -EINVAL;
	}

	audio_q_tx = audio_q_tx_in;
	audio_q_rx = audio_q_rx_in;

	return 0;
}

void audio_usb_stop(void)
{
	rx_first_data = false;
	tx_first_data = false;
	audio_q_tx = NULL;
	audio_q_rx = NULL;
}

int audio_usb_disable(void)
{
	int ret;

	audio_usb_stop();

	ret = usb_disable();
	if (ret) {
		LOG_ERR("Failed to disable USB");
		return ret;
	}

	return 0;
}

int audio_usb_init(void)
{
	int ret;
	const struct device *hs_dev = DEVICE_DT_GET(DT_NODELABEL(hs_0));

	if (!device_is_ready(hs_dev)) {
		LOG_ERR("USB Headset Device not ready");
		return -EIO;
	}

	usb_audio_register(hs_dev, &ops);

	ret = usb_enable(NULL);
	if (ret) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	LOG_INF("Ready for USB host to send/receive.");

	return 0;
}
