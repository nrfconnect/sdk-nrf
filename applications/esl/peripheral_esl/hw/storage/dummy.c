/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include "esl.h"

LOG_MODULE_DECLARE(peripheral_esl);
int write_img_to_storage(uint8_t img_idx, size_t len, off_t offset)
{
	LOG_DBG("Dummy storage");
	return 0;
}

int read_img_from_storage(uint8_t img_idx, void *data, size_t len, off_t offset)
{
	LOG_DBG("Dummy storage");
	return 0;
}

size_t read_img_size_from_storage(uint8_t img_idx)
{
	LOG_DBG("Dummy storage");
	return 0;
}

int delete_imgs_from_storage(void)
{
	LOG_DBG("Dummy storage");
	return 0;
}

int ots_storage_init(void)
{
	LOG_DBG("Dummy storage");
	return 0;
}
