/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <emds/emds.h>

LOG_MODULE_REGISTER(emds, CONFIG_EMDS_LOG_LEVEL);

int emds_init(void)
{
	return 0;
}

int emds_entry_add(struct emds_dynamic_entry *entry)
{
	return 0;
}

int emds_store(void)
{
	return 0;
}

int emds_load(void)
{
	return 0;
}

int emds_clear(void)
{
	return 0;
}

int emds_prepare(void)
{
	return 0;
}

int emds_store_time_get(void)
{
	return 0;
}

int emds_store_size_get(void)
{
	return 0;
}

bool emds_is_busy(void)
{
	return false;
}

int emds_store_size_available(void)
{
	return 0;
}
