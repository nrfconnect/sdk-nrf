/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "matter_init.h"

#include "app/fabric_table_delegate.h"
#include "app/group_data_provider.h"
#include "clusters/cluster_init.h"
#include "migration/migration_manager.h"

#ifdef CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL
#include "persistent_storage/persistent_storage_shell.h"
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS
#include "diagnostic/diagnostic_logs_provider.h"
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu/smp/dfu_over_smp.h"
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
#include "event_triggers/event_triggers.h"
#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS
#include "event_triggers/default_event_triggers.h"
#endif
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT
#include "app/task_executor.h"
#include "watchdog/watchdog.h"
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_SHELL
#include "test/test_shell.h"
#endif

#ifdef CONFIG_RAM_POWER_DOWN_LIBRARY
#include <ram_pwrdn.h>
#endif

#ifdef CONFIG_CHIP_STORE_KEYS_IN_KMU
#include <platform/nrfconnect/KMUKeyAllocator.h>
#endif

#ifdef CONFIG_OPENTHREAD
#include <openthread.h>
#include <platform/OpenThread/GenericNetworkCommissioningThreadDriver.h>
#endif

#include <app/InteractionModelEngine.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <data-model-providers/codegen/Instance.h>
#include <platform/nrfconnect/ExternalFlashManager.h>
#include <setup_payload/OnboardingCodesUtil.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip::DeviceLayer;
using namespace ::chip::Credentials;
using namespace ::chip::app;
using namespace ::chip;

/* Definitions of default Matter interface implementations from InitData aggregate. */
CommonCaseDeviceServerInitParams Nrf::Matter::InitData::sServerInitParamsDefault{};

#ifdef CONFIG_CHIP_WIFI
Clusters::NetworkCommissioning::Instance Nrf::Matter::InitData::sWiFiCommissioningInstance{
	0, &(NetworkCommissioning::NrfWiFiDriver::Instance())
};
#endif

#ifdef CONFIG_OPENTHREAD
app::Clusters::NetworkCommissioning::InstanceAndDriver<NetworkCommissioning::GenericThreadDriver>
	sThreadNetworkDriver(0 /*endpointId*/);
#endif

#ifdef CONFIG_CHIP_CRYPTO_PSA
chip::Crypto::PSAOperationalKeystore Nrf::Matter::InitData::sOperationalKeystoreDefault{};
#endif

#ifdef CONFIG_CHIP_STORE_KEYS_IN_KMU
chip::DeviceLayer::KMUSessionKeystore Nrf::Matter::InitData::sKMUSessionKeystoreDefault{};
#endif

#ifdef CONFIG_CHIP_FACTORY_DATA
FactoryDataProvider<InternalFlashFactoryData> Nrf::Matter::InitData::sFactoryDataProviderDefault{};
#endif

chip::DeviceLayer::DeviceInfoProviderImpl Nrf::Matter::InitData::sDeviceInfoProviderDefault{};

