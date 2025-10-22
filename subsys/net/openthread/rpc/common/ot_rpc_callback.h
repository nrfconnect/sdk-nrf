/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_CALLBACK_H
#define OT_RPC_CALLBACK_H

#include <stdint.h>
#include <stddef.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

/**
 * @brief Generic function pointer type.
 *
 * A function pointer type that is guaranteed to store a pointer to function of any type.
 */
typedef void (*ot_rpc_callback_func)(void);

/**
 * @brief Structure to hold a function pointer of any type and its related context.
 */
struct ot_rpc_callback {
	ot_rpc_callback_func func;
	void *context;
};

/**
 * @brief OpenThread RPC callback ID.
 *
 * A portable representation of a callback and its related context that can be passed to a remote
 * device during remote procedure calls.
 */
typedef uint32_t ot_rpc_callback_id;

enum {
	OT_RPC_CALLBACK_ID_NULL = 0,
};

ot_rpc_callback_id ot_rpc_callback_alloc(struct ot_rpc_callback *table, size_t table_size,
					 ot_rpc_callback_func func, void *context);
ot_rpc_callback_func ot_rpc_callback_get(struct ot_rpc_callback *table, size_t table_size,
					 ot_rpc_callback_id id, void **context_out);
void ot_rpc_callback_free(struct ot_rpc_callback *table, size_t table_size, ot_rpc_callback_id id);

/**
 * @brief Defines a named callback table with the specified function pointer type and size.
 *
 * This creates an array of callback entries and provides type-safe functions for managing and
 * accessing the callback table:
 *
 * @code
 * ot_rpc_callback_id ot_rpc_NAME_alloc(TYPE func, void *context);
 * TYPE ot_rpc_NAME_get(ot_rpc_callback_id id, void **context_out);
 * void ot_rpc_NAME_free(ot_rpc_callback_id id);
 * @endcode
 *
 * The callback table enables the local device to register a callback on the peer device without
 * sending the actual raw function pointer and context. Instead, the sender stores the raw function
 * pointer and context in a local callback table and sends only the identifier of the allocated
 * entry.
 * This approach enhances portability and reduces the risk of errors in callback handling.
 */
#define OT_RPC_CALLBACK_TABLE_DEFINE(name, type, size)                                             \
	static struct ot_rpc_callback name[size];                                                  \
                                                                                                   \
	static inline ot_rpc_callback_id ot_rpc_##name##_alloc(type func, void *context)           \
	{                                                                                          \
		return ot_rpc_callback_alloc(name, ARRAY_SIZE(name), (ot_rpc_callback_func)func,   \
					     context);                                             \
	}                                                                                          \
	static inline type ot_rpc_##name##_get(ot_rpc_callback_id id, void **context_out)          \
	{                                                                                          \
		return (type)ot_rpc_callback_get(name, ARRAY_SIZE(name), id, context_out);         \
	}                                                                                          \
	static inline void ot_rpc_##name##_free(ot_rpc_callback_id id)                             \
	{                                                                                          \
		ot_rpc_callback_free(name, ARRAY_SIZE(name), id);                                  \
	}

#endif /* OT_RPC_CALLBACK_H */
