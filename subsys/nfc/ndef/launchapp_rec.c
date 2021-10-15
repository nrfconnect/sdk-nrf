/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nfc/ndef/launchapp_rec.h>

/* Record Payload Type for NFC NDEF Android Application Record */
const uint8_t ndef_android_launchapp_rec_type[] = {
	'a', 'n', 'd', 'r', 'o', 'i', 'd', '.', 'c', 'o', 'm', ':', 'p', 'k', 'g'
};
