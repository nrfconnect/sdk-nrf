/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#define MODULE file_readme
#include "fs_event.h"

#if CONFIG_FS_FATFS_LFN
#define FILE_NAME         "README.txt"
#else
#define FILE_NAME         "README.TXT"
#endif
#define FILE_CONTENTS     file_contents
#define FILE_CONTENTS_LEN strlen(file_contents)

static const char file_contents[] =
	"====================\r\n"
	"  Nordic Thingy:91  \r\n"
	"====================\r\n"
	"\r\n"
	"Full Nordic Thingy:91 documentation can be found here:\r\n"
	"https://infocenter.nordicsemi.com/topic/ug_thingy91/UG/thingy91/intro/frontpage.html?cp=12_0\r\n"
	"\r\n"
	"This USB interface has the following functions:\r\n"
	"- Disk drive containing this file and others\r\n"
#if CONFIG_BRIDGE_CDC_ENABLE
	"- COM ports for nRF91 debug, trace, and firmware update\r\n"
#endif
	"\r\n"
	"For LTE link and debug monitoring, please see:\r\n"
	"https://infocenter.nordicsemi.com/topic/ug_thingy91/UG/thingy91/getting_started/connecting_lte_link_monitor.html?cp=12_0_2_0\r\n"

#if CONFIG_BRIDGE_CDC_ENABLE
	"\r\n"
	"COM Ports\r\n"
	"====================\r\n"
	"This USB interface exposes two COM ports mapped to the physical UART interfaces between the nRF91 and nRF52840 SoCs.\r\n"
	"When opening these ports manually (without using LTE Link Monitor) be aware that the USB COM port baud rate selection is applied to the UART.\r\n"
#endif

#if CONFIG_BRIDGE_BLE_ENABLE
	"\r\n"
	"BLE UART Service\r\n"
	"====================\r\n"
	"This device will advertise as \"" CONFIG_BT_DEVICE_NAME "\".\r\n"
	"Connect using a BLE Central device, for example a phone running the nRF Connect app:\r\n"
	"https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-mobile/\r\n"
	"\r\n"
	"NOTE: The BLE interface is unencrypted and intended to be used during debugging.\r\n"
	"      By default the BLE interface is disabled.\r\n"
	"      Enable it by setting the appropriate option in Config.txt.\r\n"
#endif
;

static bool event_handler(const struct event_header *eh)
{
	if (is_fs_event(eh)) {
		const struct fs_event *event =
			cast_fs_event(eh);

		if (event->req == FS_REQUEST_CREATE_FILE) {
			int err;

			err = fs_event_helper_file_write(
				event->mnt_point,
				FILE_NAME,
				FILE_CONTENTS,
				FILE_CONTENTS_LEN);

			__ASSERT_NO_MSG(err == 0);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, fs_event);
