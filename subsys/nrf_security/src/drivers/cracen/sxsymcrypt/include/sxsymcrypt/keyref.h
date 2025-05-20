/** Common function definitions for keys.
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KEYREF_HEADER_FILE
#define KEYREF_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

#define CRACEN_INTERNAL_HW_KEY1_ID (1)
#define CRACEN_INTERNAL_HW_KEY2_ID (2)

/** Returns a reference to a key whose key material is in user memory.
 *
 * This function loads the user provided key data and returns an initialized
 * sxkeyref object.
 *
 * The returned object can be passed to any of the sx_aead_create_*() or
 * sx_blkcipher_create_*() functions.
 *
 * @param[in] keysz size of the key to be loaded
 * @param[in] keymaterial key to be loaded with size \p keysz
 * @return sxkeyref initialized object with provided inputs
 *
 * @remark - \p keymaterial buffer should not be changed until the operation
 *           is completed.
 */
struct sxkeyref sx_keyref_load_material(size_t keysz, const char *keymaterial);

#ifdef __cplusplus
}
#endif

#endif
