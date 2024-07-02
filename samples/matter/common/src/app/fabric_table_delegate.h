/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <lib/support/logging/CHIPLogging.h>

#include "app/group_data_provider.h"

#ifdef CONFIG_CHIP_WIFI
#include <platform/nrfconnect/wifi/WiFiManager.h>
#endif

namespace Nrf::Matter
{

class AppFabricTableDelegate : public chip::FabricTable::Delegate {
public:
	~AppFabricTableDelegate() { chip::Server::GetInstance().GetFabricTable().RemoveFabricDelegate(this); }

	/**
	 * @brief Initialize module and add a delegation to the Fabric Table.
	 *
	 * To use the OnFabricRemoved method defined within this class and allow to react on the last fabric removal
	 * this method should be called in the application code.
	 */
	static void Init()
	{
#ifndef CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
		static AppFabricTableDelegate sAppFabricDelegate;
		chip::Server::GetInstance().GetFabricTable().AddFabricDelegate(&sAppFabricDelegate);
		k_timer_init(&sFabricRemovedTimer, &OnFabricRemovedTimerCallback, nullptr);
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
	}

private:
	void OnFabricRemoved(const chip::FabricTable &fabricTable, chip::FabricIndex fabricIndex)
	{
		k_timer_start(&sFabricRemovedTimer, K_MSEC(CONFIG_CHIP_LAST_FABRIC_REMOVED_ACTION_DELAY), K_NO_WAIT);
	}

	static void OnFabricRemovedTimerCallback(k_timer *timer)
	{
#ifndef CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
		if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
			chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {
#ifdef CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT
				GroupDataProviderImpl::Instance().WillBeFactoryReseted();
				chip::Server::GetInstance().ScheduleFactoryReset();
#elif defined(CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_ONLY) ||                                                           \
	defined(CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START)
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD_SRP_CLIENT
				chip::DeviceLayer::ThreadStackMgr().ClearAllSrpHostAndServices();
#endif // CHIP_DEVICE_CONFIG_ENABLE_THREAD_SRP_CLIENT
				/* Erase Matter data */
				chip::DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().DoFactoryReset();
				/* Erase Network credentials and disconnect */
				chip::DeviceLayer::ConnectivityMgr().ErasePersistentInfo();
#ifdef CONFIG_CHIP_WIFI
				chip::DeviceLayer::WiFiManager::Instance().Disconnect();
				chip::DeviceLayer::ConnectivityMgr().ClearWiFiStationProvision();
#endif
#ifdef CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START
				/* Start the New BLE advertising */
				if (!chip::DeviceLayer::ConnectivityMgr().IsBLEAdvertisingEnabled()) {
					if (CHIP_NO_ERROR == chip::Server::GetInstance()
								     .GetCommissioningWindowManager()
								     .OpenBasicCommissioningWindow()) {
						return;
					}
				}
				ChipLogError(FabricProvisioning, "Could not start Bluetooth LE advertising");
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT
			});
		}
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
	}

	inline static k_timer sFabricRemovedTimer;
};

} /* namespace Nrf::Matter */
