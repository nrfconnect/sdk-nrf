/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_GRAPH_H_
#define ESB_GRAPH_H_

#include <hal/nrf_dppi.h>

#include "esb_common.h"


#if defined(ESB_DPPI_INSTANCE)

#define ESB_MAX_PTX_CHANNELS 0
#define ESB_MAX_PTX_GROUPS 0
#define ESB_MAX_PRX_CHANNELS 9
#define ESB_MAX_PRX_GROUPS 3

#else

#error "Not implemented for PPI (nRF52 series devices)."

#endif


#if IS_ENABLED(CONFIG_ESB_PTX) && IS_ENABLED(CONFIG_ESB_PRX)
#define ESB_MAX_CHANNELS MAX(ESB_MAX_PTX_CHANNELS, ESB_MAX_PRX_CHANNELS)
#define ESB_MAX_GROUPS MAX(ESB_MAX_PTX_GROUPS, ESB_MAX_PRX_GROUPS)
#elif IS_ENABLED(CONFIG_ESB_PTX)
#define ESB_MAX_CHANNELS ESB_MAX_PTX_CHANNELS
#define ESB_MAX_GROUPS ESB_MAX_PTX_GROUPS
#elif IS_ENABLED(CONFIG_ESB_PRX)
#define ESB_MAX_CHANNELS ESB_MAX_PRX_CHANNELS
#define ESB_MAX_GROUPS ESB_MAX_PRX_GROUPS
#endif


/** Enable RADIO shorts in current graph. This macro can be used once per graph with multiple
 *  shorts. One short is a pair (two arguments) of event and task. Use only events and tasks names
 *  without any additional prefixes or suffixes.
 */
#define RADIO_SHORTS(...) _RADIO_SHORTS1(__VA_ARGS__, , , , , , , , , , , , , , , , , , , ,)
#define NRF_RADIO_SHORT___MASK 0
#define _RADIO_SHORT_NAME2(a, b) NRF_RADIO_SHORT_## a ## _ ## b ##_MASK
#define _RADIO_SHORT_NAME(x) _RADIO_SHORT_NAME2 x
#define _RADIO_SHORTS1(a0, b0, a1, b1, a2, b2, a3, b3, a4, b4, a5, b5, a6, b6, a7, b7, a8, b8, ...)\
	if (enable_graph) {                                                                        \
		nrf_radio_shorts_set(NRF_RADIO, FOR_EACH(_RADIO_SHORT_NAME, (|), (a0, b0),         \
		(a1, b1), (a2, b2), (a3, b3), (a4, b4), (a5, b5), (a6, b6), (a7, b7), (a8, b8)));  \
	}


/** Enable TIMER shorts in current graph. This macro can be used once per graph with multiple
 *  shorts. One short is a pair (two arguments) of event and task. Use only events and tasks names
 *  without any additional prefixes or suffixes.
 */
#define TIMER_SHORTS(...) _TIMER_SHORTS1(__VA_ARGS__, , , , , , , , , , , , , , , , , , , ,)
#define NRF_TIMER_SHORT___MASK 0
#define _TIMER_SHORT_NAME2(a, b) NRF_TIMER_SHORT_## a ## _ ## b ##_MASK
#define _TIMER_SHORT_NAME(x) _TIMER_SHORT_NAME2 x
#define _TIMER_SHORTS1(a0, b0, a1, b1, a2, b2, a3, b3, a4, b4, a5, b5, a6, b6, a7, b7, a8, b8, ...)\
	if (enable_graph) {                                                                        \
		nrf_timer_shorts_set(ESB_TIMER, FOR_EACH(_TIMER_SHORT_NAME, (|), (a0, b0),         \
		(a1, b1), (a2, b2), (a3, b3), (a4, b4), (a5, b5), (a6, b6), (a7, b7), (a8, b8)));  \
	}


/** Connect handler to specific RADIO interrupts. The handler function is followed by any number
 *  interrupt names (without prefixes or suffixes). You can use this macro once per graph.
 */
