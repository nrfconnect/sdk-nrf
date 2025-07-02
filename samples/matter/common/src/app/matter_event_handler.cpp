/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "matter_event_handler.h"
#include "group_data_provider.h"

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#include <platform/ConnectivityManager.h>

#ifdef CONFIG_CHIP_NFC_ONBOARDING_PAYLOAD
#include <platform/NFCOnboardingPayloadManager.h>
#include <setup_payload/OnboardingCodesUtil.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip::DeviceLayer;

namespace Nrf::Matter
{
CHIP_ERROR RegisterEventHandler(PlatformManager::EventHandlerFunct handler, intptr_t arg)
{
	StackLock lock;
	return PlatformMgr().AddEventHandler(handler, arg);
}

void UnregisterEventHandler(PlatformManager::EventHandlerFunct handler, intptr_t arg)
{
	StackLock lock;
	PlatformMgr().RemoveEventHandler(handler, arg);
}

void DefaultEventHandler(const ChipDeviceEvent *event, intptr_t /* unused */)
{
	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_ONBOARDING_PAYLOAD
		if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCOnboardingPayloadMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCOnboardingPayloadMgr().StopTagEmulation();
		}
#endif
		break;
#if CONFIG_CHIP_OTA_REQUESTOR
	case DeviceEventType::kServerReady:
		InitBasicOTARequestor();
		break;
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
	case DeviceEventType::kFactoryReset:
		GroupDataProviderImpl::Instance().WillBeFactoryReset();
		break;
	default:
		break;
	}
}

} // namespace Nrf::Matter
