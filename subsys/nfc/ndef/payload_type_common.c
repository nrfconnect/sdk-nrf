/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nfc/ndef/payload_type_common.h>

/* Record Payload Type for Bluetooth Carrier Configuration LE record */
const u8_t nfc_ndef_le_oob_rec_type_field[] = {
	'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/', 'v', 'n',
	'd', '.', 'b', 'l', 'u', 'e', 't', 'o', 'o', 't', 'h', '.', 'l', 'e',
	'.', 'o', 'o', 'b'
};
