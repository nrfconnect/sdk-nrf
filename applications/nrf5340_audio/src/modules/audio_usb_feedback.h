/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_app_usb Audio USB
 * @{
 * @brief Audio USB feedback interface API for Audio applications.
 *
 * This module provides USB audio feedback functionality, enabling
 * rate controlled audio input/output through USB connections.
 */

#ifndef _AUDIO_USB_FEEDBACK_H_
#define _AUDIO_USB_FEEDBACK_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include "audio_sync_timer.h"

/* @brief Feedback configuration structure */
struct usb_feedback_cfg {
	/* Feedback speed, full-speed = false, high-speed = true*/
	bool high_speed;

	/* Audio sampling frequency in Hz */
	uint16_t sampling_rate_hz;

	/* Size of the audio frame to receive on the USB in us */
	uint32_t frame_size_us;

	/* Number of capture between possible feedback calculation */
	uint16_t captures_num;

	/* If the overflow falls below this mark the feedback correction is calculated */
	uint32_t low_tide;

	/* If the overflow goes above this mark the feedback correction is calculated */
	uint32_t high_tide;
};

/* @brief Private feedback handle */
struct usb_feedback_ctx {
	/* Flag to indicate the feedback system has been initialized.
	 * Where true = initialized and false = uninitialized
	 */
	bool initialized;

	/* Flag to signal if the feedback process is running.
	 * Where true = running and false = stopped
	 */
	bool running;

	/* Configuration of the feedback system */
	struct usb_feedback_cfg fb_cfg;

	/* Number of samples per USB period */
	uint16_t samples;

	/* Nominal feedback value */
	uint32_t nominal;

	/* The calculated sink sample rate, Hz */
	uint32_t fb_value;

	/* Speed scale factor (left align and zero bits) */
	uint8_t speed_shift;

	/* Value of the counter at start of feedback cycle */
	uint32_t start_ts_us;

	/* Running count of captures*/
	uint16_t captures_cnt;
};

/**
 * @brief Read the synchronization counter.
 *
 * @return Value of sync counter at call
 */
static inline int32_t usb_feedback_capture(void)
{
	return audio_sync_timer_capture();
}

/**
 * @brief Calculate the feedback for the USB.
 *
 * @param fb_ctx      Pointer to feedback context.
 * @param ts_ns       Timestamp of the last received USB block in ns.
 * @param overflow    The number of bytes that haver overflowed the encoder buffer by.
 *
 * @return 0 if successful, error otherwise
 */
int usb_feedback_process(struct usb_feedback_ctx *const ctx, uint32_t ts_ns, uint16_t overflow);

/**
 * @brief Read the USB feedback module's last calculated value.
 *
 * @param fb_ctx      Pointer to feedback context.
 *
 * @return The feedback value in either Q10.14 (full-speed) or Q16.16 (high-speed)
 */
uint32_t usb_feedback_value(struct usb_feedback_ctx *const fb_ctx);

/**
 * @brief Start the USB feedback module.
 *
 * @param fb_ctx       Pointer to feedback context.
 *
 * @return 0 if successful, error otherwise
 */
int usb_feedback_start(struct usb_feedback_ctx *const fb_ctx);

/**
 * @brief Stop the USB feedback module.
 *
 * @param fb_ctx      Pointer to feedback context.
 *
 * @return 0 if successful, error otherwise
 */
int usb_feedback_stop(struct usb_feedback_ctx *const fb_ctx);

/**
 * @brief Reset the USB feedback module.
 *
 * @param fb_ctx      Pointer to feedback context.
 * @param high_speed  Flag to indicate if high-speed has been selected.
 *                    i.e. High-speed = true Full-speed = false
 *
 * @return 0 if successful, error otherwise
 */
int usb_feedback_reset(struct usb_feedback_ctx *const fb_ctx, bool high_speed);

/**
 * @brief Uninitialize the USB feedback module.
 *
 * @param fb_ctx      Pointer to feedback context.
 *
 * @return 0 if successful, error otherwise
 */
int usb_feedback_deinitialize(struct usb_feedback_ctx *const fb_ctx);

/**
 * @brief Initialize the USB feedback module.
 *
 * @param fb_ctx      Pointer to feedback context.
 * @param fb_cfg      Pointer to the initial configuration of the feedback module.
 *
 * @return 0 if successful, error otherwise
 */
int usb_feedback_initialize(struct usb_feedback_ctx *const fb_ctx,
			    struct usb_feedback_cfg *const fb_cfg);

#endif /* _AUDIO_USB_FEEDBACK_H_ */