namespace
{
/* Local instance of the initialization data that is overwritten by an application. */
Nrf::Matter::InitData sLocalInitData{ .mNetworkingInstance = nullptr,
				      .mServerInitParams = nullptr,
				      .mDeviceInfoProvider = nullptr,
#ifdef CONFIG_CHIP_FACTORY_DATA
				      .mFactoryDataProvider = nullptr,
#endif
#ifdef CONFIG_CHIP_CRYPTO_PSA
				      .mOperationalKeyStore = nullptr,
#endif
#ifdef CONFIG_CHIP_STORE_KEYS_IN_KMU
				      .mSessionKeystore = nullptr,
#endif
				      .mPreServerInitClbk = nullptr,
				      .mPostServerInitClbk = nullptr };

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
#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT
void FeedFromApp(Nrf::Watchdog::WatchdogSource *watchdogSource)
{
	if (watchdogSource) {
		Nrf::PostTask([watchdogSource] { watchdogSource->Feed(); });
	}
}

void FeedFromMatter(Nrf::Watchdog::WatchdogSource *watchdogSource)
{
	if (watchdogSource) {
		chip::DeviceLayer::SystemLayer().ScheduleLambda([watchdogSource] { watchdogSource->Feed(); });
	}
}
#endif

/* Matter stack design implies different initialization procedure for Thread and Wi-Fi backend. */
#if defined(CONFIG_OPENTHREAD)
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

CHIP_ERROR InitNetworkingStack()
{
	CHIP_ERROR error{ CHIP_NO_ERROR };

	error = ThreadStackMgr().InitThreadStack();
	VerifyOrReturnLogError(error == CHIP_NO_ERROR, error);

	error = ConfigureThreadRole();
	VerifyOrReturnLogError(error == CHIP_NO_ERROR, error);

	sThreadNetworkDriver.Init();

	return error;
}

#if CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT
void LockOpenThreadTask(void)
{
	chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
}

void UnlockOpenThreadTask(void)
{
	chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
}
#endif /* CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT */

#elif defined(CONFIG_CHIP_WIFI)

CHIP_ERROR InitNetworkingStack()
{
	if (sLocalInitData.mNetworkingInstance) {
		sLocalInitData.mNetworkingInstance->Init();
	}

	return CHIP_NO_ERROR;
}
#else
#error "No valid networking backend selected");
#endif /* CONFIG_OPENTHREAD */

#define VerifyInitResultOrReturn(ec, msg)                                                                              \
	VerifyOrReturn(ec == CHIP_NO_ERROR, LOG_ERR(msg " [Error: %d]", sInitResult.Format()))

#define VerifyInitResultOrReturnError(ec, msg)                                                                         \
	VerifyOrReturnError(ec == CHIP_NO_ERROR, ec, LOG_ERR(msg " [Error: %d]", sInitResult.Format()))

void DoInitChipServer(intptr_t /* unused */)
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS
	uint32_t count = 0;

	if (ConfigurationMgr().GetRebootCount(count) == CHIP_NO_ERROR) {
		/* Remove diagnostic logs on the first boot, as retention RAM is not cleared during erase/factory reset.
		 */
		if (count == 1) {
			Nrf::Matter::DiagnosticLogProvider::GetInstance().ClearAllLogs();
		}

		Nrf::Matter::DiagnosticLogProvider::GetInstance().Init();
	}
#endif

	InitGuard guard;
	LOG_INF("Init CHIP stack");

	if (sLocalInitData.mPreServerInitClbk) {
		sInitResult = sLocalInitData.mPreServerInitClbk();
		VerifyInitResultOrReturn(sInitResult, "Custom pre server initialization failed");
	}

	/* Initialize networking backend. */
	sInitResult = InitNetworkingStack();
	VerifyInitResultOrReturn(sInitResult, "Cannot initialize IPv6 networking stack");

#ifdef CONFIG_CHIP_OTA_REQUESTOR
	/* OTA image confirmation must be done before the factory data init. */
	Nrf::Matter::OtaConfirmNewImage();
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	/* Initialize DFU over SMP */
	Nrf::GetDFUOverSMP().Init();
	Nrf::GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Set External Flash into sleep mode */
	ExternalFlashManager::GetInstance().DoAction(ExternalFlashManager::Action::SLEEP);

	/* Disable unused RAM blocks to reduce power consumption */
#ifdef CONFIG_RAM_POWER_DOWN_LIBRARY
	power_down_unused_ram();
#endif

	/* Initialize CHIP server */
#ifdef CONFIG_CHIP_FACTORY_DATA
	if (sLocalInitData.mFactoryDataProvider) {
		sInitResult = sLocalInitData.mFactoryDataProvider->Init();
		VerifyInitResultOrReturn(sInitResult, "FactoryDataProvider::Init() failed");
	}

