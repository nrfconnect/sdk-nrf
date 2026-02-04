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
#include "audio_usb_feedback.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_usb, CONFIG_MODULE_AUDIO_USB_LOG_LEVEL);

#define TERMINAL_ID_HEADSET_OUT	  UAC2_ENTITY_ID(DT_NODELABEL(out_terminal))
#define TERMINAL_ID_HEADPHONES_OUT UAC2_ENTITY_ID(DT_NODELABEL(hp_out_terminal))
#define TERMINAL_ID_HEADSET_IN	  UAC2_ENTITY_ID(DT_NODELABEL(in_terminal))

/* Absolute minimum is 2 TX buffers but add 2 additional buffers to prevent out of memory
 * errors when USB host decides to perform rapid terminal enable/disable cycles.
 * Add one "overflow_bytes" buffer.
 */
#define USB_BLOCKS (4)

/* See udc_buf.h for more information about UDC_BUF_GRANULARITY and UDC_BUF_ALIGN */
K_MEM_SLAB_DEFINE_STATIC(usb_tx_slab, ROUND_UP(USB_BLOCK_SIZE_MULTI_CHAN, UDC_BUF_GRANULARITY),
			 USB_BLOCKS, UDC_BUF_ALIGN);
K_MEM_SLAB_DEFINE_STATIC(usb_rx_slab, ROUND_UP(USB_BLOCK_SIZE_MULTI_CHAN, UDC_BUF_GRANULARITY),
			 USB_BLOCKS, UDC_BUF_ALIGN);

static struct k_msgq *audio_q_tx;
static struct k_msgq *audio_q_rx;

