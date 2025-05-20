/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TFM_BUILTIN_KEY_LOADER_IDS_H
#define TFM_BUILTIN_KEY_LOADER_IDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* HUK and IAK are max 32-byte on nordic platforms */
#define TFM_BUILTIN_MAX_KEY_LEN 32

enum psa_drv_slot_number_t {
	TFM_BUILTIN_KEY_SLOT_HUK = 0,
	TFM_BUILTIN_KEY_SLOT_IAK,
	TFM_BUILTIN_KEY_SLOT_MAX,
};

#ifdef __cplusplus
}
#endif

#endif /* TFM_BUILTIN_KEY_LOADER_IDS_H */
