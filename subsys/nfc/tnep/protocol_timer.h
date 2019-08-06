/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_TNEP_TIMER_H_
#define NFC_TNEP_TIMER_H_

#include <stddef.h>
#include <zephyr.h>

/** @brief sDelay of first timer start */
#define NFC_TNEP_K_TIMER_START_DELAY 0

/** Timer signal to indicate timeout events. */
extern struct k_poll_signal nfc_tnep_sig_timer;

/** @brief Timer signal values */
enum nfc_tnep_timer_signal {
	/* Never used, programming convenience only */
	NFC_TNEP_TMER_SIGNAL_TIMER_DUMMY,
	/* Timer period signal */
	NFC_TNEP_TMER_SIGNAL_TIMER_PERIOD,
	/* Timer stop signal */
	NFC_TNEP_TMER_SIGNAL_TIMER_STOP,
};

/**
 * @brief Timer initialization function.
 * It is necessary to call this function before timer start.
 *
 * @param period_ms Timer period in ms.
 * @param counts Number of periods after which timer stop.
 */
void nfc_tnep_timer_init(size_t period_ms, u8_t counts);

/**
 * @brief Starts timer.
 *
 * The @ref timer_signal is signaled after every every period_ms,(given in
 * nfc_tnep_timer_config) with value TNEP_SIGNAL_TIMER_PERIOD.
 * The @ref timer_signal is signaled last time after N times of period_ms,
 * (N is given by count parameters in nfc_tnep_timer_config). This last signal
 * has value value TNEP_SIGNAL_TIMER_STOP.
 */
void nfc_tnep_timer_start(void);

/**
 * @brief Stop timer.
 *
 * Do nothing if timer already stopped.
 * Stop timer counting without any signal.
 */
void nfc_tnep_timer_stop(void);

#endif /* NFC_TNEP_TIMER_H_ */
