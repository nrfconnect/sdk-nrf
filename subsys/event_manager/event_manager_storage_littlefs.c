/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "event_manager_storage_priv.h"

int event_storage_littlefs_init(void)
{
	/* TODO */
	return -ENOTSUP;
}

int event_storage_littlefs_write(struct event_header *eh)
{
	/* TODO */
	return -ENOTSUP;
}

int event_storage_littlefs_read(struct event_header **eh, bool from_start)
{
	/* TODO */
	return -ENOTSUP;
}

int event_storage_littlefs_remove(struct event_header *eh)
{
	/* TODO */
	return -ENOTSUP;
}

int event_storage_littlefs_clear(void)
{
	/* TODO */
	return -ENOTSUP;
}

static struct event_storage_api api = {
	.init = event_storage_littlefs_init,
	.write = event_storage_littlefs_write,
	.read = event_storage_littlefs_read,
	.remove = event_storage_littlefs_remove,
	.clear = event_storage_littlefs_clear
};

struct event_storage_api *event_store_backend_get_api(void)
{
	return &api;
}
