/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <logging/log.h>
#if !defined(CONFIG_MCUBOOT)
#include <mgmt/mgmt.h>
#endif
#include <mgmt/settings_mgmt.h>
#include <storage/flash_map.h>

LOG_MODULE_REGISTER(mgmt_storage, CONFIG_MGMT_SETTINGS_LOG_LEVEL);

#define STORAGE_MGMT_ID_ERASE 6

int storage_erase(void)
{
	const struct flash_area *fa;


	int rc = flash_area_open(FLASH_AREA_ID(storage), &fa);
	if (rc < 0) {
		LOG_ERR("failed to open flash area");
		return rc;
	}
	rc = flash_area_erase(fa, 0, FLASH_AREA_SIZE(storage));
	if (rc < 0) {
		LOG_ERR("failed to erase flash area");
		return rc;
	}
	flash_area_close(fa);

	return rc;
}

#if !defined(CONFIG_MCUBOOT)
static int storage_erase_handler(struct mgmt_ctxt *ctxt)
{
	CborError cbor_err = 0;

	int rc = storage_erase();


	cbor_err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	cbor_err |= cbor_encode_int(&ctxt->encoder, rc);
	if (cbor_err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return MGMT_ERR_EOK;
}

static const struct mgmt_handler mgmt_storage_handlers[] = {
	[STORAGE_MGMT_ID_ERASE] = {
		.mh_read  = storage_erase_handler,
		.mh_write = storage_erase_handler,
	},
};

static struct mgmt_group storage_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)mgmt_storage_handlers,
	.mg_handlers_count = ARRAY_SIZE(mgmt_storage_handlers),
	.mg_group_id = (MGMT_GROUP_ID_OS),
};

void settings_mgmt_init(void)
{
	mgmt_register_group(&storage_mgmt_group);
}
#endif
