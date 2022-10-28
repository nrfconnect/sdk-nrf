/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_usb.h"

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>

#include "macros_common.h"
#include "data_fifo.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_usb, CONFIG_MODULE_AUDIO_USB_LOG_LEVEL);

#define USB_FRAME_SIZE_STEREO                                                                      \
	(((CONFIG_AUDIO_SAMPLE_RATE_HZ * CONFIG_AUDIO_BIT_DEPTH_OCTETS) / 1000) * 2)

static struct data_fifo *fifo_tx;
static struct data_fifo *fifo_rx;

NET_BUF_POOL_FIXED_DEFINE(pool_out, CONFIG_FIFO_FRAME_SPLIT_NUM, USB_FRAME_SIZE_STEREO, 8,
			  net_buf_destroy);

#if (CONFIG_STREAM_BIDIRECTIONAL)
static void data_write(const struct device *dev)
{
	int ret;

	if (fifo_tx == NULL) {
		return;
	}

	void *data_out;
	size_t data_out_size;
	struct net_buf *buf_out;

	buf_out = net_buf_alloc(&pool_out, K_NO_WAIT);

	ret = data_fifo_pointer_last_filled_get(fifo_tx, &data_out, &data_out_size, K_NO_WAIT);
	if (ret) {
		LOG_WRN("USB TX underrun");
		net_buf_unref(buf_out);
		return;
	}

	memcpy(buf_out->data, data_out, data_out_size);
	data_fifo_block_free(fifo_tx, &data_out);

	if (data_out_size == usb_audio_get_in_frame_size(dev)) {
		ret = usb_audio_send(dev, buf_out, data_out_size);
		if (ret) {
			LOG_WRN("USB TX failed, ret: %d", ret);
			net_buf_unref(buf_out);
		}

	} else {
		LOG_WRN("Wrong size write: %d", data_out_size);
	}
}
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */

static void data_received(const struct device *dev, struct net_buf *buffer, size_t size)
{
	int ret;
	void *data_in;

	if (fifo_rx == NULL) {
		/* Throwing away data */
		net_buf_unref(buffer);
		return;
	}

	if (!buffer || !size) {
		/* This should never happen */
		ERR_CHK(-EINVAL);
	}

	/* Receive data from USB */
	if (size != USB_FRAME_SIZE_STEREO) {
		LOG_WRN("Wrong length: %d", size);
		net_buf_unref(buffer);
		return;
	}

	ret = data_fifo_pointer_first_vacant_get(fifo_rx, &data_in, K_NO_WAIT);

	/* RX FIFO can fill up due to retransmissions or disconnect */
	if (ret == -ENOMEM) {
		void *temp;
		size_t temp_size;

		LOG_WRN("USB RX overrun");

		ret = data_fifo_pointer_last_filled_get(fifo_rx, &temp, &temp_size, K_NO_WAIT);
		ERR_CHK(ret);

		data_fifo_block_free(fifo_rx, &temp);

		ret = data_fifo_pointer_first_vacant_get(fifo_rx, &data_in, K_NO_WAIT);
	}

	ERR_CHK_MSG(ret, "RX failed to get block");

	memcpy(data_in, buffer->data, size);

	ret = data_fifo_block_lock(fifo_rx, &data_in, size);
	ERR_CHK_MSG(ret, "Failed to lock block");

	net_buf_unref(buffer);
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

	return 0;
}
