/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "device_location.h"

#include <errno.h>
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

	/* Try to write the channel value to UICR */
	ret = uicr_location_set(location);
	if (ret) {
		LOG_ERR("Unable to write channel value to UICR: %d", ret);
	}
}
#endif /* CONFIG_DEVICE_LOCATION_SET_RUNTIME */

static int device_location_init(void)
{
#if CONFIG_DEVICE_LOCATION_SET_RUNTIME
	static_location = uicr_location_get();

	if (static_location >= BT_AUDIO_LOCATION_RIGHT_SURROUND) {
		/* Invalid value in UICR if UICR is not written */
		static_location = DEVICE_LOCATION_DEFAULT;
	}
#else
	static_location = CONFIG_DEVICE_LOCATION_AT_COMPILE_TIME;
#endif /* CONFIG_DEVICE_LOCATION_SET_RUNTIME */

	LOG_WRN("Location bitfield is 0x%x", static_location);
	return 0;
}

SYS_INIT(device_location_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
