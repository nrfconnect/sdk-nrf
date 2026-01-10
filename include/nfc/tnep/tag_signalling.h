/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_TNEP_TAG_SIGNALLING_H_
#define NFC_TNEP_TAG_SIGNALLING_H_

#include <stdbool.h>

enum tnep_event {
	TNEP_EVENT_DUMMY,
	TNEP_EVENT_MSG_RX_NEW,
	TNEP_EVENT_TAG_SELECTED,
};

/** @brief Raises a receive event.
 *
 * @param event Event to be raised.
 */
void nfc_tnep_tag_signalling_rx_event_raise(enum tnep_event event);

/** @brief Raises a transmit event.
 *
 * @param event Event to be raised.
 */
void nfc_tnep_tag_signalling_tx_event_raise(enum tnep_event event);

/** @brief Checks and clears receive event.
 *
 * @param event Pointer to store the raised event.
 *              The value is valid only if the function returns @c true.
 *
 * @retval true  if a receive event was raised.
 * @retval false if no receive event was raised.
 */
bool nfc_tnep_tag_signalling_rx_event_check_and_clear(enum tnep_event *event);

/** @brief Checks and clears transmit event.
 *
 * @param event Pointer to store the raised event.
 *              The value is valid only if the function returns @c true.
 *
 * @retval true  if a transmit event was raised.
 * @retval false if no transmit event was raised.
 */
bool nfc_tnep_tag_signalling_tx_event_check_and_clear(enum tnep_event *event);

#endif /* NFC_TNEP_TAG_SIGNALLING_H_ */
