/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <shell/shell.h>

#include "zigbee_cli_ctx_mgr.h"


/* The table to store context manager entries. */
static struct ctx_entry zb_cli_ctx[CONFIG_ZIGBEE_SHELL_CTX_MGR_ENTRIES_NBR];

/* Mutex to protect context manager from being simultaneously accessed
 * by multiple threads.
 */
static K_MUTEX_DEFINE(ctx_mgr_mutex);

void ctx_mgr_delete_entry(struct ctx_entry *ctx_entry)
{
	k_mutex_lock(&ctx_mgr_mutex, K_FOREVER);

	memset(ctx_entry, 0, sizeof(struct ctx_entry));

	k_mutex_unlock(&ctx_mgr_mutex);
}

struct ctx_entry *ctx_mgr_new_entry(enum ctx_entry_type type)
{
	uint8_t i;

	k_mutex_lock(&ctx_mgr_mutex, K_FOREVER);

	for (i = 0; i < CONFIG_ZIGBEE_SHELL_CTX_MGR_ENTRIES_NBR; i++) {
		if (zb_cli_ctx[i].taken == false) {
			/* Mark the entry as used and assign an entry type. */
			zb_cli_ctx[i].taken = true;
			zb_cli_ctx[i].type = type;
			k_mutex_unlock(&ctx_mgr_mutex);

			return (zb_cli_ctx + i);
		}
	}
	k_mutex_unlock(&ctx_mgr_mutex);

	return NULL;
}

struct ctx_entry *ctx_mgr_find_ctx_entry(uint8_t id, enum ctx_entry_type type)
{
	uint8_t i;

	k_mutex_lock(&ctx_mgr_mutex, K_FOREVER);

	/* Iterate over the context entries to find a matching entry. */
	for (i = 0; i < CONFIG_ZIGBEE_SHELL_CTX_MGR_ENTRIES_NBR; i++) {
		if ((zb_cli_ctx[i].taken == true) &&
		    (zb_cli_ctx[i].type == type) && (zb_cli_ctx[i].id == id)) {
			k_mutex_unlock(&ctx_mgr_mutex);

			return (zb_cli_ctx + i);
		}
	}
	k_mutex_unlock(&ctx_mgr_mutex);

	return NULL;
}

uint8_t ctx_mgr_get_index_by_entry(struct ctx_entry *ctx_entry)
{
	k_mutex_lock(&ctx_mgr_mutex, K_FOREVER);

	if (!PART_OF_ARRAY(zb_cli_ctx, ctx_entry)) {
		k_mutex_unlock(&ctx_mgr_mutex);

		return CTX_MGR_ENTRY_IVALID_INDEX;
	}
	k_mutex_unlock(&ctx_mgr_mutex);

	return (ctx_entry - zb_cli_ctx);
}

struct ctx_entry *ctx_mgr_get_entry_by_index(uint8_t index)
{
	k_mutex_lock(&ctx_mgr_mutex, K_FOREVER);

	if (index < CONFIG_ZIGBEE_SHELL_CTX_MGR_ENTRIES_NBR) {
		k_mutex_unlock(&ctx_mgr_mutex);

		return (zb_cli_ctx + index);
	}
	k_mutex_unlock(&ctx_mgr_mutex);

	return NULL;
}