#define RADIO_IRQ(handler, ...)                                                                    \
	if (enable_graph) {                                                                        \
		esb_active_radio_irq = handler;                                                    \
		FOR_EACH(_RADIO_IRQ_CLEAR, (), __VA_ARGS__)                                        \
		nrf_radio_int_enable(NRF_RADIO, FOR_EACH(_RADIO_IRQ_NAME, (|), __VA_ARGS__));      \
		irq_enable(ESB_RADIO_IRQ_NUMBER);                                                  \
	}
#define _RADIO_IRQ_NAME(x) NRF_RADIO_INT_ ## x ## _MASK
#define _RADIO_IRQ_CLEAR(x) nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ ## x);


/** Connect handler to specific TIMER interrupts. The handler function is followed by any number
 *  interrupt names (without prefixes or suffixes). You can use this macro once per graph.
 */
#define TIMER_IRQ(handler, ...)                                                                    \
	if (enable_graph) {                                                                        \
		esb_active_timer_irq = handler;                                                    \
		FOR_EACH(_TIMER_IRQ_CLEAR, (), __VA_ARGS__)                                        \
		nrf_timer_int_enable(ESB_TIMER, FOR_EACH(_TIMER_IRQ_NAME, (|), __VA_ARGS__));      \
		irq_enable(ESB_TIMER_IRQ_NUMBER);                                                  \
	}
#define _TIMER_IRQ_NAME(x) ESB_TIMER_INT_ ## x ## _MASK
#define _TIMER_IRQ_CLEAR(x) nrf_timer_event_clear(ESB_TIMER, NRF_TIMER_EVENT_ ## x);


#if defined(ESB_DPPI_INSTANCE)

/** Macro that can be used only as argument of CONNECT_x_TO_y, REUSABLE, or REUSE macros.
 *  It represents a RADIO event or task. The name is given without any prefixes or suffixes.
 */
