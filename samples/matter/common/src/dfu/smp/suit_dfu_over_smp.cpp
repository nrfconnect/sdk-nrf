/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dfu_over_smp.h"

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT) || !defined(CONFIG_SUIT)
#error "DFUOverSMP requires MCUMGR and SUIT module configs enabled"
#endif

#include "dfu/ota/ota_util.h"

#include <platform/CHIPDeviceLayer.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;

namespace Nrf
{

DFUOverSMP DFUOverSMP::sDFUOverSMP;

void DFUOverSMP::Init()
{
	const char *name = bt_get_name();

	mAdvertisingItems[0] = BT_DATA(BT_DATA_FLAGS, &kAdvertisingFlags, sizeof(kAdvertisingFlags));
	mAdvertisingItems[1] = BT_DATA(BT_DATA_NAME_COMPLETE, name, static_cast<uint8_t>(strlen(name)));

	mAdvertisingRequest.priority = kAdvertisingPriority;
	mAdvertisingRequest.options = kAdvertisingOptions;
	mAdvertisingRequest.minInterval = kAdvertisingIntervalMin;
	mAdvertisingRequest.maxInterval = kAdvertisingIntervalMax;
	mAdvertisingRequest.advertisingData = Span<bt_data>(mAdvertisingItems);

	mAdvertisingRequest.onStarted = [](int rc) {
		if (rc == 0) {
			ChipLogProgress(SoftwareUpdate, "SMP BLE advertising started");
		} else {
			ChipLogError(SoftwareUpdate, "Failed to start SMP BLE advertising: %d", rc);
		}
	};
}

void DFUOverSMP::ConfirmNewImage()
{
	/* TODO: Add confirming the image using SUIT */
}

void DFUOverSMP::StartServer()
{
	VerifyOrReturn(!mIsStarted, ChipLogProgress(SoftwareUpdate, "SUIT DFU over SMP was already started"));

	/* Synchronize access to the advertising arbiter that normally runs on the CHIP thread. */
	PlatformMgr().LockChipStack();
	BLEAdvertisingArbiter::InsertRequest(mAdvertisingRequest);
	PlatformMgr().UnlockChipStack();

	mIsStarted = true;
	ChipLogProgress(DeviceLayer, "DFU over SMP started");
}

} /* namespace Nrf */
