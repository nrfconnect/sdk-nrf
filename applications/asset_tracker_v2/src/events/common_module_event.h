/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util_macro.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifndef _COMMON_MODULE_EVENT_H_
#define _COMMON_MODULE_EVENT_H_

#define COMMON_EVENT_TYPES						\
	COND_CODE_1(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING,			\
		    (NRF_PROFILER_ARG_STRING),				\
		    (NRF_PROFILER_ARG_U8)),					\

#define COMMON_APP_EVENT_INFO_DEFINE(ename, profile_func)		\
	APP_EVENT_INFO_DEFINE(ename,					\
			  ENCODE(COMMON_EVENT_TYPES),			\
			  ENCODE("type"),				\
			  profile_func)

#define COMMON_APP_EVENT_TYPE_DEFINE(ename, log_fn, ev_info_struct, flags)	\
	APP_EVENT_TYPE_DEFINE(ename,						\
			  log_fn,						\
			  COND_CODE_1(CONFIG_NRF_PROFILER,				\
				      (ev_info_struct),				\
				      (NULL)),					\
			  flags)

#endif /* _COMMON_MODULE_EVENT_H_ */
