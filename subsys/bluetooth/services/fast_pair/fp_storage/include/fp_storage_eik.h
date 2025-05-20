/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_EIK_H_
#define _FP_STORAGE_EIK_H_

#include <sys/types.h>

/**
 * @defgroup fp_storage_eik Fast Pair Ephemeral Identity Key (EIK) storage for the FMDN extension
 * @brief Internal API of Fast Pair Ephemeral Identity Key (EIK) storage for the FMDN extension
 *
 * The module must be initialized before using API functions.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Length in bytes of the Ephemeral Identity Key (EIK). */
#define FP_STORAGE_EIK_LEN 32

/** Save the new the Ephemeral Identity Key (EIK) or update the existing one.
 *
 * @param[in] eik Ephemeral Identity Key (32 bytes) to be saved.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_eik_save(const uint8_t *eik);

/** Delete the Ephemeral Identity Key (EIK) from the device storage.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_eik_delete(void);

/** Check if the Ephemeral Identity Key (EIK) is available in the device storage.
 *
 * @return 1 If the EIK is available in the device storage (provisioned state).
 *         0 If the EIK is not available in the device storage (unprovisioned state).
 *         Otherwise, a negative value is returned which indicates an error.
 */
int fp_storage_eik_is_provisioned(void);

/** Get the Ephemeral Identity Key (EIK) value.
 *
 * @param[out] eik Ephemeral Identity Key value (32 bytes).
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_eik_get(uint8_t *eik);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_EIK_H_ */
