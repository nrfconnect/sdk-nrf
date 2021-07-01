/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "event_manager_storage_priv.h"

static struct event_storage_api *storage_api;

int event_store_add(struct event_header *eh)
{
	return storage_api->write(eh);
}

int event_store_remove(struct event_header *eh)
{
	return storage_api->remove(eh);
}

static int event_store_play_all(void)
{
	int err;
	bool first = true;

	while (1) {
		struct event_header *header;
		err = storage_api->read(&header, first);
		if (err == -ENOMEM) {
			return -ENOMEM;
		} else if (err < 0) {
			/* No more data */
			return 0;
		}

		first = false;
		_event_submit(header);
	}
}

int event_store_init(void)
{
	int err;

	storage_api = event_store_backend_get_api();

	err = storage_api->init();
	if (err) {
		return err;
	}

	return event_store_play_all();
}