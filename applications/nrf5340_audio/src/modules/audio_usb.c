/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_usb.h"

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/net_buf.h>
#include <audio_defines.h>

#include "macros_common.h"
#include "device_location.h"
#include <pcm_stream_channel_modifier.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_usb, CONFIG_MODULE_AUDIO_USB_LOG_LEVEL);

#define TERMINAL_ID_HEADSET_OUT	  UAC2_ENTITY_ID(DT_NODELABEL(out_terminal))
#define TERMINAL_ID_HEADPHONES_OUT UAC2_ENTITY_ID(DT_NODELABEL(hp_out_terminal))
#define TERMINAL_ID_HEADSET_IN	  UAC2_ENTITY_ID(DT_NODELABEL(in_terminal))

/* Absolute minimum is 2 TX buffers but add 2 additional buffers to prevent out of memory
 * errors when USB host decides to perform rapid terminal enable/disable cycles.
 */
#define USB_BLOCKS 4

/* See udc_buf.h for more information about UDC_BUF_GRANULARITY and UDC_BUF_ALIGN */
K_MEM_SLAB_DEFINE_STATIC(usb_tx_slab, ROUND_UP(USB_BLOCK_SIZE_MULTI_CHAN, UDC_BUF_GRANULARITY),
			 USB_BLOCKS, UDC_BUF_ALIGN);
K_MEM_SLAB_DEFINE_STATIC(usb_rx_slab, ROUND_UP(USB_BLOCK_SIZE_MULTI_CHAN, UDC_BUF_GRANULARITY),
			 USB_BLOCKS, UDC_BUF_ALIGN);

static struct k_msgq *audio_q_tx;
static struct k_msgq *audio_q_rx;

NET_BUF_POOL_FIXED_DEFINE(pool_in, CONFIG_FIFO_FRAME_SPLIT_NUM, USB_BLOCK_SIZE_MULTI_CHAN,
			  sizeof(struct audio_metadata), NULL);

static uint32_t rx_num_overruns;
static bool terminal_headset_out_enabled;
static bool terminal_headphones_out_enabled;
static bool terminal_headset_in_enabled;

static bool playing_state;
static bool local_host_in;
static bool local_host_out;

/* The meta data for the USB and that required for the following audio system. */
struct audio_metadata usb_in_meta = {.data_coding = PCM,
				     .data_len_us = 1000,
				     .sample_rate_hz = CONFIG_AUDIO_SAMPLE_RATE_HZ,
				     .bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS,
				     .carried_bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS,
				     .bytes_per_location = USB_BLOCK_SIZE_MULTI_CHAN / 2,
				     .interleaved = true,
				     .locations = BT_AUDIO_LOCATION_FRONT_LEFT |
						  BT_AUDIO_LOCATION_FRONT_RIGHT,
				     .bad_data = 0};

static uint32_t tx_num_underruns;

static void terminal_update_cb(const struct device *dev, uint8_t terminal, bool enabled,
			       bool microframes, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(microframes);
	ARG_UNUSED(user_data);

	if (terminal == TERMINAL_ID_HEADSET_OUT) {
		terminal_headset_out_enabled = enabled;
	} else if (terminal == TERMINAL_ID_HEADPHONES_OUT) {
		terminal_headphones_out_enabled = enabled;
	} else if (terminal == TERMINAL_ID_HEADSET_IN) {
		terminal_headset_in_enabled = enabled;
	} else {
		LOG_WRN("Unknown terminal ID: %d", terminal);
		return;
	}
}

static void usb_send_cb(const struct device *dev, void *user_data)
{
	int ret;
	void *pcm_buf;
	struct net_buf *data_out;

	if (audio_q_tx == NULL || !terminal_headset_in_enabled || !playing_state ||
	    !local_host_in) {
		return;
	}

	ret = k_mem_slab_alloc(&usb_tx_slab, &pcm_buf, K_NO_WAIT);
	if (ret != 0) {
		LOG_WRN("Could not allocate pcm_buf, ret: %d", ret);
		return;
	}

	ret = k_msgq_get(audio_q_tx, &data_out, K_NO_WAIT);
	if (ret) {
		tx_num_underruns++;
		if ((tx_num_underruns % 100) == 1) {
			LOG_WRN("USB TX underrun. Num: %d", tx_num_underruns);
		}

		k_mem_slab_free(&usb_tx_slab, pcm_buf);

		return;
	}

	struct audio_metadata *usb_out_meta = net_buf_user_data(data_out);
	size_t pcm_size = 0;

	if (audio_metadata_num_loc_get(usb_out_meta) == 1) {
		/* Mono to stereo copy */
		pscm_copy_pad((char *)data_out->data, data_out->len, CONFIG_AUDIO_BIT_DEPTH_BITS,
			      (char *)pcm_buf, &pcm_size);
	} else {
		/* Direct copy for stereo */
		memcpy(pcm_buf, data_out->data, data_out->len);
		pcm_size = data_out->len;
	}

	ret = usbd_uac2_send(dev, TERMINAL_ID_HEADSET_IN, pcm_buf, pcm_size);
	if (ret) {
		LOG_WRN("USB TX failed, ret: %d", ret);
		k_mem_slab_free(&usb_tx_slab, pcm_buf);
	}

	net_buf_unref(data_out);
}

