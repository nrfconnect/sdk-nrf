/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/keyref.h"

struct sxkeyref sx_keyref_load_material(size_t keysz, const char *keymaterial)
{
	struct sxkeyref k;

	k.key = keymaterial;
	k.sz = keysz;
	k.cfg = 0;
	k.prepare_key = 0;
	k.clean_key = 0;
	k.user_data = 0;

	return k;
}
