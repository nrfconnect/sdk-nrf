/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_POLLER_H_
#define NFC_POLLER_H_

#include <zephyr/kernel.h>
#include <nfc/ndef/msg.h>

#ifdef _cplusplus
extern "C" {
#endif

/**@brief Non_TNEP message received callback.
 *
 * This callback is used when the NFC Tag which not supported TNEP
 * was read.
 *
 * @param[in] ndef_msg Pointer to received non-TNEP NDEF message.
 */
typedef void (*ndef_recv_cb_t)(struct nfc_ndef_msg_desc *ndef_msg);

/**@brief Initialize the NFC Poller Device.
 *
 * @param[in] events NFC Poller events. This device needs a least two free
 *                   events.
 * @param[in] ndef_recv_cb Non-TNEP message received callback.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_poller_init(struct k_poll_event *events, ndef_recv_cb_t ndef_recv_cb);

/**@brief Proccess the NFC Poller Device.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_poller_process(void);

/**@brief Check the NFC Device field status.
 *
 * @retval True If field is on.
 *         Otherwise, a false is returned.
 */
bool nfc_poller_field_active(void);

#ifdef _cplusplus
}
#endif

#endif /* NFC_POLLER_H_ */
