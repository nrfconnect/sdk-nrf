/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAMPLES_DERIVE_KEY_H__
#define SAMPLES_DERIVE_KEY_H__

psa_status_t derive_key(psa_key_attributes_t *attributes, uint8_t *key_label,
			uint32_t label_size, psa_key_id_t *key_id_out);

#endif /* SAMPLES_DERIVE_KEY_H*/
