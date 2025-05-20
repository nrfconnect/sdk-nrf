/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nfc/ndef/payload_type_common.h>

/* Record Payload Type for Bluetooth Carrier Configuration LE record */
const uint8_t nfc_ndef_le_oob_rec_type_field[] = {
	'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/', 'v', 'n',
	'd', '.', 'b', 'l', 'u', 'e', 't', 'o', 'o', 't', 'h', '.', 'l', 'e',
	'.', 'o', 'o', 'b'
};

/* Handover Select Record type. */
const uint8_t nfc_ndef_ch_hs_rec_type_field[] = {'H', 's'};

/* Handover Request Record type. */
const uint8_t nfc_ndef_ch_hr_rec_type_field[] = {'H', 'r'};

/* Handover Mediation Record type. */
const uint8_t nfc_ndef_ch_hm_rec_type_field[] = {'H', 'm'};

/* Handover Initiate Record type. */
const uint8_t nfc_ndef_ch_hi_rec_type_field[] = {'H', 'i'};

/* Handover Carrier Record. */
const uint8_t nfc_ndef_ch_hc_rec_type_field[] = {'H', 'c'};

/* Handover Alternative Carrier record type. */
const uint8_t nfc_ndef_ch_ac_rec_type_field[] = {'a', 'c'};

/* Handover Collision Resolution record type. */
const uint8_t nfc_ndef_ch_cr_rec_type_field[] = {'c', 'r'};
