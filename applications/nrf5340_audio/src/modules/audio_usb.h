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

#if (CONFIG_AUDIO_SOURCE_USB && !CONFIG_AUDIO_SAMPLE_RATE_48000_HZ)
/* Only 48kHz is supported when using USB */
#error USB only supports 48kHz
#endif /* (CONFIG_AUDIO_SOURCE_USB && !CONFIG_AUDIO_SAMPLE_RATE_48000_HZ) */

/** Calculate USB block size for stereo audio in bytes. */
#define USB_BLOCK_SIZE_STEREO                                                                      \
	(((CONFIG_AUDIO_SAMPLE_RATE_HZ * CONFIG_AUDIO_BIT_DEPTH_OCTETS) / 1000) * 2)

/**
 * @brief Set pointers to the queues to be used by the USB module and start sending/receiving data.
 *
 * @param queue_tx_in  Pointer to queue structure for tx.
 * @param queue_rx_in  Pointer to queue structure for rx.
 *
 * @return 0 if successful, error otherwise
 */
int audio_usb_start(struct k_msgq *queue_tx_in, struct k_msgq *queue_rx_in);

/**
 * @brief Stop sending/receiving data.
 *
 * @note The USB device will still be running, but all data sent to
 *       it will be discarded.
 */
void audio_usb_stop(void);

/**
 * @brief Stop and disable USB device.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_usb_disable(void);

/**
 * @brief Register and enable USB device.
 *
 * @return 0 if successful, error otherwise.
 */
int audio_usb_init(void);

/**
 * @}
 */

#endif /* _AUDIO_USB_H_ */
