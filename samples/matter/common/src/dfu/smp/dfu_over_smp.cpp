/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dfu_over_smp.h"

#if !defined(CONFIG_MCUMGR_TRANSPORT_BT)
#error "DFU over SMP requires MCUmgr Bluetooth LE module config enabled"
#endif

#if (!defined(CONFIG_MCUMGR_GRP_IMG) || !defined(CONFIG_MCUMGR_GRP_OS))
#error "DFU over SMP requires MCUmgr IMG and OS groups"
#endif

#include "dfu/ota/ota_util.h"

#include <platform/CHIPDeviceLayer.h>
#include <platform/nrfconnect/DFUSync.h>

#include <lib/support/logging/CHIPLogging.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>

#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;

constexpr static uint8_t kAdvertisingPriority = UINT8_MAX;
constexpr static uint32_t kAdvertisingOptions = BT_LE_ADV_OPT_CONN;
constexpr static uint16_t kAdvertisingIntervalMin = 400;
constexpr static uint16_t kAdvertisingIntervalMax = 500;
constexpr static uint8_t kAdvertisingFlags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

static bool sDfuInProgress = false;
static uint32_t sDfuSyncMutexId;

namespace
{
enum mgmt_cb_return UploadConfirmHandler(uint32_t, enum mgmt_cb_return, int32_t *rc, uint16_t *,
					 bool *, void *data, size_t)
{
	const img_mgmt_upload_check &imgData = *static_cast<img_mgmt_upload_check *>(data);
	IgnoreUnusedVariable(imgData);

	if (!sDfuInProgress) {
		if (DFUSync::GetInstance().Take(sDfuSyncMutexId) == CHIP_NO_ERROR) {
			sDfuInProgress = true;
		} else {
			ChipLogError(SoftwareUpdate, "Cannot start DFU over SMP, another DFU in progress.");
			*rc = MGMT_ERR_EBUSY;
			return MGMT_CB_ERROR_RC;
		}
	}

	ChipLogProgress(SoftwareUpdate, "DFU over SMP progress: %u/%u B of image %u",
			static_cast<unsigned>(imgData.req->off), static_cast<unsigned>(imgData.action->size),
			static_cast<unsigned>(imgData.req->image));
	return MGMT_CB_OK;
}

enum mgmt_cb_return CommandHandler(uint32_t event, enum mgmt_cb_return, int32_t *, uint16_t *,
				   bool *, void *, size_t)
{
	if (event == MGMT_EVT_OP_CMD_RECV) {
		ExternalFlashManager::GetInstance().DoAction(ExternalFlashManager::Action::WAKE_UP);
	} else if (event == MGMT_EVT_OP_CMD_DONE) {
		ExternalFlashManager::GetInstance().DoAction(ExternalFlashManager::Action::SLEEP);
	}

	return MGMT_CB_OK;
}

enum mgmt_cb_return DfuStoppedHandler(uint32_t, enum mgmt_cb_return, int32_t *, uint16_t *,
				      bool *, void *, size_t)
{
	DFUSync::GetInstance().Free(sDfuSyncMutexId);

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

mgmt_callback sDfuStopped = {
	.callback = DfuStoppedHandler,
	.event_id = (MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED | MGMT_EVT_OP_IMG_MGMT_DFU_PENDING),
};

} /* namespace */

namespace Nrf
{

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = DFUOverSMP::Disconnected,
};

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
	mgmt_callback_register(&sDfuStopped);
}

void DFUOverSMP::ConfirmNewImage()
{
#if !defined(CONFIG_BOOT_UPGRADE_ONLY)
	/* Check if the image is run in the REVERT mode and eventually */
	/* confirm it to prevent reverting on the next boot. */
	VerifyOrReturn(mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT);
#endif

	if (boot_write_img_confirmed()) {
		ChipLogError(SoftwareUpdate, "Failed to confirm firmware image, it will be reverted on the next boot");
	} else {
		ChipLogProgress(SoftwareUpdate, "New firmware image confirmed");
	}
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

void DFUOverSMP::Disconnected(bt_conn *conn, uint8_t reason)
{
	bt_conn_info btInfo;

	VerifyOrReturn(sDfuInProgress);
	VerifyOrReturn(bt_conn_get_info(conn, &btInfo) == 0);

	// Drop all callbacks incoming for the role other than peripheral, required by the Matter accessory
	VerifyOrReturn(btInfo.role == BT_CONN_ROLE_PERIPHERAL);

	sDfuInProgress = false;

	DFUSync::GetInstance().Free(sDfuSyncMutexId);
}

} /* namespace Nrf */
