/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "bolt_lock_manager.h"
#include "led_widget.h"
#include "thread_util.h"

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
#include "nfc_widget.h"
#endif

#include "attribute-storage.h"
#include "gen/attribute-id.h"
#include "gen/attribute-type.h"
#include "gen/cluster-id.h"

#include <platform/CHIPDeviceLayer.h>

#include <app/server/QRCodeUtil.h>
#include <app/server/Server.h>
#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

#include <algorithm>

using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace
{
static constexpr size_t kAppEventQueueSize = 10;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
LEDWidget sStatusLED;
LEDWidget sLockLED;

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
NFCWidget sNFC;
#endif

bool sIsThreadProvisioned;
bool sIsThreadEnabled;
bool sHaveBLEConnections;
bool sHaveServiceConnectivity;
} /* namespace */

AppTask AppTask::sAppTask;

int AppTask::Init()
{
	/* Initialize LEDs */
	LEDWidget::InitGpio();

	sStatusLED.Init(DK_LED1);
	sLockLED.Init(DK_LED2);
	sLockLED.Set(!BoltLockMgr().IsUnlocked());

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return ret;
	}

	/* Initialize lock manager */
	BoltLockMgr().Init();

	/* Init ZCL Data Model and start server */
	InitServer();
	ConfigurationMgr().LogDeviceConfig();
	PrintQRCode(chip::RendezvousInformationFlags::kBLE);

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
	ret = sNFC.Init(ConnectivityMgr());
	if (ret) {
		LOG_ERR("NFC initialization failed");
		return ret;
	}

	PlatformMgr().AddEventHandler(AppTask::ThreadProvisioningHandler, 0);
#endif

	return 0;
}

int AppTask::StartApp()
{
	int ret = Init();

	if (ret) {
		LOG_ERR("AppTask.Init() failed");
		return ret;
	}

	AppEvent event = {};

	while (true) {
		ret = k_msgq_get(&sAppEventQueue, &event, K_MSEC(10));

		while (!ret) {
			DispatchEvent(event);
			ret = k_msgq_get(&sAppEventQueue, &event, K_NO_WAIT);
		}

		/* Collect connectivity and configuration state from the CHIP stack.  Because the
		 * CHIP event loop is being run in a separate task, the stack must be locked
		 * while these values are queried.  However we use a non-blocking lock request
		 * (TryLockChipStack()) to avoid blocking other UI activities when the CHIP
		 * task is busy (e.g. with a long crypto operation). */

		if (PlatformMgr().TryLockChipStack()) {
			sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
			sIsThreadEnabled = ConnectivityMgr().IsThreadEnabled();
			sHaveBLEConnections = (ConnectivityMgr().NumBLEConnections() != 0);
			sHaveServiceConnectivity = ConnectivityMgr().HaveServiceConnectivity();
			PlatformMgr().UnlockChipStack();
		}

		/* Update the status LED.
		 *
		 * If system has "full connectivity", keep the LED On constantly.
		 *
		 * If thread and service provisioned, but not attached to the thread network yet OR no
		 * connectivity to the service OR subscriptions are not fully established
		 * THEN blink the LED Off for a short period of time.
		 *
		 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
		 * rate of 100ms.
		 *
		 * Otherwise, blink the LED On for a very short time. */
		if (sHaveServiceConnectivity) {
			sStatusLED.Set(true);
		} else if (sIsThreadProvisioned && sIsThreadEnabled) {
			sStatusLED.Blink(950, 50);
		} else if (sHaveBLEConnections) {
			sStatusLED.Blink(100, 100);
		} else {
			sStatusLED.Blink(50, 950);
		}

		sStatusLED.Animate();
		sLockLED.Animate();
	}
}

void AppTask::PostEvent(const AppEvent &event)
{
	if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT)) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::UpdateClusterState()
{
	uint8_t newValue = !BoltLockMgr().IsUnlocked();

	/* write the new on/off value */
	EmberAfStatus status = emberAfWriteAttribute(1, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID,
						     CLUSTER_MASK_SERVER, &newValue, ZCL_BOOLEAN_ATTRIBUTE_TYPE);

	if (status != EMBER_ZCL_STATUS_SUCCESS) {
		LOG_ERR("Updating on/off cluster failed: %x", status);
	}
}

