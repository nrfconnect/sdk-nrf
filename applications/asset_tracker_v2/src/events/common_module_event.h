/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/util_macro.h>
#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifndef _COMMON_MODULE_EVENT_H_
#define _COMMON_MODULE_EVENT_H_

#define COMMON_EVENT_TYPES						\
	COND_CODE_1(CONFIG_PROFILER_EVENT_TYPE_STRING,			\
		    (PROFILER_ARG_STRING),				\
		    (PROFILER_ARG_U8)),					\

#define COMMON_EVENT_INFO_DEFINE(ename, profile_func)			\
	EVENT_INFO_DEFINE(ename,					\
			  ENCODE(COMMON_EVENT_TYPES),			\
			  ENCODE("type"),				\
			  profile_func)

#define COMMON_EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct) \
	EVENT_TYPE_DEFINE(ename,					\
			  init_log_en,					\
			  log_fn,					\
			  COND_CODE_1(CONFIG_PROFILER,			\
				      (ev_info_struct),			\
				      (NULL)))

#endif /* _COMMON_MODULE_EVENT_H_ */
