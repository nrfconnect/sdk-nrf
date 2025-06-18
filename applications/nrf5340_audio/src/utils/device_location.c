/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "device_location.h"
#include "uicr.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(location, CONFIG_DEVICE_LOCATION_LOG_LEVEL);

static uint32_t static_location;

void device_location_get(enum bt_audio_location *location)
{
	*location = (enum bt_audio_location)static_location;
}

#if CONFIG_DEVICE_LOCATION_SET_RUNTIME
void device_location_set(enum bt_audio_location location)
{
	int ret;

	static_location = location;

	/* Try to write the location to UICR */
	ret = uicr_location_set(location);
	if (ret) {
		LOG_WRN("Unable to write location to UICR: %d. Will revert on boot", ret);
	}
}
#endif /* CONFIG_DEVICE_LOCATION_SET_RUNTIME */

static int device_location_init(void)
{
#if CONFIG_DEVICE_LOCATION_SET_RUNTIME
	static_location = uicr_location_get();

	if (static_location >= BT_AUDIO_LOCATION_RIGHT_SURROUND) {
		/* Invalid value in UICR if UICR is not written */
		LOG_WRN("Invalid location in UICR, using default location");
		static_location = DEVICE_LOCATION_DEFAULT;
	}
#else
	static_location = CONFIG_DEVICE_LOCATION_AT_COMPILE_TIME;
#endif /* CONFIG_DEVICE_LOCATION_SET_RUNTIME */

	LOG_DBG("Location bitfield is 0x%08x", static_location);
	return 0;
}

SYS_INIT(device_location_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
