/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_rpc_cbkproxy Bluetooth RPC callback proxy API
 * @{
 * @brief API for the Bluetooth RPC callback proxy.
 */


#ifndef CBKPROXY_H
#define CBKPROXY_H

#include <stdint.h>

/** @brief Creates a handler for output callback proxy.
 *
 * It is a variadic macro that takes following parameters:
 *
 * ''CBKPROXY_HANDLER(handler_name, callback_name, [return_type,]
 *                    [handler_parameters, callback_arguments])''
 *
 * @param handler_name  name of the handler function that can ne passed to @ref
 *                      cbkproxy_out_get() function.
 * @param callback_name Callback that will be called by this handler. Best
 *                      approach is to declare it as 'static inline' to make
 *                      sure that handler and callback will be combined into
 *                      one function during the optimization. This callback
 *                      has the same parameters as the original one (provided
 *                      in ''handler_parameters'') with following parameter
 *                      appended to the end: ''uint32_t callback_slot''.
 * @param return_type   Optional return type of the callback. It must be
 *                      convertable to ''uint32_t'' without lost of information.
 *                      If it is ''void'' this macro parameter must be skipped.
 * @param handler_parameters
 *                      List of parameters with its types enclosed in
 *                      parentheses, e.g. ''(uint8_t *buf, size_t len)''.
 *                      This macro parameter is optional and must be skipped
 *                      if callback does not take any parameres.
 * @param callback_arguments
 *                      List of arguments with the same names as in
 *                      ''handler_parameters'' enclosed in parentheses,
 *                      e.g. ''(buf, len)''. This macro parameter is optional
 *                      and must be skipped if callback does not take any
 *                      parameres.
 */
#define CBKPROXY_HANDLER(...) \
	_CBKPROXY_HANDLER_CAT(_CBKPROXY_HANDLER_, _CBKPROXY_HANDLER_CNT(__VA_ARGS__)) (__VA_ARGS__)
#define _CBKPROXY_HANDLER_CNT2(a, b, c, d, e, f, ...) f
#define _CBKPROXY_HANDLER_CNT(...) \
	_CBKPROXY_HANDLER_CNT2(__VA_ARGS__, RET_PARAM, VOID_PARAM, RET_VOID, VOID_VOID)
#define _CBKPROXY_HANDLER_CAT2(a, b) a ## b
#define _CBKPROXY_HANDLER_CAT(a, b) _CBKPROXY_HANDLER_CAT2(a, b)
#define _CBKPROXY_HANDLER_PAR1(...) \
	uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1, uint32_t _ret, __VA_ARGS__
#define _CBKPROXY_HANDLER_PAR(p) (_CBKPROXY_HANDLER_PAR1 p)
#define _CBKPROXY_HANDLER_ARG1(...) __VA_ARGS__, callback_slot
#define _CBKPROXY_HANDLER_ARG(p) (_CBKPROXY_HANDLER_ARG1 p)
#define _CBKPROXY_HANDLER_VOID_VOID(f, h) \
	uint64_t f(uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1, \
		   uint32_t _ret) \
	{ \
		h(); \
		return (uint64_t)_ret << 32; \
	}
#define _CBKPROXY_HANDLER_RET_VOID(f, h, r) \
	uint64_t f(uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1, uint32_t _ret) \
	{ \
		BUILD_ASSERT(sizeof(r) <= 4, "Return value does not fit into uint32_t."); \
		uint32_t _low_ret = (uint32_t)h(); \
		return (uint64_t)_low_ret | ((uint64_t)_ret << 32); \
	}
#define _CBKPROXY_HANDLER_VOID_PARAM(f, h, p, a) \
	uint64_t f _CBKPROXY_HANDLER_PAR(p) \
	{ \
		h _CBKPROXY_HANDLER_ARG(a); \
		return (uint64_t)_ret << 32; \
	}
#define _CBKPROXY_HANDLER_RET_PARAM(f, h, r, p, a) \
	uint64_t f _CBKPROXY_HANDLER_PAR(p) \
	{ \
		BUILD_ASSERT(sizeof(r) <= 4, "Return value does not fit into uint32_t."); \
		uint32_t _low_ret = (uint32_t)h _CBKPROXY_HANDLER_ARG(a); \
		return (uint64_t)_low_ret | ((uint64_t)_ret << 32); \
	}

/** @brief Declares a handler for output callback proxy.
 *
 * This declaration can be placed in the header file to share one handler
 * in multiple source files.
 *
 * All parameter must be the same as in related handler defined by
 * the CBKPROXY_HANDLER macro except callback_name which should be skipped in this macro.
 */
#define CBKPROXY_HANDLER_DECL(...) _CBKPROXY_HANDLER_CAT(_CBKPROXY_HANDLER_DECL_, \
	_CBKPROXY_HANDLER_DECL_CNT(__VA_ARGS__)) (__VA_ARGS__)
#define _CBKPROXY_HANDLER_DECL_CNT(...) \
	_CBKPROXY_HANDLER_CNT2(_, __VA_ARGS__, RET_PARAM, VOID_PARAM, RET_VOID, VOID_VOID)
#define _CBKPROXY_HANDLER_DECL_VOID_VOID(f) \
	uint64_t f(uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1, uint32_t _ret)
#define _CBKPROXY_HANDLER_DECL_RET_VOID(f, r) \
	uint64_t f(uint32_t callback_slot, uint32_t _rsv0, uint32_t _rsv1, uint32_t _ret)
#define _CBKPROXY_HANDLER_DECL_VOID_PARAM(f, p, a) \
	uint64_t f _CBKPROXY_HANDLER_PAR(p)
#define _CBKPROXY_HANDLER_DECL_RET_PARAM(f, r, p, a) \
	uint64_t f _CBKPROXY_HANDLER_PAR(p)

/** @brief Get output callback proxy.
 *
 * This function creates an output callback proxy. Calling proxy will call
 * handler with untouched parameters on stack and registers. Because of that
 * type of handler must be the same as callback proxy. This function is
 * type independent, so function pointer are void* and caller is responsible
 * for correct type casting. Handler is called with one additional information
 * (obtained by cbkproxy_out_start_handler) which is a slot number. It can be
 * used to recognize with callback proxy was called in single handler.
 *
 * @param index    Slot index.
 * @param handler  Pointer to handler function. The function has to be created
 *                 by the @ref CBKPROXY_HANDLER macro.
 * @returns        Pointer to function that calls provided handler using
 *                 provided slot index. The pointer has to be casted to
 *                 the same type as handler. NULL when index is too high or
 *                 the function was called again with the same index, but
 *                 different handler.
 */
void *cbkproxy_out_get(int index, void *handler);

/** @brief Sets input callback proxy.
 *
 * Calling the function again with the same callback parameter will not
 * allocate new slot, but it will return previously allocated.
 *
 * @param callback Callback function.
 *
 * @returns Slot number or -1 if no more slots are available.
 */
int cbkproxy_in_set(void *callback);

/** @brief Gets input callback proxy.
 *
 * @param      index     Slot index.
 *
 * @returns Callback function or NULL if slot index is invalid.
 */
void *cbkproxy_in_get(int index);

#endif /* CBKPROXY_H */
