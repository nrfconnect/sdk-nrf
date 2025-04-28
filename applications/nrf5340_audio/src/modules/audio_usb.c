/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_usb.h"

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>
#include <data_fifo.h>
#include <audio_defines.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_usb, CONFIG_MODULE_AUDIO_USB_LOG_LEVEL);

#define USB_FRAME_SIZE_STEREO                                                                      \
	(((CONFIG_AUDIO_SAMPLE_RATE_HZ * CONFIG_AUDIO_BIT_DEPTH_OCTETS) / 1000) * 2)

static struct data_fifo *fifo_tx;
static struct data_fifo *fifo_rx;

NET_BUF_POOL_FIXED_DEFINE(pool_out, CONFIG_FIFO_FRAME_SPLIT_NUM, USB_FRAME_SIZE_STEREO, 8,
			  net_buf_destroy);
NET_BUF_POOL_FIXED_DEFINE(pool_in, CONFIG_FIFO_FRAME_SPLIT_NUM, USB_FRAME_SIZE_STEREO, 8,
			  net_buf_destroy);

static uint32_t rx_num_overruns;
static bool rx_first_data;
static bool tx_first_data;

#if (CONFIG_STREAM_BIDIRECTIONAL)
static uint32_t tx_num_underruns;

static void data_write(const struct device *dev)
{
	int ret;

	if (fifo_tx == NULL) {
		return;
	}

	void *data_out;
	size_t data_out_size;
	struct net_buf *audio_buf_out;

	audio_buf_out = net_buf_alloc(&pool_out, K_NO_WAIT);

	ret = data_fifo_pointer_last_filled_get(fifo_tx, &data_out, &data_out_size, K_NO_WAIT);
	if (ret) {
		tx_num_underruns++;
		if ((tx_num_underruns % 100) == 1) {
			LOG_WRN("USB TX underrun. Num: %d", tx_num_underruns);
		}
		net_buf_unref(audio_buf_out);

		return;
	}

	memcpy(audio_buf_out->data, data_out, data_out_size);
	data_fifo_block_free(fifo_tx, data_out);

	if (data_out_size == usb_audio_get_in_frame_size(dev)) {
		ret = usb_audio_send(dev, audio_buf_out, data_out_size);
		if (ret) {
			LOG_WRN("USB TX failed, ret: %d", ret);
			net_buf_unref(audio_buf_out);
			return;
		}

	} else {
		LOG_WRN("Wrong size write: %d", data_out_size);
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
	struct audio_data *audio_block = NULL;
	struct net_buf *audio_buf_in;

	if (fifo_rx == NULL) {
		/* Throwing away data */
		net_buf_unref(buffer);
		return;
	}

	if (buffer == NULL || size == 0 || buffer->data == NULL) {
		/* This should never happen */
		ERR_CHK(-EINVAL);
	}

	/* Receive data from USB */
	if (size != USB_FRAME_SIZE_STEREO) {
		LOG_WRN("Wrong length: %d", size);
		net_buf_unref(buffer);
		return;
	}

	audio_buf_in = net_buf_alloc(&pool_in, K_NO_WAIT);
	if (audio_buf_in == NULL) {
		LOG_WRN("Out of RX buffers");
		net_buf_unref(buffer);
		return;
	}

	/* Copy data to RX FIFO */
	net_buf_add_mem(audio_buf_in, buffer->data, size);
	net_buf_unref(buffer);

	ret = data_fifo_pointer_first_vacant_get(fifo_rx, (void *)&audio_block, K_NO_WAIT);

	/* RX FIFO can fill up due to re-transmissions or disconnect */
	if (ret == -ENOMEM) {
		struct audio_data *stale_usb_data;
		size_t stale_size;

		rx_num_overruns++;
		if ((rx_num_overruns % 100) == 1) {
			LOG_WRN("USB RX overrun. Num: %d", rx_num_overruns);
		}

		ret = data_fifo_pointer_last_filled_get(fifo_rx, (void *)&stale_usb_data,
							&stale_size, K_NO_WAIT);
		ERR_CHK(ret);

		struct net_buf *buf = stale_usb_data->data;

		net_buf_unref(buf);
		data_fifo_block_free(fifo_rx, stale_usb_data);

		ret = data_fifo_pointer_first_vacant_get(fifo_rx, (void *)&audio_block, K_NO_WAIT);
	}

	ERR_CHK_MSG(ret, "RX failed to get block");

	audio_block->data = audio_buf_in;
	audio_block->data_size = size;

	/* Add meta data */
	audio_block->meta.data_coding = PCM;
	audio_block->meta.data_len_us = 1000;
	audio_block->meta.sample_rate_hz = CONFIG_AUDIO_SAMPLE_RATE_HZ;
	audio_block->meta.bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
	audio_block->meta.carried_bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
	audio_block->meta.locations = BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT;
	audio_block->meta.bad_data = false;

	ret = data_fifo_block_lock(fifo_rx, (void *)&audio_block, sizeof(struct audio_data));
	ERR_CHK_MSG(ret, "Failed to lock block");

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

int audio_usb_start(struct data_fifo *fifo_tx_in, struct data_fifo *fifo_rx_in)
{
	if (fifo_tx_in == NULL || fifo_rx_in == NULL) {
		return -EINVAL;
	}

	fifo_tx = fifo_tx_in;
	fifo_rx = fifo_rx_in;

	return 0;
}

void audio_usb_stop(void)
{
	rx_first_data = false;
	tx_first_data = false;
	fifo_tx = NULL;
	fifo_rx = NULL;
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
