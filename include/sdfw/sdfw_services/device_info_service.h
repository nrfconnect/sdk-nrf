/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEVICE_INFO_SERVICE_H__
#define DEVICE_INFO_SERVICE_H__

#include <stddef.h>
#include <stdint.h>

static inline int ssf_device_info_get_uuid(uint8_t *uuid_buff)
{
	(void)uuid_buff;

	/* Dummy function kept for portability. */

	return -1;
}

#endif /* DEVICE_INFO_SERVICE_H__ */
