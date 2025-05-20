/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nfc/ndef/msg.h>
#include <nfc/ndef/uri_msg.h>
#include <nfc/ndef/le_oob_rec.h>

/** @brief Initialize NFC.
 */
void app_nfc_init(void);

/** @brief Retrieve Out of Band data.
 */
struct bt_le_oob *app_nfc_oob_data_get(void);