NET_BUF_POOL_FIXED_DEFINE(pool_in, USB_BLOCKS,
			  (USB_BLOCK_SIZE_MULTI_CHAN *
			   (CONFIG_FIFO_FRAME_SPLIT_NUM + USB_SAMPLE_MULTI_CHAN_SIZE)),
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

static struct usb_feedback_ctx feedback_ctx;
static struct usb_feedback_cfg feedback_cfg = {.high_speed = CONFIG_USBD_MAX_SPEED,
					       .sampling_rate_hz = CONFIG_AUDIO_SAMPLE_RATE_HZ,
					       .captures_num = CONFIG_USB_FEEDBACK_CAPTURE_NUM,
					       .low_tide = CONFIG_USB_FEEDBACK_LOW_TIDE,
					       .high_tide = CONFIG_USB_FEEDBACK_HIGH_TIDE};

static void terminal_update_cb(const struct device *dev, uint8_t terminal, bool enabled,
			       bool microframes, void *user_data)
{
	ARG_UNUSED(dev);
	struct usb_feedback_ctx *const fb_ctx = user_data;

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

	usb_feedback_reset(fb_ctx, microframes);
}

static void usb_send_cb(const struct device *dev, void *user_data)
{
	int ret;
	void *pcm_buf;
	struct net_buf *data_out;

	/* Fast path exit - cache combined condition */
	bool tx_ready = (audio_q_tx != NULL && terminal_headset_in_enabled && playing_state &&
			 local_host_in);

	if (unlikely(!tx_ready)) {
		return;
	}

	ret = k_mem_slab_alloc(&usb_tx_slab, &pcm_buf, K_NO_WAIT);
	if (unlikely(ret != 0)) {
		LOG_WRN("Could not allocate pcm_buf, ret: %d", ret);
		return;
	}

	ret = k_msgq_get(audio_q_tx, &data_out, K_NO_WAIT);
	if (unlikely(ret)) {
		/* Reduce logging overhead */
		if (unlikely((++tx_num_underruns % 100) == 1)) {
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

	int ret;
	/* Frame accumulation for 10ms frames */
	static struct net_buf *current_frame, *overflow_frame;
	static uint32_t current_frame_bytes, overflow_frame_bytes;
	struct usb_feedback_ctx *const fb_ctx = user_data;
	uint16_t overflow_bytes;
	uint32_t timestamp;

	timestamp = usb_feedback_capture();

	if (unlikely(buf == NULL || user_data == NULL)) {
		LOG_ERR("NULL pointer");
		return;
	}

	/* Fast exit conditions */
	if (unlikely(size == 0 || !playing_state)) {
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	/* Terminal check */
	if (unlikely(!(terminal_headset_out_enabled || terminal_headphones_out_enabled))) {
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	/* Size validation, can have one less or one more samples in each channel if running with
	 * feedback
	 */
	if (unlikely(!IN_RANGE(size, USB_BLOCK_SIZE_MULTI_CHAN - USB_SAMPLE_MULTI_CHAN_SIZE,
			       USB_BLOCK_SIZE_MULTI_CHAN + USB_SAMPLE_MULTI_CHAN_SIZE))) {
		LOG_WRN("Incorrect buffer size: %d (%u +/- 1)", size, USB_BLOCK_SIZE_MULTI_CHAN);
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	if (current_frame_bytes + size > USB_OUTPUT_MULTI_CHAN_SIZE) {
		overflow_bytes = (current_frame_bytes + size) - USB_OUTPUT_MULTI_CHAN_SIZE;
	} else {
		overflow_bytes = 0;
	}

	ret = usb_feedback_process(fb_ctx, timestamp, overflow_bytes);
	if (ret) {
		LOG_WRN("Failed to generate USB feedback");
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	/* Allocate new frame if we don't have one */
	if (current_frame == NULL) {
		/* Check space availability */
		if (unlikely(k_msgq_num_free_get(audio_q_rx) == 0 || pool_in.avail_count == 0)) {
			goto overrun_cleanup;
		}

		current_frame = net_buf_alloc(&pool_in, K_NO_WAIT);
		if (unlikely(current_frame == NULL)) {
			LOG_WRN("Out of RX buffers for frame");
			goto overrun_cleanup;
		}

		/* Initialize metadata for the first block */
		struct audio_metadata *meta = net_buf_user_data(current_frame);
		*meta = usb_in_meta;

		meta->data_len_us = 0;
		meta->bytes_per_location = 0;
		current_frame_bytes = 0;
	} else if (overflow_bytes) {
		/* Check space availability */
		if (k_msgq_num_free_get(audio_q_rx) == 0 || pool_in.avail_count == 0) {
			goto overrun_cleanup;
		}

		overflow_frame = net_buf_alloc(&pool_in, K_NO_WAIT);
		if (unlikely(overflow_frame == NULL)) {
			LOG_WRN("Out of RX buffers for overflow frame");
			net_buf_unref(current_frame);
			goto overrun_cleanup;
		}

		/* Initialize metadata for the first block */
		struct audio_metadata *meta = net_buf_user_data(overflow_frame);
		*meta = usb_in_meta;

		/* Add data directly to overflow frame */
		size -= overflow_bytes;
		net_buf_add_mem(overflow_frame, (uint8_t *)buf + size, overflow_bytes);

		meta->data_len_us = 0;
		meta->bytes_per_location = 0;
		overflow_frame_bytes = overflow_bytes;
	}

	current_frame_bytes += size;

	/* Add block data directly to current frame */
	net_buf_add_mem(current_frame, buf, size);

	/* Release USB buffer */
	k_mem_slab_free(&usb_rx_slab, buf);

	/* Check if we have a complete frame */
	if (current_frame_bytes == USB_OUTPUT_MULTI_CHAN_SIZE) {
		/* Update metadata */
		struct audio_metadata *meta = net_buf_user_data(current_frame);

		meta->bytes_per_location = current_frame_bytes / audio_metadata_num_loc_get(meta);
		meta->data_len_us = meta->bytes_per_location * (1 / CONFIG_AUDIO_SAMPLE_RATE_HZ);

		/* Put complete frame into RX queue */
		ret = k_msgq_put(audio_q_rx, (void *)&current_frame, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to store complete frame");
			net_buf_unref(current_frame);
		}

		/* Reset for next frame */
		if (overflow_frame != NULL) {
			current_frame = overflow_frame;
			current_frame_bytes = overflow_frame_bytes;
		} else {
			current_frame = NULL;
			current_frame_bytes = 0;
		}
	}

	return;

overrun_cleanup:
	if (unlikely((++rx_num_overruns % 100) == 1)) {
		LOG_WRN("USB RX overrun. Num: %d", rx_num_overruns);
	}

	current_frame_bytes = 0;
	overflow_frame_bytes = 0;

	k_mem_slab_free(&usb_rx_slab, buf);
}

static uint32_t usb_feedback_cb(const struct device *dev, uint8_t terminal, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	struct usb_feedback_ctx *const fb_ctx = user_data;

	return usb_feedback_value(fb_ctx);
}

static struct uac2_ops ops = {
	.sof_cb = usb_send_cb,
	.terminal_update_cb = terminal_update_cb,
	.buf_release_cb = send_buf_release_cb,
	.feedback_cb = usb_feedback_cb,
	.get_recv_buf = get_recv_buf_cb,
	.data_recv_cb = data_recv_cb,
};

static struct usbd_context *audio_usbd;

int audio_usb_start(struct k_msgq *audio_q_tx_in, struct k_msgq *audio_q_rx_in)
{
	int ret;

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

	ret = usb_feedback_start(&feedback_ctx);
	if (ret) {
		LOG_ERR("USB device feedback failed to start");
		return ret;
	}

	return 0;
}

void audio_usb_stop(void)
{
	int ret;

	if (audio_usbd == NULL) {
		LOG_ERR("USB device not initialized");
		return;
	}

	ret = usb_feedback_stop(&feedback_ctx);

	if (ret) {
		LOG_ERR("USB device feedback failed to stop");
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
	}

	return ret;
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

	feedback_cfg.frame_size_us = usb_in_meta.data_len_us * CONFIG_USB_FEEDBACK_CAPTURE_NUM;

	ret = usb_feedback_initialize(&feedback_ctx, &feedback_cfg);
	if (ret) {
		LOG_ERR("Failed to initialize USB feedback");
		return ret;
	}

	usbd_uac2_set_ops(dev, &ops, &feedback_ctx);

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
