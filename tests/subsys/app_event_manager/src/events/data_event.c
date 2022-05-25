/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "data_event.h"


static void profile_data_event(struct log_event_buf *buf,
			       const struct app_event_header *aeh)
{
	struct data_event *event = cast_data_event(aeh);

	ARG_UNUSED(event);
	nrf_profiler_log_encode_int8(buf, event->val1);
	nrf_profiler_log_encode_int16(buf, event->val2);
	nrf_profiler_log_encode_int32(buf, event->val3);
	nrf_profiler_log_encode_uint8(buf, event->val1u);
	nrf_profiler_log_encode_uint16(buf, event->val2u);
	nrf_profiler_log_encode_uint32(buf, event->val3u);
	nrf_profiler_log_encode_string(buf, event->descr);
}

APP_EVENT_INFO_DEFINE(data_event,
		  ENCODE(NRF_PROFILER_ARG_S8, NRF_PROFILER_ARG_S16, NRF_PROFILER_ARG_S32,
			 NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U16, NRF_PROFILER_ARG_U32,
			 NRF_PROFILER_ARG_STRING),
		  ENCODE("val1", "val2", "val3",
			 "val1u", "val2u", "val3u",
			 "descr"),
		  profile_data_event);

APP_EVENT_TYPE_DEFINE(data_event,
		  NULL,
		  &data_event_info,
		  APP_EVENT_FLAGS_CREATE());
