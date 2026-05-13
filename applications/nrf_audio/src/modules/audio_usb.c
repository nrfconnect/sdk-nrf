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

/* USB blocks are 1 ms each, but we split them into 0.5 ms blocks for processing,
 * hence the dividing by two
 */
NET_BUF_POOL_FIXED_DEFINE(pool_in, USB_BLOCKS,
			  (USB_BLOCK_SIZE_MULTI_CHAN * CONFIG_FIFO_FRAME_SPLIT_NUM / 2),
			  sizeof(struct audio_metadata), NULL);

static uint32_t rx_num_overruns;
static bool terminal_headset_out_enabled;
static bool terminal_headphones_out_enabled;
static bool terminal_headset_in_enabled;

static bool playing_state;
static bool local_host_in;
static bool local_host_out;
static bool usb_sof_synchronized;

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

static void half_buf_add(struct net_buf *frame, void *data, uint16_t size,
			 uint32_t *blocks_in_frame)
{
	const uint32_t half_data_len_us = usb_in_meta.data_len_us / 2;
	const uint32_t half_bytes_per_location = usb_in_meta.bytes_per_location / 2;

	net_buf_add_mem(frame, data, size);

	struct audio_metadata *meta = net_buf_user_data(frame);

	meta->data_len_us += half_data_len_us;
	meta->bytes_per_location += half_bytes_per_location;
	(*blocks_in_frame)++;
}

static void data_recv_cb(const struct device *dev, uint8_t terminal, void *buf, uint16_t size,
			 void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	int ret;
	/* Frame accumulation */
	static struct net_buf *frame_current;
	static struct net_buf *frame_spillover;
	static uint32_t blocks_in_frame_current;
	static uint32_t blocks_in_frame_spillover;

	if (unlikely(buf == NULL)) {
		LOG_ERR("Received NULL buffer");
		return;
	}

	/* Fast exit conditions */
	if (unlikely(size == 0 || !playing_state) ||
	    !(terminal_headset_out_enabled || terminal_headphones_out_enabled)) {
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	/* Size validation */
	if (unlikely(size != USB_BLOCK_SIZE_MULTI_CHAN)) {
		LOG_WRN("Incorrect buffer size: %d (%u)", size, USB_BLOCK_SIZE_MULTI_CHAN);
		k_mem_slab_free(&usb_rx_slab, buf);
		return;
	}

	/* Allocate new frame if we don't have one */
	if (frame_current == NULL) {
		if (frame_spillover != NULL) {
			frame_current = frame_spillover;
			blocks_in_frame_current = blocks_in_frame_spillover;
			frame_spillover = NULL;
			blocks_in_frame_spillover = 0;
		} else {
			/* Check space availability */
			if (unlikely(k_msgq_num_free_get(audio_q_rx) == 0 ||
				     pool_in.avail_count == 0)) {
				goto overrun_cleanup;
			}

			frame_current = net_buf_alloc(&pool_in, K_NO_WAIT);
			if (unlikely(frame_current == NULL)) {
				LOG_WRN("Out of RX buffers for frame");
				goto overrun_cleanup;
			}

			/* Initialize metadata for the first block */
			struct audio_metadata *meta = net_buf_user_data(frame_current);

			*meta = usb_in_meta;
			meta->data_len_us = 0;
			meta->bytes_per_location = 0;
			blocks_in_frame_current = 0;
		}
	}

	/* Check if we are about to spill over, if so: allocate spill_over frame */
	if ((blocks_in_frame_current + 2) > CONFIG_FIFO_FRAME_SPLIT_NUM) {
		const size_t half_size = size / 2;

		/* Add half the buffer to current frame */
		half_buf_add(frame_current, buf, half_size, &blocks_in_frame_current);

		if (frame_spillover != NULL) {
			LOG_WRN("Previous spillover frame not consumed, dropping it");
			net_buf_unref(frame_spillover);
			blocks_in_frame_spillover = 0;
		}

		/* Allocate new frame for spill over data since current frame is full. If allocation
		 * fails, drop the spill over data to prevent blocking the USB endpoint, but keep
		 * the current frame to allow it to be sent to the audio system.
		 */
		frame_spillover = net_buf_alloc(&pool_in, K_NO_WAIT);
		if (unlikely(frame_spillover == NULL)) {
			LOG_WRN("Out of RX buffers for spill over frame");
			goto overrun_cleanup;
		}

		/* Initialize metadata for the first block */
		struct audio_metadata *meta_spill_over = net_buf_user_data(frame_spillover);

		*meta_spill_over = usb_in_meta;
		meta_spill_over->data_len_us = 0;
		meta_spill_over->bytes_per_location = 0;
		blocks_in_frame_spillover = 0;

		/* Add half the buffer to spill over frame */
		half_buf_add(frame_spillover, (char *)buf + half_size, half_size,
			     &blocks_in_frame_spillover);
	} else {
		/* Add block data directly to current frame */
		net_buf_add_mem(frame_current, buf, size);
		/* Update metadata */
		struct audio_metadata *meta = net_buf_user_data(frame_current);

		/* Accumulate one full 1 ms USB buffer, which counts as two 0.5 ms blocks. */
		meta->data_len_us += usb_in_meta.data_len_us;
		meta->bytes_per_location += usb_in_meta.bytes_per_location;
		blocks_in_frame_current += 2;
	}

	/* Release USB buffer */
	k_mem_slab_free(&usb_rx_slab, buf);

	/* Check if we have a complete frame */
	if (blocks_in_frame_current >= CONFIG_FIFO_FRAME_SPLIT_NUM) {
		/* Put complete frame into RX queue */
		ret = k_msgq_put(audio_q_rx, (void *)&frame_current, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to store complete frame");
			net_buf_unref(frame_current);
		}

		/* Reset for next frame */
		frame_current = NULL;
		blocks_in_frame_current = 0;
	}

	return;

overrun_cleanup:
	if (unlikely((++rx_num_overruns % 100) == 1)) {
		LOG_WRN("USB RX overrun. Num: %d", rx_num_overruns);
	}

	k_mem_slab_free(&usb_rx_slab, buf);
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
		usb_sof_synchronized = DT_PROP(DT_NODELABEL(uac_aclk), sof_synchronized);
		LOG_INF("USB initialized as bidirectional (headset).");
	} else if (host_out) {
		dev = DEVICE_DT_GET(DT_NODELABEL(uac2_headphones));
		usb_sof_synchronized = DT_PROP(DT_NODELABEL(hp_uac_aclk), sof_synchronized);
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

	LOG_INF("Ready for USB host to send/receive: %s",
		usb_sof_synchronized ? "No Sync (multi-clock) " : "Async (host-adjustable)");

	return 0;
}