	SetDeviceInstanceInfoProvider(sLocalInitData.mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(sLocalInitData.mFactoryDataProvider);
	SetCommissionableDataProvider(sLocalInitData.mFactoryDataProvider);

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
	/* Read the enable key from the factory data set */
	uint8_t enableKeyData[chip::TestEventTriggerDelegate::kEnableKeyLength] = {};
	MutableByteSpan enableKey(enableKeyData);
	sInitResult = sLocalInitData.mFactoryDataProvider->GetEnableKey(enableKey);
	VerifyInitResultOrReturn(sInitResult, "GetEnableKey() failed");
	Nrf::Matter::TestEventTrigger::Instance().SetEnableKey(enableKey);
	VerifyInitResultOrReturn(sInitResult, "SetEnableKey() failed");
#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS
	sLocalInitData.mServerInitParams->testEventTriggerDelegate = &Nrf::Matter::TestEventTrigger::Instance();
	Nrf::Matter::DefaultTestEventTriggers::Register();
#endif /* CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS */
#endif /* CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS */
#else
	SetDeviceInstanceInfoProvider(&DeviceInstanceInfoProviderMgrImpl());
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
	/* The default CommissionableDataProvider is set internally in the GenericConfigurationManagerImpl::Init(). */
#endif

#if CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT
	// Set up OpenThread configuration when OpenThread is included
	chip::Inet::EndPointStateOpenThread::OpenThreadEndpointInitParam nativeParams;
	nativeParams.lockCb = LockOpenThreadTask;
	nativeParams.unlockCb = UnlockOpenThreadTask;
	nativeParams.openThreadInstancePtr = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
	sLocalInitData.mServerInitParams->endpointNativeParams = static_cast<void *>(&nativeParams);
#endif /* CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT */

#ifdef CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL
	VerifyOrReturn(Nrf::PersistentStorageShell::Init(),
		       LOG_ERR("Matter settings shell has been enabled, but it cannot be initialized."));
#endif

#ifdef CONFIG_CHIP_CRYPTO_PSA
	sLocalInitData.mServerInitParams->operationalKeystore = sLocalInitData.mOperationalKeyStore;
#endif

/* Set KMUKeyAllocator for devices that supports KMU */
#ifdef CONFIG_CHIP_STORE_KEYS_IN_KMU
	static KMUKeyAllocator kmuAllocator;
	Crypto::SetPSAKeyAllocator(&kmuAllocator);
	sLocalInitData.mServerInitParams->sessionKeystore = sLocalInitData.mSessionKeystore;
#endif

	VerifyOrReturn(sLocalInitData.mServerInitParams, LOG_ERR("No valid server initialization parameters"));
	sInitResult = sLocalInitData.mServerInitParams->InitializeStaticResourcesBeforeServerInit();
	VerifyInitResultOrReturn(sInitResult, "InitializeStaticResourcesBeforeServerInit() failed");

	/* Inject Nordic specific group data provider that allows for optimization of factory reset. */
	Nrf::Matter::GroupDataProviderImpl::Instance().SetStorageDelegate(
		sLocalInitData.mServerInitParams->persistentStorageDelegate);
	Nrf::Matter::GroupDataProviderImpl::Instance().SetSessionKeystore(
		sLocalInitData.mServerInitParams->sessionKeystore);
	sInitResult = Nrf::Matter::GroupDataProviderImpl::Instance().Init();
	VerifyInitResultOrReturn(sInitResult, "Initialization of GroupDataProvider failed");
	sLocalInitData.mServerInitParams->groupDataProvider = &Nrf::Matter::GroupDataProviderImpl::Instance();

	sInitResult = PlatformMgr().AddEventHandler(sLocalInitData.mEventHandler, 0);
	VerifyInitResultOrReturn(sInitResult, "Cannot register CHIP event handler");

	SetDeviceInfoProvider(sLocalInitData.mDeviceInfoProvider);

	sLocalInitData.mServerInitParams->dataModelProvider =
		app::CodegenDataModelProviderInstance(sLocalInitData.mServerInitParams->persistentStorageDelegate);

	sInitResult = Server::GetInstance().Init(*sLocalInitData.mServerInitParams);
	VerifyInitResultOrReturn(sInitResult, "Server::Init() failed");

#ifdef CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS
	sInitResult = Nrf::Matter::Migration::MoveOperationalKeysFromKvsToIts(
		sLocalInitData.mServerInitParams->persistentStorageDelegate,
		sLocalInitData.mServerInitParams->operationalKeystore);
	VerifyInitResultOrReturn(sInitResult, "MoveOperationalKeysFromKvsToIts() failed");
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT
	/* Create and start Watchdog objects for Main and Matter threads */
	static Nrf::Watchdog::WatchdogSource sAppWatchdog(CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME,
							  FeedFromApp);
	static Nrf::Watchdog::WatchdogSource sMatterWatchdog(CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME,
							     FeedFromMatter);

	if (!Nrf::Watchdog::InstallSource(sAppWatchdog)) {
		sInitResult = CHIP_ERROR_INTERNAL;
		VerifyInitResultOrReturn(sInitResult, "Cannot install Application watchdog source");
	}

	if (!Nrf::Watchdog::InstallSource(sMatterWatchdog)) {
		sInitResult = CHIP_ERROR_INTERNAL;
		VerifyInitResultOrReturn(sInitResult, "Cannot install Matter watchdog source");
	}
#endif

	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
	Nrf::Matter::AppFabricTableDelegate::Init();

	if (sLocalInitData.mPostServerInitClbk) {
		sInitResult = sLocalInitData.mPostServerInitClbk();
		VerifyInitResultOrReturn(sInitResult, "Custom post server initialization failed");
	}

#ifdef CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT
	sInitResult = Nrf::Watchdog::Enable() ? CHIP_NO_ERROR : CHIP_ERROR_INTERNAL;
	VerifyInitResultOrReturn(sInitResult, "Cannot enable global Watchdog");
#endif
}

CHIP_ERROR WaitForReadiness()
{
	InitGuard::Wait();
	return sInitResult;
}
} // namespace

