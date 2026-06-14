/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_app_usb Audio USB
 * @{
 * @brief Audio USB interface API for Audio applications.
 *
 * This module provides USB audio functionality, enabling audio
 * input/output through USB connections.
 */

#ifndef _AUDIO_USB_H_
#define _AUDIO_USB_H_

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>

#if (CONFIG_AUDIO_SOURCE_USB && !CONFIG_AUDIO_SAMPLE_RATE_48000_HZ &&                              \
	MAX(CONFIG_AUDIO_INPUT_CHANNELS, CONFIG_AUDIO_OUTPUT_CHANNELS) != 2)
/* Only 48kHz stereo is supported when using USB */
#error USB only supports 48kHz stereo
#endif /* (CONFIG_AUDIO_SOURCE_USB && !CONFIG_AUDIO_SAMPLE_RATE_48000_HZ) */

#define USB_1MS_BLOCKS_NUM_MAX                                                                     \
	((CONFIG_AUDIO_FRAME_DURATION_US / USEC_PER_MSEC) +                                        \
	 (CONFIG_AUDIO_FRAME_DURATION_US % USEC_PER_MSEC ? 1 : 0))
#define USB_BLOCK_1MS_MONO_SIZE                                                                    \
	(CONFIG_AUDIO_SAMPLE_RATE_HZ * CONFIG_AUDIO_BIT_DEPTH_OCTETS / MSEC_PER_SEC)
#define USB_BLOCK_MULTI_CHAN_1MS_SIZE                                                              \
	(USB_BLOCK_1MS_MONO_SIZE * MAX(CONFIG_AUDIO_INPUT_CHANNELS, CONFIG_AUDIO_OUTPUT_CHANNELS))

struct usbd_context *audio_usbd_init_device(usbd_msg_cb_t msg_cb);

/**
 * @brief Get the state of the USB devices headphones out.
 *
 * @return true: USB headphones out enabled.
 *         false: USB headphones out disabled.
 */
bool audio_usb_headphones_out_enabled(void);

/**
 * @brief Get the state of the USB devices headset out.
 *
 * @return true: USB headset out enabled.
 *         false: USB headset out disabled.
 */
bool audio_usb_headset_out_enabled(void);

/**
 * @brief Get the state of the USB devices headset in.
 *
 * @return true: USB headset in enabled.
 *         false: USB headset in disabled.
 */
bool audio_usb_headset_in_enabled(void);

/**
 * @brief Set pointers to the queues to be used by the USB module and start sending/receiving data.
 *
 * @param audio_q_out_ptr  Pointer to queue structure for out.
 * @param audio_q_in_ptr   Pointer to queue structure for in.
 *
 * @return 0 if successful, error otherwise
 */
int audio_usb_start(struct k_msgq *audio_q_out_ptr, struct k_msgq *audio_q_in_ptr);

/**
 * @brief Stop sending/receiving data.
 *
 * @note The USB device will still be running, but all data sent to
 *       it will be discarded.
 *
 * @return 0 if successful, error otherwise
 */
int audio_usb_stop(void);

/**
 * @brief Stop and disable USB device.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_usb_disable(void);

/**
 * @brief Register and enable USB device.
 *
 * @param	host_in		If true, initializes USB as input (host in),
 * @param	host_out	If true, initializes USB as output (host out).
 *
 * @note If both @p host_in and @p host_out are true, USB is initialized as bidirectional
 * (headset).
 *
 * @return 0 if successful, error otherwise.
 */
int audio_usb_init(bool host_in, bool host_out);

/**
 * @}
 */

#endif /* _AUDIO_USB_H_ */
