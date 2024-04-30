/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TFM_BUILTIN_KEY_IDS_H__
#define __TFM_BUILTIN_KEY_IDS_H__

#include <nrf.h>

#ifdef NRF_CRACENCORE

#include <cracen_psa_key_ids.h>
#define TFM_BUILTIN_KEY_LOADER_KEY_LOCATION PSA_KEY_LOCATION_CRACEN
enum tfm_builtin_key_id_t {
	TFM_BUILTIN_KEY_ID_HUK = CRACEN_BUILTIN_MKEK_ID,
	TFM_BUILTIN_KEY_ID_IAK = CRACEN_BUILTIN_IDENTITY_KEY_ID,
};

#else
/**
 * \brief The PSA driver location for TF-M builtin keys. Arbitrary within the
 * ranges documented at
 * https://armmbed.github.io/mbed-crypto/html/api/keys/lifetimes.html#c.psa_key_location_t
 */
#define TFM_BUILTIN_KEY_LOADER_KEY_LOCATION ((psa_key_location_t)0x800001)

/**
 * \brief The persistent key identifiers for TF-M builtin keys.
 *
 * The value of TFM_BUILTIN_KEY_ID_MIN (and therefore of the whole range) is
 * completely arbitrary except for being inside the PSA builtin keys range.
 *
 */
enum tfm_builtin_key_id_t {
	TFM_BUILTIN_KEY_ID_MIN = 0x7FFF815Bu,
	TFM_BUILTIN_KEY_ID_HUK,
	TFM_BUILTIN_KEY_ID_IAK,
	TFM_BUILTIN_KEY_ID_PLAT_SPECIFIC_MIN = 0x7FFF816Bu,
	/* Platform specific keys here */
	TFM_BUILTIN_KEY_ID_MAX = 0x7FFF817Bu,
};
#endif /* NRF_CRACEN_BASE */

#endif /* __TFM_BUILTIN_KEY_IDS_H__ */
