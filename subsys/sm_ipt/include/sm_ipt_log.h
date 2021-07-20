/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SM_IPT_LOG_H_
#define SM_IPT_LOG_H_

#include <logging/log.h>

#ifndef SM_IPT_LOG_MODULE

#define SM_IPT_LOG_MODULE SM_IPT

#endif

#define _SM_IPT_LOG_REGISTER2(module) \
	LOG_MODULE_REGISTER(module, CONFIG_ ## module ## _LOG_LEVEL)

#define _SM_IPT_LOG_REGISTER1(module) \
	_SM_IPT_LOG_REGISTER2(module)

_SM_IPT_LOG_REGISTER1(SM_IPT_LOG_MODULE);

#define SM_IPT_ERR(...)  LOG_ERR(__VA_ARGS__)
#define SM_IPT_WRN(...)  LOG_WRN(__VA_ARGS__)
#define SM_IPT_INF(...)  LOG_INF(__VA_ARGS__)
#define SM_IPT_DBG(...)  LOG_DBG(__VA_ARGS__)

#endif /* SM_IPT_LOG_H_ */
