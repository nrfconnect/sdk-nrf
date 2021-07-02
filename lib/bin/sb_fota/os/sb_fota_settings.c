/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "sb_fota_settings.h"

int sb_fota_cloud_sec_tag()
{
	return CONFIG_SB_FOTA_TLS_SECURITY_TAG;
}

int sb_fota_jwt_sec_tag()
{
	return CONFIG_SB_FOTA_JWT_SECURITY_TAG;
}
