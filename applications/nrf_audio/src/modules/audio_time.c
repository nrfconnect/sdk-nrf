/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_time.h"
#include "controller_time_grtc.h"
#include "audio_sync_timer.h"


uint32_t audio_time_us_get(void)
{
#if defined(CONFIG_HAS_HW_NRF_GRTC)
	return (uint32_t)controller_time_us_get();
#elif defined(CONFIG_AUDIO_SYNC_TIMER_USES_RTC)
	return audio_sync_timer_capture();
#else
#error "No audio time source defined"
#endif
}
