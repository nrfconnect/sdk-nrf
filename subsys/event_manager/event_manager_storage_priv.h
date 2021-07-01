/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EVENT_MANAGER_STORAGE_PRIV_H
#define _EVENT_MANAGER_STORAGE_PRIV_H

#include <event_manager.h>
#include <device.h>

typedef int (*event_storage_init_t)(void);

typedef int (*event_storage_write_t)(struct event_header *eh);

typedef int (*event_storage_read_t)(struct event_header **eh, bool from_start);

typedef int (*event_storage_remove_t)(struct event_header *eh);

struct event_storage_api {
	event_storage_init_t init;
	event_storage_write_t write;
	event_storage_read_t read;
	event_storage_remove_t remove;
};

int event_store_add(struct event_header *eh);

int event_store_remove(struct event_header *eh);

int event_store_init(void);

struct event_storage_api *event_store_backend_get_api(void);

#endif /* _EVENT_MANAGER_STORAGE_PRIV_H */