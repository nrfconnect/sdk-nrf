/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "matter_init.h"

#include "fabric_table_delegate.h"

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip::DeviceLayer;
using namespace ::chip::Credentials;
using namespace ::chip::app;
using namespace ::chip;

/* Definitions of default Matter interface implementations from InitData aggregate. */
CommonCaseDeviceServerInitParams Nordic::Matter::InitData::sServerInitParamsDefault{};

#ifdef CONFIG_CHIP_WIFI
Clusters::NetworkCommissioning::Instance Nordic::Matter::InitData::sWiFiCommissioningInstance{
	0, &(NetworkCommissioning::NrfWiFiDriver::Instance())
};
#endif

#if CONFIG_CHIP_FACTORY_DATA
FactoryDataProvider<InternalFlashFactoryData> Nordic::Matter::InitData::sDefaultFactoryDataProvider{};
#endif
namespace
{
/* Local instance of the initialization data that is overwritten by an application. */
Nordic::Matter::InitData sLocalInitData
{
	.mNetworkingInstance = nullptr, .mServerInitParams = nullptr, .mDeviceInfoProvider = nullptr,
#if CONFIG_CHIP_FACTORY_DATA
	.mFactoryDataProvider = nullptr,
#endif
	.mPreServerInitClbk = nullptr, .mPostServerInitClbk = nullptr
};

/* Synchronization primitives */
K_MUTEX_DEFINE(sInitMutex);
K_CONDVAR_DEFINE(sInitCondVar);
CHIP_ERROR sInitResult;
bool sInitDone{ false };

/* RAII utility to implement automatic cleanup and signalling. */
struct InitGuard {
	InitGuard()
	{
		sInitDone = false;
		k_mutex_lock(&sInitMutex, K_FOREVER);
	}
	~InitGuard()
	{
		sInitDone = true;
		k_condvar_signal(&sInitCondVar);
		k_mutex_unlock(&sInitMutex);
	}

