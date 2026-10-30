/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "current_time.h"
#include "current_time_grtc.h"
#include "audio_sync_timer.h"


uint32_t current_time_us_get(void)
{
#if defined(CONFIG_HAS_HW_NRF_GRTC)
	return (uint32_t)current_time_grtc_us_get();
#elif defined(CONFIG_NRF_AUDIO_SYNC_TIMER_USES_RTC)
	return audio_sync_timer_capture();
#else
#error "No audio time source defined"
#endif
}