/* Public API */
namespace Nrf::Matter
{
CHIP_ERROR PrepareServer(const InitData &initData)
{
	sLocalInitData = initData;

	/* Before we schedule anything to execute in the CHIP thread, the platform memory
	   and the stack itself must be initialized first. */
	CHIP_ERROR err = Platform::MemoryInit();
	VerifyInitResultOrReturnError(err, "Platform::MemoryInit() failed");
	err = PlatformMgr().InitChipStack();
	VerifyInitResultOrReturnError(err, "PlatformMgr().InitChipStack() failed");

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_SHELL
	Nrf::RegisterTestCommands();
#endif

	/* Schedule all CHIP initializations to the CHIP thread for better synchronization. */
	return PlatformMgr().ScheduleWork(DoInitChipServer, 0);
}

CHIP_ERROR StartServer()
{
	CHIP_ERROR err = PlatformMgr().StartEventLoopTask();
	VerifyInitResultOrReturnError(err, "PlatformMgr().StartEventLoopTask() failed");

	/* Wait for the CHIP server to be initialized. */
	err = WaitForReadiness();
	VerifyInitResultOrReturnError(err, "CHIP server initialization failed");

	/* Run all code-driven registered cluster initialization callbacks. */
	if (!nrf_matter_cluster_init_run_all()) {
		return CHIP_ERROR_INTERNAL;
	}

	return CHIP_NO_ERROR;
}

#ifdef CONFIG_CHIP_FACTORY_DATA
FactoryDataProviderBase *GetFactoryDataProvider()
{
	return sLocalInitData.mFactoryDataProvider;
}
#endif

PersistentStorageDelegate *GetPersistentStorageDelegate()
{
	return sLocalInitData.mServerInitParams->persistentStorageDelegate;
}

} /* namespace Nrf::Matter */
