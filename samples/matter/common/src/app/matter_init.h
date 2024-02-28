/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app/matter_event_handler.h"

#include <DeviceInfoProviderImpl.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/Server.h>
#include <lib/core/Optional.h>
#include <lib/support/Variant.h>
#include <platform/PlatformManager.h>

#ifdef CONFIG_CHIP_WIFI
#include <platform/nrfconnect/wifi/NrfWiFiDriver.h>
#endif

#ifdef CONFIG_CHIP_CRYPTO_PSA
#include <crypto/PSAOperationalKeystore.h>
#endif

#ifdef CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#else
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
#endif

namespace Nrf::Matter
{
using CustomInit = CHIP_ERROR (*)(void);

/** @brief Matter initialization data.
 *
 * This structure contains all user specific implementations of Matter interfaces
 * and custom initialization callbacks that must be initialized in the Matter thread.
 */
struct InitData {
	/** @brief Matter stack events handler. */
	chip::DeviceLayer::PlatformManager::EventHandlerFunct mEventHandler{ DefaultEventHandler };
	/** @brief Pointer to the user provided NetworkCommissioning instance. */
#ifdef CONFIG_CHIP_WIFI
	chip::app::Clusters::NetworkCommissioning::Instance *mNetworkingInstance{ &sWiFiCommissioningInstance };
#else
	chip::app::Clusters::NetworkCommissioning::Instance *mNetworkingInstance{ nullptr };
#endif
	/** @brief Pointer to the user provided custom server initialization parameters. */
	chip::CommonCaseDeviceServerInitParams *mServerInitParams{ &sServerInitParamsDefault };
	/** @brief Pointer to the user provided custom device info provider implementation. */
	chip::DeviceLayer::DeviceInfoProviderImpl *mDeviceInfoProvider{ nullptr };
#ifdef CONFIG_CHIP_FACTORY_DATA
	/** @brief Pointer to the user provided FactoryDataProvider implementation. */
	chip::DeviceLayer::FactoryDataProviderBase *mFactoryDataProvider{ &sFactoryDataProviderDefault };
#endif
#ifdef CONFIG_CHIP_CRYPTO_PSA
	/** @brief Pointer to the user provided OperationalKeystore implementation. */
	chip::Crypto::OperationalKeystore *mOperationalKeyStore{ &sOperationalKeystoreDefault };
#endif
	/** @brief Custom code to execute in the Matter main event loop before the server initialization. */
	CustomInit mPreServerInitClbk{ nullptr };
	/** @brief Custom code to execute in the Matter main event loop after the server initialization. */
	CustomInit mPostServerInitClbk{ nullptr };

	/** @brief Default implementation static objects that will be stripped by the compiler when above
	 * pointers are overwritten by the application. */
#ifdef CONFIG_CHIP_WIFI
	static chip::app::Clusters::NetworkCommissioning::Instance sWiFiCommissioningInstance;
#endif
	static chip::CommonCaseDeviceServerInitParams sServerInitParamsDefault;
#ifdef CONFIG_CHIP_FACTORY_DATA
	static chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData>
		sFactoryDataProviderDefault;
#endif
#ifdef CONFIG_CHIP_CRYPTO_PSA
	static chip::Crypto::PSAOperationalKeystore sOperationalKeystoreDefault;
#endif
};

/**
 * @brief Prepare Matter server.
 *
 * This function schedules initialization of all Matter server components including memory, networking backend
 * and factory data in the Matter thread. After this function is used, the StartServer() must be called to
 * start the Matter thread, eventually execute the initialization and wait to synchronize the caller's thread
 * with the Matter thread.
 *
 * If the initData argument is not provided, the default configuration is applied, including the default Matter
 * events handler.
 *
 * @param initData Matter initialization data
 * @return CHIP_NO_ERROR on success, other error code otherwise
 */
CHIP_ERROR PrepareServer(const InitData &initData = InitData{});

/**
 * @brief Start Matter server and wait until it is initialized.
 *
 * This is a blocking function which starts the Matter event loop task (CHIP thread)
 * and waits until all Matter server components are initialized.
 *
 * @return CHIP_NO_ERROR on success, other error code otherwise
 */
CHIP_ERROR StartServer();

#ifdef CONFIG_CHIP_FACTORY_DATA
/**
 * @brief Get the currently set FactoryDataProvider.
 *
 * Returns generic pointer to the FactoryDataProvider object that was
 * either set externally by the user or internally by default initialization.
 *
 * @return pointer to the stored FactoryDataProviderBase
 */
chip::DeviceLayer::FactoryDataProviderBase *GetFactoryDataProvider();
#endif

} /* namespace Nrf::Matter */
