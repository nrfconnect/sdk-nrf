/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AUDIO_USB_H_
#define _AUDIO_USB_H_

#include "data_fifo.h"

#if (CONFIG_AUDIO_SOURCE_USB && !CONFIG_AUDIO_SAMPLE_RATE_48000_HZ)
/* Only 48kHz is supported when using USB */
#error USB only supports 48kHz
#endif /* (CONFIG_AUDIO_SOURCE_USB && !CONFIG_AUDIO_SAMPLE_RATE_48000_HZ) */

/**
 * @brief Set fifo buffers to be used by USB module and start sending/receiving data
 *
 * @param fifo_tx_in  Pointer to fifo structure for tx
 * @param fifo_rx_in  Pointer to fifo structure for rx
 *
 * @return 0 if successful, error otherwise
 */
int audio_usb_start(struct data_fifo *fifo_tx_in, struct data_fifo *fifo_rx_in);

/**
 * @brief Stop sending/receiving data
 *
 * @note The USB device will still be running, but all data sent to
 *       it will be discarded
 */
void audio_usb_stop(void);

/**
 * @brief Stop and disable USB device
 *
 * @return 0 if successful, error otherwise
 */
int audio_usb_disable(void);

/**
 * @brief Register and enable USB device
 *
 * @return 0 if successful, error otherwise
 */
int audio_usb_init(void);

#endif /* _AUDIO_USB_H_ */
