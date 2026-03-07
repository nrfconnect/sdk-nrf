/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_usb_feedback.h"

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <audio_defines.h>

#include "audio_sync_timer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(audio_usb);

/* Full-Speed isochronous feedback is Q10.10 unsigned integer left-justified in
 * the 24-bits so it has Q10.14 format. This sample application puts zeroes to
 * the 4 least significant bits (does not use the bits for extra precision).
 *
 * High-Speed isochronous feedback is Q12.13 unsigned integer aligned in the
 * 32-bits so the binary point is located between second and third byte so it
 * has Q16.16 format. This sample applications puts zeroes to the 3 least
 * significant bits (does not use the bits for extra precision).
 */
#define FEEDBACK_FS_K		10
#define FEEDBACK_HS_K		13
#define FEEDBACK_FS_SHIFT	4
#define FEEDBACK_HS_SHIFT	3
#define FEEDBACK_FS_DIV		1000
#define FEEDBACK_HS_DIV		8000

/* Range of the feedback values for each speed */
#define FEEDBACK_HS_MAX (0x00310000)
#define FEEDBACK_HS_MIN (0x002FC000)
#define FEEDBACK_FS_MAX (0x000C4000)
#define FEEDBACK_FS_MIN (0x000BC000)

int usb_feedback_process(struct usb_feedback_ctx *const fb_ctx, uint32_t ts_us, uint16_t overflow)
{
	if (unlikely(fb_ctx == NULL)) {
		LOG_ERR("Feedback pointer NULL");
		return -EINVAL;
	}

	if (unlikely(!fb_ctx->initialized || !fb_ctx->running)) {
		LOG_ERR("Feedback system uninitialized or not running");
		return -EINVAL;
	}

	/* If reached last capture, test if need to correct */
	if (fb_ctx->captures_cnt == fb_ctx->fb_cfg.captures_num) {
		uint32_t delta = ts_us - fb_ctx->start_ts_us;

		/* Update the correction if outside tide lines */
		if (!IN_RANGE(overflow, fb_ctx->fb_cfg.low_tide, fb_ctx->fb_cfg.high_tide)) {
			fb_ctx->fb_value =
				((delta << fb_ctx->speed_shift) / fb_ctx->fb_cfg.frame_size_us) *
				fb_ctx->samples;

			/* Limit the feedback to +/- one sample.
			 * e.g. For full-speed 48000Hz giving nominal of 48 samples
			 *      the range is 47 (0x0BC000) or 49 (0x0C4000)
			 */
			if (fb_ctx->fb_value <= fb_ctx->fb_min) {
				fb_ctx->fb_value = fb_ctx->fb_min;
			}

			if (fb_ctx->fb_value >= fb_ctx->fb_max) {
				fb_ctx->fb_value = fb_ctx->fb_max;
			}
		}

		fb_ctx->captures_cnt = 0;
		fb_ctx->start_ts_us = ts_us;

	} else if (unlikely(!fb_ctx->captures_cnt)) {
		/* Only do this once after initialize or reset */
		fb_ctx->start_ts_us = ts_us;
	}

	fb_ctx->captures_cnt++;

	return 0;
}

int usb_feedback_start(struct usb_feedback_ctx *const fb_ctx)
{
	if (fb_ctx == NULL) {
		LOG_ERR("Feedback pointer NULL");
		return -EINVAL;
	}

	if (fb_ctx->initialized) {
		fb_ctx->running = true;
	} else {
		LOG_ERR("Feedback system uninitialized");
		return -EINVAL;
	}

	return 0;
}

int usb_feedback_stop(struct usb_feedback_ctx *const fb_ctx)
{
	if (fb_ctx == NULL) {
		LOG_ERR("Feedback pointer NULL");
		return -EINVAL;
	}

	if (fb_ctx->initialized) {
		fb_ctx->running = false;
	} else {
		LOG_ERR("Feedback system uninitialized");
		return -EINVAL;
	}

	return 0;
}