	static void Wait()
	{
		k_mutex_lock(&sInitMutex, K_FOREVER);
		if (!sInitDone) {
			k_condvar_wait(&sInitCondVar, &sInitMutex, K_FOREVER);
		}
		k_mutex_unlock(&sInitMutex);
	}
};

/* Local helper functions. */
#if defined(CONFIG_NET_L2_OPENTHREAD)
CHIP_ERROR ConfigureThreadRole()
{
	using ThreadRole = ConnectivityManager::ThreadDeviceType;

	ThreadRole threadRole{ ThreadRole::kThreadDeviceType_MinimalEndDevice };
#ifdef CONFIG_OPENTHREAD_MTD_SED
#ifdef CONFIG_CHIP_THREAD_SSED
	threadRole = ThreadRole::kThreadDeviceType_SynchronizedSleepyEndDevice;
#else
	threadRole = ThreadRole::kThreadDeviceType_SleepyEndDevice;
#endif /* CONFIG_CHIP_THREAD_SSED */
#elif defined(CONFIG_OPENTHREAD_FTD)
	threadRole = ThreadRole::kThreadDeviceType_Router;
#endif /* CONFIG_OPENTHREAD_MTD_SED */

	return ConnectivityMgr().SetThreadDeviceType(threadRole);
}
#endif /* CONFIG_NET_L2_OPENTHREAD */

#define VerifyInitResultOrReturn(ec, msg)                                                                              \
	VerifyOrReturn(ec == CHIP_NO_ERROR, LOG_ERR(msg " [Error: %d]", sInitResult.Format()))

#define VerifyInitResultOrReturnError(ec, msg)                                                                         \
	VerifyOrReturnError(ec == CHIP_NO_ERROR, ec, LOG_ERR(msg " [Error: %d]", sInitResult.Format()))

void DoInitChipServer(intptr_t arg)
{
	InitGuard guard;
	LOG_INF("Init CHIP stack");

	if (sLocalInitData.mPreServerInitClbk) {
		sInitResult = sLocalInitData.mPreServerInitClbk();
		VerifyInitResultOrReturn(sInitResult, "Custom pre server initialization failed");
	}

#if defined(CONFIG_NET_L2_OPENTHREAD)
	sInitResult = ThreadStackMgr().InitThreadStack();
	VerifyInitResultOrReturn(sInitResult, "ThreadStackMgr().InitThreadStack() failed");

	sInitResult = ConfigureThreadRole();
	VerifyInitResultOrReturn(sInitResult, "Cannot configure Thread role");

#elif defined(CONFIG_CHIP_WIFI)
	if (!sLocalInitData.mNetworkingInstance) {
		sInitResult = CHIP_ERROR_INTERNAL;
		VerifyInitResultOrReturn(sInitResult, "No valid commissioning instance");
	}
	sLocalInitData.mNetworkingInstance->Init();
#else
	sInitResult = CHIP_ERROR_INTERNAL;
	VerifyInitResultOrReturn(sInitResult, "No valid L2 network backend selected");
#endif /* CONFIG_NET_L2_OPENTHREAD */

#ifdef CONFIG_CHIP_OTA_REQUESTOR
	/* OTA image confirmation must be done before the factory data init. */
	OtaConfirmNewImage();
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init();
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize CHIP server */
#if CONFIG_CHIP_FACTORY_DATA
	if (sLocalInitData.mFactoryDataProvider) {
		sInitResult = sLocalInitData.mFactoryDataProvider->Init();
		VerifyInitResultOrReturn(sInitResult, "FactoryDataProvider::Init() failed");
	}

	SetDeviceInstanceInfoProvider(sLocalInitData.mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(sLocalInitData.mFactoryDataProvider);
	SetCommissionableDataProvider(sLocalInitData.mFactoryDataProvider);
#else
	SetDeviceInstanceInfoProvider(&DeviceInstanceInfoProviderMgrImpl());
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
	/* The default CommissionableDataProvider is set internally in the GenericConfigurationManagerImpl::Init(). */
#endif

	VerifyOrReturn(sLocalInitData.mServerInitParams, LOG_ERR("No valid server initialization parameters"));
	sInitResult = sLocalInitData.mServerInitParams->InitializeStaticResourcesBeforeServerInit();
	VerifyInitResultOrReturn(sInitResult, "InitializeStaticResourcesBeforeServerInit() failed");

	sInitResult = PlatformMgr().AddEventHandler(reinterpret_cast<PlatformManager::EventHandlerFunct>(arg), 0);
	VerifyInitResultOrReturn(sInitResult, "Cannot register CHIP event handler");

	SetDeviceInfoProvider(sLocalInitData.mDeviceInfoProvider);

	sInitResult = Server::GetInstance().Init(*sLocalInitData.mServerInitParams);
	VerifyInitResultOrReturn(sInitResult, "Server::Init() failed");

	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
	AppFabricTableDelegate::Init();

	if (sLocalInitData.mPostServerInitClbk) {
		sInitResult = sLocalInitData.mPostServerInitClbk();
		VerifyInitResultOrReturn(sInitResult, "Custom post server initialization failed");
	}
}

CHIP_ERROR WaitForReadiness()
{
	InitGuard::Wait();
	return sInitResult;
}
} // namespace

/* Public API */
namespace Nordic
{
namespace Matter
{
	CHIP_ERROR PrepareServer(PlatformManager::EventHandlerFunct matterEventHandler, const InitData &initData)
	{
		sLocalInitData = initData;

		/* Before we schedule anything to execute in the CHIP thread, the platform memory
		   and the stack itself must be initialized first. */
		CHIP_ERROR err = Platform::MemoryInit();
		VerifyInitResultOrReturnError(err, "Platform::MemoryInit() failed");
		err = PlatformMgr().InitChipStack();
		VerifyInitResultOrReturnError(err, "PlatformMgr().InitChipStack() failed");

		/* Schedule all CHIP initializations to the CHIP thread for better synchronization. */
		return PlatformMgr().ScheduleWork(DoInitChipServer, reinterpret_cast<intptr_t>(matterEventHandler));
	}

	CHIP_ERROR StartServer()
	{
		CHIP_ERROR err = PlatformMgr().StartEventLoopTask();
		VerifyInitResultOrReturnError(err, "PlatformMgr().StartEventLoopTask() failed");
		return WaitForReadiness();
	}

#if CONFIG_CHIP_FACTORY_DATA
	FactoryDataProviderBase *GetFactoryDataProvider()
	{
		return sLocalInitData.mFactoryDataProvider;
	}
#endif

} // namespace Matter
} // namespace Nordic
