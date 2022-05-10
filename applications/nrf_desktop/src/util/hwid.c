/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/hwinfo.h>

#include "hwid.h"

void hwid_get(uint8_t *buf, size_t buf_size)
{
	__ASSERT_NO_MSG(buf_size >= HWID_LEN);
	ssize_t ret = hwinfo_get_device_id(buf, HWID_LEN);

	__ASSERT_NO_MSG(ret == HWID_LEN);
	ARG_UNUSED(ret);
}
