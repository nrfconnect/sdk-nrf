/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_TNEP_TAG_SIGNALLING_ZEPHYR_H_
#define NFC_TNEP_TAG_SIGNALLING_ZEPHYR_H_

#include <stdint.h>
#include <zephyr/kernel.h>

/** NFC TNEP library event count. */
#define NFC_TNEP_EVENTS_NUMBER 2

/** @brief Initializes TNEP signalling using Zephyr primitives.
 *
 * @param[out] events     Pointer to array of k_poll_event structures.
 * @param[in]  event_cnt  Number of events in the array.
 *                        Must be @ref NFC_TNEP_EVENTS_NUMBER.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, an (negative) error code is returned.
 */
int nfc_tnep_tag_signalling_init(struct k_poll_event *events, uint8_t event_cnt);

#endif /* NFC_TNEP_TAG_SIGNALLING_ZEPHYR_H_ */