uint32_t usb_feedback_value(struct usb_feedback_ctx *const fb_ctx)
{
	if (unlikely(fb_ctx == NULL)) {
		LOG_ERR("Feedback pointer NULL");
		return -EINVAL;
	}

	if (fb_ctx->running) {
		return fb_ctx->fb_value;
	} else {
		return fb_ctx->nominal;
	}
}

/**
 * @brief Calculate nominal feedback and speed related data within the USB feedback context.
 *
 * @param fb_ctx  Pointer to the USB feedback context to configure.
 *
 */
static void nominal_feedback_values(struct usb_feedback_ctx *const fb_ctx)
{
	/* Put shift and data into correct fixed-point */
	if (fb_ctx->fb_cfg.high_speed) {
		fb_ctx->speed_shift = FEEDBACK_HS_K + FEEDBACK_HS_SHIFT;
		fb_ctx->samples = fb_ctx->fb_cfg.sampling_rate_hz / 8000;
		fb_ctx->nominal = fb_ctx->samples << fb_ctx->speed_shift;
		fb_ctx->fb_max = FEEDBACK_HS_MAX;
		fb_ctx->fb_min = FEEDBACK_HS_MIN;
	} else {
		fb_ctx->speed_shift = FEEDBACK_FS_K + FEEDBACK_FS_SHIFT;
		fb_ctx->samples = fb_ctx->fb_cfg.sampling_rate_hz / 1000;
		fb_ctx->nominal = fb_ctx->samples << fb_ctx->speed_shift;
		fb_ctx->fb_max = FEEDBACK_FS_MAX;
		fb_ctx->fb_min = FEEDBACK_FS_MIN;
	}
}

int usb_feedback_reset(struct usb_feedback_ctx *const fb_ctx, bool microframes)
{
	if (unlikely(fb_ctx == NULL)) {
		LOG_ERR("Feedback pointer NULL");
		return -EINVAL;
	}

	if (fb_ctx->initialized) {
		fb_ctx->fb_cfg.high_speed = microframes;
		nominal_feedback_values(fb_ctx);
		fb_ctx->start_ts_us = 0;
		fb_ctx->captures_cnt = 0;
	} else {
		LOG_ERR("Feedback system uninitialized");
		return -EINVAL;
	}

	return 0;
}

int usb_feedback_initialize(struct usb_feedback_ctx *const fb_ctx,
			    struct usb_feedback_cfg *const fb_cfg)
{
	if (fb_ctx == NULL || fb_cfg == NULL) {
		LOG_ERR("Feedback pointers NULL");
		return -EINVAL;
	}

	if (fb_ctx->initialized) {
		LOG_ERR("Feedback already initialized");
		return -EALREADY;
	}

	memcpy(&fb_ctx->fb_cfg, fb_cfg, sizeof(struct usb_feedback_cfg));

	/* Configure nominal values for fixed-point calculations */
	nominal_feedback_values(fb_ctx);
	fb_ctx->running = false;
	fb_ctx->fb_value = fb_ctx->nominal;
	fb_ctx->start_ts_us = 0;
	fb_ctx->captures_cnt = 0;

	fb_ctx->initialized = true;

	return 0;
}

int usb_feedback_deinitialize(struct usb_feedback_ctx *const fb_ctx)
{
	int ret;

	if (fb_ctx == NULL) {
		LOG_ERR("Feedback pointer NULL");
		return -EINVAL;
	}

	if (fb_ctx->initialized) {
		ret = usb_feedback_stop(fb_ctx);
		if (ret) {
			LOG_ERR("Failed to stop feedback system");
			return ret;
		}

		memset(fb_ctx, 0, sizeof(struct usb_feedback_ctx));
	} else {
		LOG_ERR("USB feedback system in not initialised or running");
		return -EINVAL;
	}

	return 0;
}