static void send_buf_release_cb(const struct device *dev, uint8_t terminal, void *buf,
				void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (terminal != TERMINAL_ID_HEADSET_IN) {
		LOG_ERR("Buffer release callback for unknown terminal: %d", terminal);
		return;
	}

	k_mem_slab_free(&usb_tx_slab, buf);
}

static void *get_recv_buf_cb(const struct device *dev, uint8_t terminal, uint16_t size,
			     void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(size);
	ARG_UNUSED(user_data);

	int ret;
	void *buf = NULL;

	if (terminal == TERMINAL_ID_HEADSET_OUT || terminal == TERMINAL_ID_HEADPHONES_OUT) {
		ret = k_mem_slab_alloc(&usb_rx_slab, &buf, K_NO_WAIT);
	}

	return buf;
}

static void data_recv_cb(const struct device *dev, uint8_t terminal, void *buf, uint16_t size,
			 void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	int ret;
	struct net_buf *audio_block;

	if ((!terminal_headset_out_enabled && !terminal_headphones_out_enabled) || size == 0 ||
	    !playing_state) {
		if (buf != NULL) {
			k_mem_slab_free(&usb_rx_slab, buf);
		}

		return;
	}

	/* Receive data from USB */
	if (size != USB_BLOCK_SIZE_MULTI_CHAN) {
		LOG_WRN("Incorrect buffer size: %d (%u)", size, USB_BLOCK_SIZE_MULTI_CHAN);
		k_mem_slab_free(&usb_rx_slab, buf);
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
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	/* Copy data to RX buffer */
	net_buf_add_mem(audio_block, buf, size);

	/* Send USB buffer back to USB stack */
	k_mem_slab_free(&usb_rx_slab, buf);

	/* Store USB related metadata */
	struct audio_metadata *meta = net_buf_user_data(audio_block);
	*meta = usb_in_meta;

	/* Put the block into RX queue */
	ret = k_msgq_put(audio_q_rx, (void *)&audio_block, K_NO_WAIT);
	ERR_CHK_MSG(ret, "RX failed to store block");
}

static struct uac2_ops ops = {
	.sof_cb = usb_send_cb,
	.terminal_update_cb = terminal_update_cb,
	.buf_release_cb = send_buf_release_cb,
	.get_recv_buf = get_recv_buf_cb,
	.data_recv_cb = data_recv_cb,
};

static struct usbd_context *audio_usbd;

int audio_usb_start(struct k_msgq *audio_q_tx_in, struct k_msgq *audio_q_rx_in)
{
	if (audio_usbd == NULL) {
		LOG_ERR("USB device not initialized");
		return -ENOTCONN;
	}

	if (audio_q_tx_in == NULL || audio_q_rx_in == NULL) {
		return -EINVAL;
	}

	audio_q_tx = audio_q_tx_in;
	audio_q_rx = audio_q_rx_in;

	playing_state = true;

	return 0;
}

void audio_usb_stop(void)
{
	if (audio_usbd == NULL) {
		LOG_ERR("USB device not initialized");
		return;
	}

	playing_state = false;
}

int audio_usb_disable(void)
{
	int ret;

	if (audio_usbd == NULL) {
		LOG_ERR("USB device not initialized");
		return -ENOTCONN;
	}

	ret = usbd_disable(audio_usbd);
	if (ret) {
		LOG_ERR("Failed to disable USB");
		return ret;
	}

	audio_usb_stop();

	return 0;
}

int audio_usb_init(bool host_in, bool host_out)
{
	int ret;
	const struct device *dev;

	local_host_in = host_in;
	local_host_out = host_out;

	if (host_in && host_out) {
		dev = DEVICE_DT_GET(DT_NODELABEL(uac2_headset));
		LOG_INF("USB initialized as bidirectional (headset).");
	} else if (host_out) {
		dev = DEVICE_DT_GET(DT_NODELABEL(uac2_headphones));
		LOG_INF("USB initialized as unidirectional (headphones only).");
	} else {
		LOG_ERR("USB currently only supports output (host out) or bidirectional (host in "
			"and host out).");
		return -ENOTSUP;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("USB Device not ready");
		return -EIO;
	}

	usbd_uac2_set_ops(dev, &ops, NULL);

	audio_usbd = audio_usbd_init_device(NULL);
	if (audio_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(audio_usbd);
	if (ret) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	LOG_INF("Ready for USB host to send/receive.");

	return 0;
}
