/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "bolt_lock_manager.h"
#include "led_widget.h"
#include "thread_util.h"

#include <platform/CHIPDeviceLayer.h>

#include <app/common/gen/attribute-id.h>
#include <app/common/gen/attribute-type.h>
#include <app/common/gen/cluster-id.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

/* MCUMgr BT FOTA includes */
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_SMP_BT
#include <mgmt/mcumgr/smp_bt.h>
#endif
#ifdef CONFIG_BOOTLOADER_MCUBOOT
#include <dfu/mcuboot.h>
#endif

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

#include <algorithm>

using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace
{
static constexpr size_t kAppEventQueueSize = 10;
static constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
static constexpr uint32_t kFactoryResetCancelWindow = 3000;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
LEDWidget sStatusLED;
LEDWidget sLockLED;
LEDWidget sUnusedLED;
LEDWidget sUnusedLED_1;

bool sIsThreadProvisioned;
bool sIsThreadEnabled;
bool sHaveBLEConnections;
bool sHaveServiceConnectivity;

k_timer sFunctionTimer;
} /* namespace */

AppTask AppTask::sAppTask;

int AppTask::Init()
{
	/* Initialize LEDs */
	LEDWidget::InitGpio();

	sStatusLED.Init(DK_LED1);
	sLockLED.Init(DK_LED2);
	sLockLED.Set(!BoltLockMgr().IsUnlocked());
	sUnusedLED.Init(DK_LED3);
	sUnusedLED_1.Init(DK_LED4);

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return ret;
	}

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	/* Check if the image is run in the REVERT mode and eventually
	confirm it to prevent reverting on the next boot. */
	if (mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT) {
		if (boot_write_img_confirmed()) {
			LOG_ERR("Confirming firmware image failed, it will be reverted on the next boot.");
		} else {
			LOG_INF("New firmware image confirmed.");
		}
	}
#endif

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Initialize lock manager */
	BoltLockMgr().Init();

	/* Init ZCL Data Model and start server */
	InitServer();
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
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
		sUnusedLED.Animate();
		sUnusedLED_1.Animate();
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

int AppTask::SoftwareUpdateConfirmationHandler(uint32_t offset, uint32_t size, void *arg)
{
	/* For now just print update progress and confirm data chunk without any additional checks. */
	LOG_INF("Software update progress %d B / %d B", offset, size);

	return 0;
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
	case AppEvent::FunctionPress:
		FunctionPressHandler();
		break;
	case AppEvent::FunctionRelease:
		FunctionReleaseHandler();
		break;
	case AppEvent::FunctionTimer:
		FunctionTimerEventHandler();
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

void AppTask::FunctionPressHandler()
{
	sAppTask.StartFunctionTimer(kFactoryResetTriggerTimeout);
	sAppTask.mFunction = TimerFunction::SoftwareUpdate;
}

void AppTask::FunctionReleaseHandler()
{
	if (sAppTask.mFunction == TimerFunction::SoftwareUpdate) {
		sAppTask.CancelFunctionTimer();
		sAppTask.mFunction = TimerFunction::NoneSelected;

#if defined(CONFIG_MCUMGR_SMP_BT) && defined(CONFIG_MCUMGR_CMD_IMG_MGMT) && defined(CONFIG_MCUMGR_CMD_OS_MGMT)
		if (!sAppTask.mSoftwareUpdateEnabled) {
			sAppTask.mSoftwareUpdateEnabled = true;
			os_mgmt_register_group();
			img_mgmt_register_group();
			img_mgmt_set_upload_cb(SoftwareUpdateConfirmationHandler, NULL);
			smp_bt_register();

			LOG_INF("Enabled software update");
		} else {
			LOG_INF("Software update is already enabled");
		}

#else
		LOG_INF("Software update is disabled");
#endif

	} else if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sUnusedLED_1.Set(false);
		sUnusedLED.Set(false);

		/* Set lock status LED back to show state of lock. */
		sLockLED.Set(!BoltLockMgr().IsUnlocked());

		sAppTask.CancelFunctionTimer();
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset has been Canceled");
	}
}

void AppTask::FunctionTimerEventHandler()
{
	if (sAppTask.mFunction == TimerFunction::SoftwareUpdate) {
		LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.", kFactoryResetCancelWindow);
		sAppTask.StartFunctionTimer(kFactoryResetCancelWindow);
		sAppTask.mFunction = TimerFunction::FactoryReset;

		/* Turn off all LEDs before starting blink to make sure blink is co-ordinated. */
		sStatusLED.Set(false);
		sLockLED.Set(false);
		sUnusedLED_1.Set(false);
		sUnusedLED.Set(false);

		sStatusLED.Blink(500);
		sLockLED.Blink(500);
		sUnusedLED.Blink(500);
		sUnusedLED_1.Blink(500);

	} else if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset triggered");
		ConfigurationMgr().InitiateFactoryReset();
	}
}

void AppTask::StartThreadHandler()
{
	if (AddTestPairing() != CHIP_NO_ERROR) {
		LOG_ERR("Failed to add test pairing");
	}

	if (!ConnectivityMgr().IsThreadProvisioned()) {
		StartDefaultThreadNetwork();
		LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration");
	} else {
		LOG_INF("Device is commissioned to a Thread network");
	}
}

void AppTask::StartBLEAdvertisingHandler()
{
	if (chip::DeviceLayer::ConnectivityMgr().IsThreadProvisioned() && !sAppTask.mSoftwareUpdateEnabled) {
		LOG_INF("NFC Tag emulation and BLE advertisement not started - device is commissioned to a Thread network.");
		return;
	}

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
	if (NFCMgr().IsTagEmulationStarted()) {
		LOG_INF("NFC Tag emulation is already started");
	} else {
		ShareQRCodeOverNFC(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
	}
#endif

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE Advertisement is already enabled");
		return;
	}

	if (OpenDefaultPairingWindow(chip::ResetAdmins::kNo) == CHIP_NO_ERROR) {
		LOG_INF("Enabled BLE Advertisement");
	} else {
		LOG_ERR("OpenDefaultPairingWindow() failed");
	}
}

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
void AppTask::ThreadProvisioningHandler(const ChipDeviceEvent *event, intptr_t)
{
	if (event->Type == DeviceEventType::kCHIPoBLEAdvertisingChange &&
	    event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
		NFCMgr().StopTagEmulation();
	}
}
#endif

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	if (DK_BTN1_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionPress });
	} else if (DK_BTN1_MSK & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionRelease });
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

void AppTask::CancelFunctionTimer()
{
	k_timer_stop(&sFunctionTimer);
}

void AppTask::StartFunctionTimer(uint32_t timeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
}

void AppTask::TimerEventHandler(k_timer *timer)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionTimer });
}
