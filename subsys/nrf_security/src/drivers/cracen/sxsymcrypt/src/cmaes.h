/** Common definitions for Cryptomaster AES modules
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CMAES_HEADER_FILE
#define CMAES_HEADER_FILE

#define CM_CFG_DECRYPT 1
#define CM_CFG_ENCRYPT 0

static inline uint32_t sx_aes_keysz(size_t keysz)
{
	if (keysz == 16) {
		return (0u << 2);
	} else if (keysz == 24) {
		return (1u << 3);
	} else if (keysz == 32) {
		return (1u << 2);
	} else {
		return ~0;
	}
}

#endif
