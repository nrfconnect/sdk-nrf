/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_callback.h"

ot_rpc_callback_id ot_rpc_callback_alloc(struct ot_rpc_callback *table, size_t table_size,
					 ot_rpc_callback_func func, void *context)
{
	struct ot_rpc_callback *cb;
	ot_rpc_callback_id free_id = OT_RPC_CALLBACK_ID_NULL;

	if (func == NULL) {
		return OT_RPC_CALLBACK_ID_NULL;
	}

	for (ot_rpc_callback_id id = 1; id <= table_size; id++) {
		cb = &table[id - 1];

		/* Avoid adding the same function and context twice - reuse the previous entry. */
		if (cb->func == func && cb->context == context) {
			return id;
		}

		if (free_id == OT_RPC_CALLBACK_ID_NULL && cb->func == NULL) {
			free_id = id;
		}
	}

	if (free_id != OT_RPC_CALLBACK_ID_NULL) {
		cb = &table[free_id - 1];
		cb->func = func;
		cb->context = context;
	}

	return free_id;
}

ot_rpc_callback_func ot_rpc_callback_get(struct ot_rpc_callback *table, size_t table_size,
					 ot_rpc_callback_id id, void **context_out)
{
	struct ot_rpc_callback *cb;

	if (id == OT_RPC_CALLBACK_ID_NULL || id > table_size) {
		return NULL;
	}

	cb = &table[id - 1];
	*context_out = cb->context;

	return cb->func;
}

void ot_rpc_callback_free(struct ot_rpc_callback *table, size_t table_size, ot_rpc_callback_id id)
{
	struct ot_rpc_callback *cb;

	if (id == OT_RPC_CALLBACK_ID_NULL || id > table_size) {
		return;
	}

	cb = &table[id - 1];
	cb->func = NULL;
	cb->context = NULL;
}
