/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "tfm_plat_provisioning.h"
#include <stddef.h>

enum tfm_plat_err_t tfm_plat_provisioning_is_required(bool *provisioning_required)
{
	if (provisioning_required == NULL) {
		return TFM_PLAT_ERR_INVALID_INPUT;
	}

	*provisioning_required = false;
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_provisioning_perform(void)
{
	return TFM_PLAT_ERR_SUCCESS;
}

void tfm_plat_provisioning_check_for_dummy_keys(void)
{
}