void AppTask::DispatchEvent(const AppEvent &event)
{
	switch (event.Type) {
	case AppEvent::Lock:
		LockActionHandler(BoltLockManager::Action::Lock, event.LockEvent.ChipInitiated);
		break;
	case AppEvent::Unlock:
		LockActionHandler(BoltLockManager::Action::Unlock, event.LockEvent.ChipInitiated);
		break;
	case AppEvent::Toggle:
		LockActionHandler(BoltLockMgr().IsUnlocked() ? BoltLockManager::Action::Lock :
							       BoltLockManager::Action::Unlock,
				  event.LockEvent.ChipInitiated);
		break;
	case AppEvent::CompleteLockAction:
		CompleteLockActionHandler();
		break;
	case AppEvent::FactoryReset:
		LOG_INF("Factory Reset triggered");
		ConfigurationMgr().InitiateFactoryReset();
		break;
	case AppEvent::StartThread:
		StartThreadHandler();
		break;
	case AppEvent::StartBleAdvertising:
		StartBLEAdvertisingHandler();
		break;
	default:
		LOG_INF("Unknown event received");
		break;
	}
}

void AppTask::LockActionHandler(BoltLockManager::Action action, bool chipInitiated)
{
	if (BoltLockMgr().InitiateAction(action, chipInitiated)) {
		sLockLED.Blink(50, 50);
	}
}

void AppTask::CompleteLockActionHandler()
{
	bool chipInitiatedAction;

	if (!BoltLockMgr().CompleteCurrentAction(chipInitiatedAction)) {
		return;
	}

	sLockLED.Set(!BoltLockMgr().IsUnlocked());

	/* If the action wasn't initiated by CHIP, update CHIP clusters with the new lock state */
	if (!chipInitiatedAction) {
		UpdateClusterState();
	}
}

void AppTask::StartThreadHandler()
{
	if (!ConnectivityMgr().IsThreadProvisioned()) {
		StartDefaultThreadNetwork();
		LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration");
	} else {
		LOG_INF("Device is commissioned to a Thread network");
	}
}

void AppTask::StartBLEAdvertisingHandler()
{
	if (chip::DeviceLayer::ConnectivityMgr().IsThreadProvisioned()) {
		LOG_INF("NFC Tag emulation and BLE advertisement not started - device is commissioned to a Thread network.");
		return;
	}

	if (!sNFC.IsTagEmulationStarted()) {
		if (!(GetAppTask().StartNFCTag() < 0)) {
			LOG_INF("Started NFC Tag emulation");
		} else {
			LOG_ERR("Starting NFC Tag failed");
		}
	} else {
		LOG_INF("NFC Tag emulation is already started");
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE Advertisement is already enabled");
		return;
	}

	if (OpenDefaultPairingWindow() == CHIP_NO_ERROR) {
		LOG_INF("Enabled BLE Advertisement");
	} else {
		LOG_ERR("OpenDefaultPairingWindow() failed");
	}
}

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
int AppTask::StartNFCTag()
{
	/* Get QR Code and emulate its content using NFC tag */
	std::string QRCode;

	int result = GetQRCode(QRCode, chip::RendezvousInformationFlags::kBLE);
	VerifyOrExit(!result, ChipLogError(AppServer, "Getting QR code payload failed"));

	std::replace(QRCode.begin(), QRCode.end(), ' ', '_');

	result = sNFC.StartTagEmulation(QRCode.c_str(), QRCode.size());
	VerifyOrExit(result >= 0, ChipLogError(AppServer, "Starting NFC Tag emulation failed"));

exit:
	return result;
}

void AppTask::ThreadProvisioningHandler(const ChipDeviceEvent *event, intptr_t arg)
{
	ARG_UNUSED(arg);
	if ((event->Type == DeviceEventType::kServiceProvisioningChange) && ConnectivityMgr().IsThreadProvisioned()) {
		const int result = sNFC.StopTagEmulation();
		if (result) {
			LOG_ERR("Stopping NFC Tag emulation failed");
		}
	}
}
#endif

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	if (DK_BTN1_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FactoryReset });
	}

	if (DK_BTN2_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::Toggle, false });
	}

	if (DK_BTN3_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::StartThread });
	}

	if (DK_BTN4_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::StartBleAdvertising });
	}
}
