/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_LOG_H_
#define NRF_RPC_LOG_H_

#include <logging/log.h>

#define _NRF_RPC_LOG_REGISTER2(module) \
	LOG_MODULE_REGISTER(module, CONFIG_ ## module ## _LOG_LEVEL)

#define _NRF_RPC_LOG_REGISTER1(module) \
	_NRF_RPC_LOG_REGISTER2(module)

#define _NRF_RPC_LOG_DECLARE2(module) \
	LOG_MODULE_DECLARE(module, CONFIG_ ## module ## _LOG_LEVEL)

#define _NRF_RPC_LOG_DECLARE1(module) \
	_NRF_RPC_LOG_DECLARE2(module)

#ifdef NRF_RPC_LOG_MODULE
_NRF_RPC_LOG_REGISTER1(NRF_RPC_LOG_MODULE);
#elif defined(NRF_RPC_LOG_MODULE_DECLARE)
_NRF_RPC_LOG_DECLARE1(NRF_RPC_LOG_MODULE_DECLARE);
#endif

#define NRF_RPC_ERR(...)  LOG_ERR(__VA_ARGS__)
#define NRF_RPC_WRN(...)  LOG_WRN(__VA_ARGS__)
#define NRF_RPC_INF(...)  LOG_INF(__VA_ARGS__)
#define NRF_RPC_DBG(...)  LOG_DBG(__VA_ARGS__)

#define NRF_RPC_DUMP_ERR(memory, length, text) \
	LOG_HEXDUMP_ERR(memory, length, text)
#define NRF_RPC_DUMP_WRN(memory, length, text) \
	LOG_HEXDUMP_WRN(memory, length, text)
#define NRF_RPC_DUMP_INF(memory, length, text) \
	LOG_HEXDUMP_INF(memory, length, text)
#define NRF_RPC_DUMP_DBG(memory, length, text) \
	LOG_HEXDUMP_DBG(memory, length, text)

#endif /* NRF_RPC_LOG_H_ */
