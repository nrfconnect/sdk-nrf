/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Application Event Manager private header.
 *
 * Although these defines are globally visible they must not be used directly.
 */

#ifndef _APP_EVENT_MANAGER_PROFILER_TRACER_PRIV_H_
#define _APP_EVENT_MANAGER_PROFILER_TRACER_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Wrappers used for defining event infos */
#ifdef CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION
#define EM_MEM_ADDRESS_LABEL "_em_mem_address_",
#define MEM_ADDRESS_TYPE NRF_PROFILER_ARG_U32,

#else
#define EM_MEM_ADDRESS_LABEL
#define MEM_ADDRESS_TYPE

#endif /* CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION */


#ifdef CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_PROFILE_EVENT_DATA
#define _ARG_LABELS_DEFINE(...) \
	{EM_MEM_ADDRESS_LABEL __VA_ARGS__}
#define _ARG_TYPES_DEFINE(...) \
	 {MEM_ADDRESS_TYPE __VA_ARGS__}

#else
#define _ARG_LABELS_DEFINE(...) \
	{EM_MEM_ADDRESS_LABEL}
#define _ARG_TYPES_DEFINE(...) \
	 {MEM_ADDRESS_TYPE}

#endif /* CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_PROFILE_EVENT_DATA */


/* Declarations and definitions - for more details refer to public API. */
#define _APP_EVENT_INFO_DEFINE(ename, types, labels, profile_func)				\
	const static char *_CONCAT(ename, _nrf_profiler_arg_labels[]) __used =			\
					_ARG_LABELS_DEFINE(labels);				\
	const static enum nrf_profiler_arg _CONCAT(ename, _nrf_profiler_arg_types[]) __used =	\
						_ARG_TYPES_DEFINE(types);			\
	STRUCT_SECTION_ITERABLE(nrf_profiler_info, _CONCAT(ename, _info)) = {			\
				.profile_fn		=					\
					profile_func,						\
				.nrf_profiler_arg_cnt	=					\
					ARRAY_SIZE(_CONCAT(ename, _nrf_profiler_arg_labels)),	\
				.nrf_profiler_arg_labels	=				\
					_CONCAT(ename, _nrf_profiler_arg_labels),		\
				.nrf_profiler_arg_types	=					\
					_CONCAT(ename, _nrf_profiler_arg_types),		\
				.name			=					\
					STRINGIFY(ename)					\
			}


#ifdef __cplusplus
}
#endif

#endif /* _APP_EVENT_MANAGER_PROFILER_TRACER_PRIV_H_ */
