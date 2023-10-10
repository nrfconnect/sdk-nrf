/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_task.h"

#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#ifdef CONFIG_CHIP_WIFI
#include <platform/nrfconnect/wifi/WiFiManager.h>
#endif

class AppFabricTableDelegate : public chip::FabricTable::Delegate
{
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
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
    }

private:
    void OnFabricRemoved(const chip::FabricTable & fabricTable, chip::FabricIndex fabricIndex)
    {
#ifndef CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
        {
#ifdef CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT
            chip::Server::GetInstance().ScheduleFactoryReset();
#elif defined(CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_ONLY) || defined(CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START)
            chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {
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
                AppEvent event;
                event.Handler = AppTask::StartBLEAdvertisementHandler;
                AppTask::Instance().PostEvent(event);
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START
            });
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_REBOOT
        }
#endif // CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE
    }
};
