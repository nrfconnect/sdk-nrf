/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "matter_event_handler.h"

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#include <platform/ConnectivityManager.h>

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
#include <app/server/OnboardingCodesUtil.h>
#include <platform/NFCManager.h>
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
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
		if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCMgr().StopTagEmulation();
		}
#endif
		break;
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kDnssdInitialized:
#if CONFIG_CHIP_OTA_REQUESTOR
		InitBasicOTARequestor();
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
		break;
#elif defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->WiFiConnectivityChange.Result == kConnectivity_Established) {
			InitBasicOTARequestor();
		}
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
#endif /* CONFIG_NET_L2_OPENTHREAD */
		break;
	default:
		break;
	}
}

} // namespace Nrf::Matter