#define RADIO(name) \
	nrf_radio_publish_set(NRF_RADIO, NRF_RADIO_EVENT_ ## name, current_channel),               \
	nrf_radio_publish_clear(NRF_RADIO, NRF_RADIO_EVENT_ ## name),                              \
	nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_ ## name, current_channel),              \
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_ ## name)

/** Macro that can be used only as argument of CONNECT_x_TO_y, REUSABLE, or REUSE macros.
 *  It represents a TIMER event or task. The name is given without any prefixes or suffixes.
 */
#define TIMER(name) \
	nrf_timer_publish_set(ESB_TIMER, NRF_TIMER_EVENT_ ## name, current_channel),               \
	nrf_timer_publish_clear(ESB_TIMER, NRF_TIMER_EVENT_ ## name),                              \
	nrf_timer_subscribe_set(ESB_TIMER, NRF_TIMER_TASK_ ## name, current_channel),              \
	nrf_timer_subscribe_clear(ESB_TIMER, NRF_TIMER_TASK_ ## name)

/** Macro that can be used only as argument of CONNECT_x_TO_y, REUSABLE, or REUSE macros.
 *  It represents task that enables specific group. You cannot use with a group that was not
 *  finished yet (with GROUP_END macro) - use ENABLE_GROUP_DECL and ENABLE_GROUP_IMPL instead.
 */
#define ENABLE_GROUP(g)                                                                            \
	_this_is_not_event_,                                                                       \
	_this_is_not_event_,                                                                       \
	nrf_dppi_subscribe_set(ESB_DPPIC, nrf_dppi_group_enable_task_get(esb_groups[g]),           \
			       current_channel),                                                   \
	nrf_dppi_subscribe_clear(ESB_DPPIC, nrf_dppi_group_enable_task_get(esb_groups[g]))

/** Macro that can be used only as argument of CONNECT_x_TO_y, REUSABLE, or REUSE macros.
 *  It represents task that disables specific group. You cannot use with a group that was not
 *  finished yet (with GROUP_END macro) - use ENABLE_GROUP_DECL and ENABLE_GROUP_IMPL instead.
 */
#define DISABLE_GROUP(g)                                                                           \
	_this_is_not_event_,                                                                       \
	_this_is_not_event_,                                                                       \
	nrf_dppi_subscribe_set(ESB_DPPIC, nrf_dppi_group_disable_task_get(esb_groups[g]),          \
			       current_channel),                                                   \
	nrf_dppi_subscribe_clear(ESB_DPPIC, nrf_dppi_group_disable_task_get(esb_groups[g]))

/** Macro that can be used only as argument of CONNECT_x_TO_y, REUSABLE, or REUSE macros.
 *  It works similarly to ENABLE_GROUP, but actual implementation will be later when the group
 *  is ended with GROUP_END macro. If you used this macro, you must later call ENABLE_GROUP_IMPL
 *  exactly once with the same group number.
 */
#define ENABLE_GROUP_DECL(g)                                                                       \
	_this_is_not_event_,                                                                       \
	_this_is_not_event_,                                                                       \
	_enable_group ## g = current_channel,                                                      \
	_enable_group ## g = current_channel

/** Macro that can be used only as argument of CONNECT_x_TO_y, REUSABLE, or REUSE macros.
 *  It works similarly to DISABLE_GROUP, but actual implementation will be later when the group
 *  is ended with GROUP_END macro. If you used this macro, you must later call DISABLE_GROUP_IMPL
 *  exactly once with the same group number.
 */
#define DISABLE_GROUP_DECL(g)                                                                      \
	_this_is_not_event_,                                                                       \
	_this_is_not_event_,                                                                       \
	_disable_group ## g = current_channel,                                                     \
	_disable_group ## g = current_channel

/** Call this function outside connection or group if you used ENABLE_GROUP_DECL before. */
#define ENABLE_GROUP_IMPL(g)                                                                       \
	if (enable_graph) {                                                                        \
		nrf_dppi_subscribe_set(ESB_DPPIC, nrf_dppi_group_enable_task_get(esb_groups[g]),   \
				       _enable_group ## g);                                        \
	} else {                                                                                   \
		nrf_dppi_subscribe_clear(ESB_DPPIC, nrf_dppi_group_enable_task_get(esb_groups[g]));\
	}

/** Call this function outside connection or group if you used DISABLE_GROUP_DECL before. */
#define DISABLE_GROUP_IMPL(g)                                                                      \
	if (enable_graph) {                                                                        \
		nrf_dppi_subscribe_set(ESB_DPPIC, nrf_dppi_group_disable_task_get(esb_groups[g]),  \
				       _disable_group ## g);                                       \
	} else {                                                                                   \
		nrf_dppi_subscribe_clear(ESB_DPPIC,                                                \
			nrf_dppi_group_disable_task_get(esb_groups[g]));                           \
	}

/** Connect events and tasks using PPI or DPPI. The "enable" argument specifies initial state of
 * the connection. DPPI implementation requires one channel per connection, PPI requires multiple
 * channels and can be calculated as:
 * (number of input channels) * ROUND_UP((number of output channels) / 2)
 */
#define CONNECT_1_TO_1(enable, from1, to1)                                                         \
	_CONNECT_DPPI(enable, from1, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      EMPTY, EMPTY, EMPTY, to1, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,   \
		      EMPTY, EMPTY, EMPTY, EMPTY, EMPTY)
#define CONNECT_1_TO_2(enable, from1, to1, to2)                                                    \
	_CONNECT_DPPI(enable, from1, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      EMPTY, EMPTY, EMPTY, to1, to2, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,     \
		      EMPTY, EMPTY)
#define CONNECT_1_TO_3(enable, from1, to1, to2, to3)                                               \
	_CONNECT_DPPI(enable, from1, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      EMPTY, EMPTY, EMPTY, to1, to2, to3, EMPTY, EMPTY, EMPTY, EMPTY)
#define CONNECT_1_TO_4(enable, from1, to1, to2, to3, to4)                                          \
	_CONNECT_DPPI(enable, from1, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      EMPTY, EMPTY, EMPTY, to1, to2, to3, to4)
#define CONNECT_2_TO_1(enable, from1, from2, to1)                                                  \
	_CONNECT_DPPI(enable, from1, from2, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      to1, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,   \
		      EMPTY, EMPTY)
#define CONNECT_2_TO_2(enable, from1, from2, to1, to2)                                             \
	_CONNECT_DPPI(enable, from1, from2, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      to1, to2, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY)
#define CONNECT_2_TO_3(enable, from1, from2, to1, to2, to3)                                        \
	_CONNECT_DPPI(enable, from1, from2, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      to1, to2, to3, EMPTY, EMPTY, EMPTY, EMPTY)
#define CONNECT_2_TO_4(enable, from1, from2, to1, to2, to3, to4)                                   \
	_CONNECT_DPPI(enable, from1, from2, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,\
		      to1, to2, to3, to4)
#define _CONNECT_DPPI(enable,                                                                      \
	from1_set, from1_clear, _a1, _b1,                                                          \
	from2_set, from2_clear, _a2, _b2,                                                          \
	from3_set, from3_clear, _a3, _b3,                                                          \
	from4_set, from4_clear, _a4, _b4,                                                          \
	_c1, _d1, to1_set, to1_clear,                                                              \
	_c2, _d2, to2_set, to2_clear,                                                              \
	_c3, _d3, to3_set, to3_clear,                                                              \
	_c4, _d4, to4_set, to4_clear)                                                              \
	do {                                                                                       \
		uint8_t current_channel = *channel_ptr;                                            \
		__ASSERT(current_channel != 0xFF && channel_ptr - esb_channels < ESB_MAX_CHANNELS, \
			"Channel %d out of range", channel_ptr - esb_channels);                    \
		channel_ptr++;                                                                     \
		if (enable_graph) {                                                                \
			from1_set; from2_set; from3_set; from4_set;                                \
			to1_set; to2_set; to3_set; to4_set;                                        \
		} else {                                                                           \
			from1_clear; from2_clear; from3_clear; from4_clear;                        \
			to1_clear; to2_clear; to3_clear; to4_clear;                                \
		}                                                                                  \
		if (!enable) {                                                                     \
			disabled_mask |= BIT(current_channel);                                     \
		}                                                                                  \
		used_mask |= BIT(current_channel);                                                 \
	} while (0)

/** Begin a group of connections. The group can be enabled and disabled by the hardware with
 *  PPI/DPPI or by the software. Groups can be nested. It uses one group in PPI/DPPI controller.
 */
#define GROUP_BEGIN(group_index)                                                                   \
	do {                                                                                       \
		__ASSERT(group_index < ESB_MAX_GROUPS, "Group %d exceeds max count %d.",           \
			 group_index, ESB_MAX_GROUPS);                                             \
		const uint8_t current_group = esb_groups[group_index];                             \
		uint32_t used_mask_copy;                                                           \
		do {                                                                               \
			uint32_t used_mask = 0;

/** End of group of connections. */
#define GROUP_END()                                                                                \
			if (enable_graph) {                                                        \
				nrf_dppi_channels_group_set(ESB_DPPIC, used_mask, current_group);  \
			}                                                                          \
			used_mask_copy = used_mask;                                                \
		} while (0);                                                                       \
		used_mask |= used_mask_copy;                                                       \
	} while (0)

/** Similar to GROUP_BEGIN, but allows only manual enabling and disabling of the group by software.
 *  Channel mask of this group is returned through out_mask_pointer. It does not require a group
 *  in PPI/DPPI controller.
 */
#define MANUAL_GROUP_BEGIN(out_mask_pointer)                                                       \
	do {                                                                                       \
		uint32_t *const result = (out_mask_pointer);                                       \
		uint32_t used_mask_copy;                                                           \
		do {                                                                               \
			uint32_t used_mask = 0;

/** End of manual group of connections. */
#define MANUAL_GROUP_END()                                                                         \
			if (enable_graph) {                                                        \
				*result = used_mask;                                               \
			}                                                                          \
			used_mask_copy = used_mask;                                                \
		} while (0);                                                                       \
		used_mask |= used_mask_copy;                                                       \
	} while (0)

/** In DPPI, single task or event cannot be assigned to multiple connections. You can use this macro
 *  to pass event or task over EGU to split it into multiple channels. Use this macro if you use
 *  specific event or task for the first time. Use REUSE macro to reuse the same event or task
 *  later. The "channel_var" is a `uint8_t` variable that will hold information needed later for
 *  REUSE macro. In PPI, this macro does nothing.
 */
#define REUSABLE(x, channel_var) _REUSABLE1(x, channel_var)
#define _REUSABLE1(from_set, from_clear, to_set, to_clear, channel_var)\
	do { \
		do { \
			uint8_t current_channel = *channel_ptr;                                    \
			__ASSERT(current_channel != 0xFF &&                                        \
				 channel_ptr - esb_channels < ESB_MAX_CHANNELS,                    \
				 "Channel %d out of range", channel_ptr - esb_channels);           \
			channel_ptr++;                                                             \
			from_set;                                                                  \
			(channel_var) = current_channel;                                           \
			nrf_egu_subscribe_set(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                     \
				(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event,       \
				current_channel);                                                  \
			egu_channels_mask |= BIT(current_channel);                                 \
		} while (0);                                                                       \
		nrf_egu_publish_set(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                            \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event,         \
			current_channel);                                                          \
		egu_event++;                                                                       \
	} while (0),                                                                               \
	do {                                                                                       \
		channel_ptr++;                                                                     \
		nrf_egu_publish_clear(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                          \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event);        \
		nrf_egu_subscribe_clear(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                           \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event);              \
		from_clear;                                                                        \
		egu_event++;                                                                       \
	} while (0),                                                                               \
	do {                                                                                       \
		do {                                                                               \
			uint8_t current_channel = *channel_ptr;                                    \
			__ASSERT(current_channel != 0xFF &&                                        \
				channel_ptr - esb_channels < ESB_MAX_CHANNELS,                     \
				"Channel %d out of range", channel_ptr - esb_channels);            \
			channel_ptr++;                                                             \
			to_set;                                                                    \
			(channel_var) = current_channel;                                           \
			nrf_egu_publish_set(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                    \
				(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event, \
				current_channel);                                                  \
			egu_channels_mask |= BIT(current_channel);                                 \
		} while (0);                                                                       \
		nrf_egu_subscribe_set(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                             \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event,               \
			current_channel);                                                          \
		egu_event++;                                                                       \
	} while (0),                                                                               \
	do {                                                                                       \
		channel_ptr++;                                                                     \
		nrf_egu_subscribe_clear(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                           \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event);              \
		nrf_egu_publish_clear(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                          \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event);        \
		to_clear;                                                                          \
		egu_event++;                                                                       \
	} while (0)

/** Reuse event or task used earlier with the REUSABLE macro. The arguments must be the same as
 *  previously used with REUSABLE macro. In PPI, this macro does nothing.
 */
#define REUSE(x, channel_var)                                                                      \
	do {                                                                                       \
		nrf_egu_subscribe_set(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                             \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event,               \
			(channel_var));                                                            \
		nrf_egu_publish_set(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                            \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event,         \
			current_channel);                                                          \
		egu_event++;                                                                       \
	} while (0),                                                                               \
	do {                                                                                       \
		nrf_egu_subscribe_clear(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                           \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event);              \
		nrf_egu_publish_clear(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                          \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event);        \
		egu_event++;                                                                       \
	} while (0),                                                                               \
	do {                                                                                       \
		nrf_egu_publish_set(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                            \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event,         \
			(channel_var));                                                            \
		nrf_egu_subscribe_set(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                             \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event,               \
			current_channel);                                                          \
		egu_event++;                                                                       \
	} while (0),                                                                               \
	do {                                                                                       \
		nrf_egu_publish_clear(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0 +                          \
			(NRF_EGU_EVENT_TRIGGERED1 - NRF_EGU_EVENT_TRIGGERED0) * egu_event);        \
		nrf_egu_subscribe_clear(ESB_EGU, NRF_EGU_TASK_TRIGGER0 +                           \
			(NRF_EGU_TASK_TRIGGER1 - NRF_EGU_TASK_TRIGGER0) * egu_event);              \
		egu_event++;                                                                       \
	} while (0)

/** Define a DPPI/PPI graph in this function. The function must be of type:
 *  `void graph_name(bool enable_graph, uint32_t flags)`. Any flags that changes structure of the
 *  graph must be in the `flags` argument. This ensures that the graph will be cleared the same way
 *  it was created since flags are stored together with the active graph.
 */
#define GRAPH_BEGIN(this_function_name)                                                            \
	irq_disable(ESB_RADIO_IRQ_NUMBER);                                                         \
	irq_disable(ESB_TIMER_IRQ_NUMBER);                                                         \
	irq_disable(ESB_EGU_IRQ_NUMBER);                                                           \
	if (enable_graph) {                                                                        \
		if (esb_active_graph != NULL) {                                                    \
			esb_active_graph(false, esb_active_graph_flags);                           \
		}                                                                                  \
		esb_active_graph = this_function_name;                                             \
		esb_active_graph_flags = flags;                                                    \
	} else {                                                                                   \
		nrf_radio_int_disable(NRF_RADIO, 0xFFFFFFFF);                                      \
		nrf_timer_int_disable(ESB_TIMER, 0xFFFFFFFF);                                      \
		nrf_radio_shorts_set(NRF_RADIO, 0);                                                \
		nrf_timer_shorts_set(ESB_TIMER, 0);                                                \
		nrf_dppi_channels_disable(ESB_DPPIC, esb_channels_mask);                           \
	}                                                                                          \
	uint32_t used_mask = 0;                                                                    \
	uint32_t disabled_mask = 0;                                                                \
	uint8_t *channel_ptr = esb_channels;                                                       \
	uint8_t egu_event = 1;                                                                     \
	uint32_t egu_channels_mask = 0;                                                            \
	LISTIFY(6, _GRAPH_BEGIN_CHANNELS, ());                                                     \
	ARG_UNUSED(channel_ptr);                                                                   \
	ARG_UNUSED(egu_event);
#define _GRAPH_BEGIN_CHANNELS(i, ...)                                                              \
	uint8_t _enable_group ## i;                                                                \
	uint8_t _disable_group ## i;                                                               \
	ARG_UNUSED(_enable_group ## i);                                                            \
	ARG_UNUSED(_disable_group ## i);

/** Finish graph definition. */
#define GRAPH_END()                                                                                \
	if (enable_graph) {                                                                        \
		nrf_dppi_channels_enable(ESB_DPPIC,                                                \
			(used_mask ^ disabled_mask) | egu_channels_mask);                          \
	}                                                                                          \
	irq_enable(ESB_EGU_IRQ_NUMBER);                                                            \
	__ASSERT(egu_event <= nrf_egu_channel_count(ESB_EGU),                                      \
		 "Graph used too many EGU events %d, max %d.",                                     \
		 egu_event, nrf_egu_channel_count(ESB_EGU));

#else

#error "Not implemented for PPI (nRF52 series devices)."

#endif

/** Bitmask of all allocated DPPI/PPI channels.
 */
extern uint32_t esb_channels_mask;

/** Bitmask of all allocated DPPI/PPI groups.
 */
extern uint32_t esb_groups_mask;

/** Array of allocated DPPI/PPI channels. Each entry holds channel number or 0xFF if not allocated.
 */
extern uint8_t esb_channels[ESB_MAX_CHANNELS];

/** Array of allocated DPPI/PPI groups. Each entry holds group number or 0xFF if not allocated.
 *  Group numbers used in the graph is an index in this array and it may not be the same as the
 *  actual group number in the DPPI/PPI controller.
 */
extern uint8_t esb_groups[ESB_MAX_GROUPS];

/** Function that defines the active graph or NULL if there is no active graph.
 */
extern void (*volatile esb_active_graph)(bool enable_graph, uint32_t flags);

/** Flags used to create the active graph.
 */
extern volatile uint32_t esb_active_graph_flags;

/** Clear the active graph. It will disable and disconnect all channels, shorts, and interrupts.
 */
void esb_clear_graph(void);

/** Allocate specific number of DPPI/PPI channels and groups.
 */
int esb_alloc_channels_and_groups(int max_channels, int max_groups);

/** Deallocates all allocated DPPI/PPI channels and groups.
 */
void esb_free_channels_and_groups(void);

/** Disable specific group.
 */
void esb_disable_group(int group);

/** Enable specific group.
 */
void esb_enable_group(int group);


#endif /* ESB_GRAPH_H_ */
