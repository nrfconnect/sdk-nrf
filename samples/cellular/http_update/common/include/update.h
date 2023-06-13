/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UPDATE_H__
#define UPDATE_H__

#define TLS_SEC_TAG 42

#ifndef CONFIG_USE_HTTPS
#define SEC_TAG (-1)
#else
#define SEC_TAG (TLS_SEC_TAG)
#endif

/**
 * @typedef update_start_cb
 *
 * @brief Signature for callback invoked when button is pressed.
 */
typedef void (*update_start_cb)(void);

/**
 * @brief Initialization struct.
 */
struct update_sample_init_params {
	/* Callback invoked when the button is pressed */
	update_start_cb update_start;

	/* Number of LEDs to enable. Must be 0, 1 or 2. */
	int num_leds;

	/* Update file to download */
	const char *filename;
};

/** Initialize common update sample library.
 *
 * This will initialize one button with the given callback, and enable
 * one or two LEDs based on the result of `params->two_leds`
 *
 * @param[in]  params   Pointer to initiazilation parameters.
 *
 * @return non-negative on success, negative errno code on fail
 */
int update_sample_init(struct update_sample_init_params *params);

/** Begin downloading specified FOTA update
 */
void update_sample_start(void);

/** Notify the library that the update has been stopped.
 *
 * This will re-enable the button. Making it possible to
 * restart or continue the update.
 */
void update_sample_stop(void);

/** Notify the library that the update has been completed.
 *
 * This will send the modem to power off mode, while waiting
 * for a reset.
 */
void update_sample_done(void);

#endif /* UPDATE_H__ */
