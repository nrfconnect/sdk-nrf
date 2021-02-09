/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "thread_util.h"

#include <platform/CHIPDeviceLayer.h>
#include <platform/internal/DeviceNetworkInfo.h>

#include <zephyr.h>

#include <cstring>

void StartDefaultThreadNetwork(uint64_t datasetTimestamp)
{
	using namespace chip::DeviceLayer;

	Internal::DeviceNetworkInfo deviceNetworkInfo = {};

	const uint8_t masterKey[Internal::kThreadMasterKeyLength] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
								      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

	memcpy(deviceNetworkInfo.ThreadNetworkName, CONFIG_OPENTHREAD_NETWORK_NAME,
	       strlen(CONFIG_OPENTHREAD_NETWORK_NAME));
	net_bytes_from_str(deviceNetworkInfo.ThreadExtendedPANId, 8, CONFIG_OPENTHREAD_XPANID);
	deviceNetworkInfo.FieldPresent.ThreadExtendedPANId = true;
	memcpy(deviceNetworkInfo.ThreadMasterKey, masterKey, sizeof(masterKey));
	deviceNetworkInfo.ThreadPANId = CONFIG_OPENTHREAD_PANID;
	deviceNetworkInfo.ThreadChannel = CONFIG_OPENTHREAD_CHANNEL;
	deviceNetworkInfo.ThreadDatasetTimestamp = datasetTimestamp;

	ThreadStackMgr().SetThreadEnabled(false);
	ThreadStackMgr().SetThreadProvision(deviceNetworkInfo);
	ThreadStackMgr().SetThreadEnabled(true);
}
