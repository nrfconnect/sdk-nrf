/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dfu_over_smp.h"

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT)
#error "DFU over SMP requires MCUmgr Bluetooth LE module config enabled"
#endif

#if !defined(CONFIG_SUIT) && (!defined(CONFIG_MCUMGR_GRP_IMG) || !defined(CONFIG_MCUMGR_GRP_OS))
#error "DFU over SMP requires MCUmgr IMG and OS groups"
#endif

#include "dfu/ota/ota_util.h"

#include <platform/CHIPDeviceLayer.h>

#include <lib/support/logging/CHIPLogging.h>

#ifndef CONFIG_SUIT
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#endif

#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;

constexpr static uint8_t kAdvertisingPriority = UINT8_MAX;
constexpr static uint32_t kAdvertisingOptions = BT_LE_ADV_OPT_CONNECTABLE;
constexpr static uint16_t kAdvertisingIntervalMin = 400;
constexpr static uint16_t kAdvertisingIntervalMax = 500;
constexpr static uint8_t kAdvertisingFlags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

namespace
{
enum mgmt_cb_return UploadConfirmHandler(uint32_t event, enum mgmt_cb_return prev_status, int32_t *rc, uint16_t *group,
					 bool *abort_more, void *data, size_t data_size)
{
/* Currently img_mgmt hooks are not supported by SUIT */
#ifndef CONFIG_SUIT
	const img_mgmt_upload_check &imgData = *static_cast<img_mgmt_upload_check *>(data);
	IgnoreUnusedVariable(imgData);

	ChipLogProgress(SoftwareUpdate, "DFU over SMP progress: %u/%u B of image %u",
			static_cast<unsigned>(imgData.req->off), static_cast<unsigned>(imgData.action->size),
			static_cast<unsigned>(imgData.req->image));
#endif
	return MGMT_CB_OK;
}

enum mgmt_cb_return CommandHandler(uint32_t event, enum mgmt_cb_return prev_status, int32_t *rc, uint16_t *group,
				   bool *abort_more, void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_CMD_RECV) {
		Nrf::Matter::GetFlashHandler().DoAction(ExternalFlashManager::Action::WAKE_UP);
	} else if (event == MGMT_EVT_OP_CMD_DONE) {
		Nrf::Matter::GetFlashHandler().DoAction(ExternalFlashManager::Action::SLEEP);
	}

	return MGMT_CB_OK;
}

mgmt_callback sUploadCallback = {
	.callback = UploadConfirmHandler,
	.event_id = MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK,
};

mgmt_callback sCommandCallback = {
	.callback = CommandHandler,
	.event_id = (MGMT_EVT_OP_CMD_RECV | MGMT_EVT_OP_CMD_DONE),
};

} /* namespace */

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

	mgmt_callback_register(&sUploadCallback);
	mgmt_callback_register(&sCommandCallback);
}

void DFUOverSMP::ConfirmNewImage()
{
#ifndef CONFIG_SUIT
	/* Check if the image is run in the REVERT mode and eventually */
	/* confirm it to prevent reverting on the next boot. */
	VerifyOrReturn(mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT);

	if (boot_write_img_confirmed()) {
		ChipLogError(SoftwareUpdate, "Failed to confirm firmware image, it will be reverted on the next boot");
	} else {
		ChipLogProgress(SoftwareUpdate, "New firmware image confirmed");
	}
#endif
}

void DFUOverSMP::StartServer()
{
	VerifyOrReturn(!mIsStarted, ChipLogProgress(SoftwareUpdate, "DFU over SMP was already started"));

	/* Synchronize access to the advertising arbiter that normally runs on the CHIP thread. */
	PlatformMgr().LockChipStack();
	BLEAdvertisingArbiter::InsertRequest(mAdvertisingRequest);
	PlatformMgr().UnlockChipStack();

	mIsStarted = true;
	ChipLogProgress(DeviceLayer, "DFU over SMP started");
}

} /* namespace Nrf */
