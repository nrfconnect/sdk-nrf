/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "thread_wifi_switch.h"

#include <platform/CHIPDeviceLayer.h>

#include <app/clusters/network-commissioning/network-commissioning.h>
#include <lib/dnssd/Discovery_ImplPlatform.h>
#include <lib/support/CodeUtils.h>
#include <platform/nrfconnect/wifi/NrfWiFiDriver.h>
#include <platform/nrfconnect/Reboot.h>
#include <platform/OpenThread/GenericNetworkCommissioningThreadDriver.h>
#include <system/SystemError.h>

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>

#define SETTINGS_SUBTREE "tws"
#define SETTINGS_CURRENT_TRANSPORT "t"

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::DeviceLayer;

namespace
{
bool gIsThreadActive = false;

NetworkCommissioning::GenericThreadDriver sThreadDriver;
app::Clusters::NetworkCommissioning::Instance sThreadCommissioning(0, &sThreadDriver);
app::Clusters::NetworkCommissioning::Instance sWiFiCommissioning(0, &NetworkCommissioning::NrfWiFiDriver::Instance());
} /* namespace */

namespace ThreadWifiSwitch
{
bool IsThreadActive()
{
	return gIsThreadActive;
}

CHIP_ERROR StartCurrentTransport()
{
	int rc = settings_load_subtree(SETTINGS_SUBTREE);
	VerifyOrReturnError(rc == 0, System::MapErrorZephyr(rc));

	LOG_INF("Starting Matter over %s", IsThreadActive() ? "Thread" : "Wi-Fi");

	if (IsThreadActive()) {
		ReturnErrorOnFailure(sThreadCommissioning.Init());
		Dnssd::Resolver::SetInstance(Dnssd::DiscoveryImplPlatform::GetInstance());
		Dnssd::ServiceAdvertiser::SetInstance(Dnssd::DiscoveryImplPlatform::GetInstance());
	} else {
		ReturnErrorOnFailure(sWiFiCommissioning.Init());
		/* No need to change DNS-SD implementation - minimal mDNS is the default. */
	}

	return CHIP_NO_ERROR;
}

void SwitchTransport()
{
	CHIP_ERROR error = SystemLayer().ScheduleLambda([] {
		bool threadActive = !IsThreadActive();
		settings_save_one(SETTINGS_SUBTREE "/" SETTINGS_CURRENT_TRANSPORT, &threadActive, sizeof(threadActive));

		PlatformMgr().HandleServerShuttingDown();
		ConfigurationMgr().InitiateFactoryReset();
	});

	if (error != CHIP_NO_ERROR) {
		LOG_ERR("Failed to schedule switching to %s: %" CHIP_ERROR_FORMAT,
			IsThreadActive() ? "Wi-Fi" : "Thread", error.Format());
	}
}
} /* namespace ThreadWifiSwitch */

static int ThreadWifiSwitchSettingHandler(const char *key, size_t size, settings_read_cb readCb, void *readCbArg)
{
	VerifyOrReturnError(strcmp(key, SETTINGS_CURRENT_TRANSPORT) == 0, -ENOENT);
	VerifyOrReturnError(size == sizeof(gIsThreadActive), -EINVAL);

	const ssize_t rc = readCb(readCbArg, &gIsThreadActive, size);
	VerifyOrReturnError(rc > 0, static_cast<int>(rc));

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(thread_wifi_switch, SETTINGS_SUBTREE, NULL, ThreadWifiSwitchSettingHandler, NULL, NULL);

#ifdef CONFIG_THREAD_WIFI_SWITCHING_SHELL

static int ExecSwitchTransport(const shell *shell, size_t argc, char **argv)
{
	VerifyOrReturnError(argc == 1, -EINVAL);

	shell_print(shell, "Switching to %s", ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");
	ThreadWifiSwitch::SwitchTransport();

	return 0;
}

SHELL_CMD_ARG_REGISTER(switch_transport, NULL, "Switch to Thread or Wi-Fi", ExecSwitchTransport, 1, 0);

#endif /* CONFIG_THREAD_WIFI_SWITCHING_SHELL */
