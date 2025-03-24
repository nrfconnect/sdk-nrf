/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_RESOURCE_
#define OT_RPC_RESOURCE_

#include <ot_rpc_types.h>

#include <openthread/message.h>
#if defined(CONFIG_OPENTHREAD_MESH_DIAG)
#include <openthread/mesh_diag.h>
#endif /* CONFIG_OPENTHREAD_MESH_DIAG */

#define OT_RPC_RESOURCE_TABLE_DECLARE(name, type)                                                  \
	ot_rpc_res_tab_key ot_res_tab_##name##_alloc(type * ptr);                                  \
	void ot_res_tab_##name##_free(ot_rpc_res_tab_key key);                                     \
	type *ot_res_tab_##name##_get(ot_rpc_res_tab_key key);

#define OT_RPC_RESOURCE_TABLE_REGISTER(name, type, size)                                           \
	static type *ot_##name##_res_tab[size];                                                    \
												   \
	ot_rpc_res_tab_key ot_res_tab_##name##_alloc(type *ptr)                                    \
	{                                                                                          \
		if (ptr == NULL) {                                                                 \
			return 0;                                                                  \
		}                                                                                  \
												   \
		for (ot_rpc_res_tab_key i = 0; i < size; i++) {                                    \
			if (ot_##name##_res_tab[i] == NULL) {                                      \
				ot_##name##_res_tab[i] = ptr;                                      \
				return i + 1;                                                      \
			}                                                                          \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
												   \
	void ot_res_tab_##name##_free(ot_rpc_res_tab_key key)                                      \
	{                                                                                          \
		if (key == 0 || key > size) {                                                      \
			return;                                                                    \
		}                                                                                  \
												   \
		ot_##name##_res_tab[key - 1] = NULL;                                               \
	}                                                                                          \
												   \
	type *ot_res_tab_##name##_get(ot_rpc_res_tab_key key)                                      \
	{                                                                                          \
		if (key == 0 || key > size) {                                                      \
			return NULL;                                                               \
		}                                                                                  \
												   \
		return ot_##name##_res_tab[key - 1];                                               \
	}

/* Declare resource tables that should be accessible by other modules */
OT_RPC_RESOURCE_TABLE_DECLARE(msg, otMessage);
#if defined(CONFIG_OPENTHREAD_MESH_DIAG)
OT_RPC_RESOURCE_TABLE_DECLARE(meshdiag_ip6_it, otMeshDiagIp6AddrIterator);
OT_RPC_RESOURCE_TABLE_DECLARE(meshdiag_child_it, otMeshDiagChildIterator);
#endif /* CONFIG_OPENTHREAD_MESH_DIAG */

#endif /* OT_RPC_RESOURCE_ */
