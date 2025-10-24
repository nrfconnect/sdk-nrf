/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/keyref.h"

struct sxkeyref sx_keyref_load_material(size_t keysz, const uint8_t *keymaterial)
{
	struct sxkeyref keyref;

	keyref.key = keymaterial;
	keyref.sz = keysz;
	keyref.cfg = 0;
	keyref.prepare_key = 0;
	keyref.clean_key = 0;
	keyref.user_data = 0;

	return keyref;
}
