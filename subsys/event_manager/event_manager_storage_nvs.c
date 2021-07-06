/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <event_manager.h>
#include <storage/flash_map.h>
#include <fs/nvs.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(event_manager, CONFIG_EVENT_MANAGER_LOG_LEVEL);

static const struct flash_area *events_area;

#define LAST_EVENT_INDEX                                (0)
#define LAST_EVENT_CONSUMED_INDEX                        (1)
#define FIRST_AVAILABLE_INDEX                                (2)

struct event_nvs {
	struct nvs_fs cf_nvs;
	uint16_t last_saved_event;
	uint16_t last_consumed_event;
	uint16_t read_cursor;
	const char *flash_dev_name;
};

static struct event_nvs event_nvs_cfg;

struct event_nvs_entry {
	struct event_type *type;
	size_t event_size;
	uint8_t payload[CONFIG_EVENT_MANAGER_STORAGE_PAYLOAD_MAX_SIZE];
};

static int event_storage_nvs_backend_init(struct event_nvs *nvs)
{
	int rc;
	uint16_t last_saved_event;

	LOG_DBG("Initializing NVS backend");

	rc = nvs_init(&nvs->cf_nvs, nvs->flash_dev_name);
	if (rc) {
		return rc;
	}

	rc = nvs_read(&nvs->cf_nvs, LAST_EVENT_INDEX, &last_saved_event,
		sizeof(last_saved_event));
	if (rc < 0) {
		nvs->last_saved_event = FIRST_AVAILABLE_INDEX;
	} else {
		nvs->last_saved_event = last_saved_event;
	}

	LOG_DBG("Last NVS entry id = %d", nvs->last_saved_event);

	rc = nvs_read(&nvs->cf_nvs, LAST_EVENT_CONSUMED_INDEX,
		&last_saved_event,
		sizeof(last_saved_event));
	if (rc < 0) {
		nvs->last_consumed_event = FIRST_AVAILABLE_INDEX;
	} else {
		nvs->last_consumed_event = last_saved_event;
	}

	LOG_DBG("Last consumed NVS entry id = %d", nvs->last_consumed_event);

	return 0;
}

int event_storage_nvs_init(void)
{
	uint32_t sector_cnt = 1;
	struct flash_sector hw_flash_sector;
	int err;

	err = flash_area_open(FLASH_AREA_ID(events_storage), &events_area);
	if (err) {
		return err;
	}

	err = flash_area_get_sectors(FLASH_AREA_ID(events_storage), &sector_cnt,
		&hw_flash_sector);
	if (err == -ENODEV) {
		return err;
	} else if (err != 0 && err != -ENOMEM) {
		k_panic();
	}

	if (hw_flash_sector.fs_size > UINT16_MAX) {
		return -EDOM;
	}

	event_nvs_cfg.cf_nvs.sector_size = hw_flash_sector.fs_size;
	event_nvs_cfg.cf_nvs.sector_count = 2;
	event_nvs_cfg.cf_nvs.offset = events_area->fa_off;
	event_nvs_cfg.flash_dev_name = events_area->fa_dev_name;

	return event_storage_nvs_backend_init(&event_nvs_cfg);
}

int event_storage_nvs_write(struct event_header *eh)
{
	int err;
	size_t offset = WB_UP(sizeof(struct event_header));
	struct event_nvs_entry entry = {
		.type = (struct event_type *) eh->type_id,
		.event_size = eh->type_id->event_size
	};

	memcpy(entry.payload, ((char *) eh) + offset,
		entry.event_size - offset);
	eh->entry_id = event_nvs_cfg.last_saved_event++;

	LOG_DBG("Writing event %s of size %zd, type = %p,"
		" payload size = %d, offset: %zd",
		log_strdup(eh->type_id->name), entry.event_size,
		entry.type, eh->type_id->event_size - offset, offset);

	err = nvs_write(&event_nvs_cfg.cf_nvs, eh->entry_id,
		&entry, sizeof(entry));
	if (err < 0) {
		LOG_ERR("Could not write event to nvs backend, err %d", err);
		return err;
	}

	err = nvs_write(&event_nvs_cfg.cf_nvs, LAST_EVENT_INDEX,
		&event_nvs_cfg.last_saved_event,
		sizeof(event_nvs_cfg.last_saved_event));
	if (err < 0) {
		LOG_ERR("Could not write event to nvs backend, err %d", err);
		return err;
	}

	return 0;
}

int event_storage_nvs_remove(struct event_header *header)
{
	int err;

	if (header->entry_id < FIRST_AVAILABLE_INDEX) {
		return 0;
	}

	LOG_DBG("Removing event %d from nvs storage", header->entry_id);
	err = nvs_delete(&event_nvs_cfg.cf_nvs, header->entry_id);
	if (err < 0) {
		LOG_ERR("Could not remove event %d from nvs storage", err);
		return err;
	}

	event_nvs_cfg.last_consumed_event = header->entry_id;
	err = nvs_write(&event_nvs_cfg.cf_nvs, LAST_EVENT_CONSUMED_INDEX,
		&event_nvs_cfg.last_consumed_event,
		sizeof(event_nvs_cfg.last_consumed_event));
	if (err < 0) {
		return -EIO;
	}

	return 0;
}

int event_storage_nvs_read(struct event_header **eh, bool from_start)
{
	int err;
	struct event_nvs_entry entry;

	if (from_start || event_nvs_cfg.read_cursor == 0) {
		event_nvs_cfg.read_cursor =
			event_nvs_cfg.last_consumed_event + 1;
	}

	LOG_DBG("Reading event %d from nvs storage", event_nvs_cfg.read_cursor);

	err = nvs_read(&event_nvs_cfg.cf_nvs, event_nvs_cfg.read_cursor,
		&entry, sizeof(entry));
	if (err < 0) {
		LOG_DBG("No more event in nvs storage");
		return err;
	}

	LOG_DBG("Allocating %zu bytes for event %d", entry.event_size,
		event_nvs_cfg.read_cursor);
	event_nvs_cfg.read_cursor++;

	struct event_header *header =
		(struct event_header *) k_malloc(entry.event_size);
	if (!header) {
		LOG_ERR("Could not allocate memory for event");
		return -ENOMEM;
	}

	size_t offset = WB_UP(sizeof(struct event_header));

	LOG_DBG("Event entry saved in NVS: type = %p, size= %zu",
		entry.type, entry.event_size);
	LOG_HEXDUMP_DBG(entry.payload, entry.event_size - offset, "Payload");
	header->type_id = entry.type;
	header->entry_id = event_nvs_cfg.read_cursor - 1;

	/* We need to compute the address location of the
	 * first field of the payload
	 */
	memcpy(((char *) header) + offset, entry.payload,
		entry.event_size - offset);

	*eh = header;

	return 0;
}

static int event_storage_nvs_clear(void)
{
	return nvs_clear(&event_nvs_cfg.cf_nvs);
}

static struct event_storage_api api = {
	.init = event_storage_nvs_init,
	.write = event_storage_nvs_write,
	.read = event_storage_nvs_read,
	.remove = event_storage_nvs_remove,
	.clear = event_storage_nvs_clear,
};

struct event_storage_api *event_store_backend_get_api(void)
{
	return &api;
}
