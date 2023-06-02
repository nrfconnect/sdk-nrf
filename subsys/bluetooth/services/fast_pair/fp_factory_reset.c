/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/fast_pair.h>

#include "fp_storage.h"

int bt_fast_pair_factory_reset(void)
{
	return fp_storage_factory_reset();
}
