/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "watchdog/watchdog.h"

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace Nrf::Watchdog;

namespace
{
const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
bool sEnabled = false;
sys_slist_t sWatchdogList;
} // namespace

bool Nrf::Watchdog::Enable()
{
	if (sEnabled) {
		return false;
	}

	/* Enable the global timer only of there is at least one source assigned */
	if (sys_slist_is_empty(&sWatchdogList)) {
		LOG_ERR("No watchdog sources installed");
		return false;
	}

	if (wdt_setup(wdt, WatchdogSource::kWatchdogOptions) != 0) {
		return false;
	}

	LOG_DBG("Global watchdog started with timeout %d ms", CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT);
	sEnabled = true;
	return true;
}

bool Nrf::Watchdog::Disable()
{
	if (!sEnabled) {
		return false;
	}

	/* Disable the global timer and remove all assigned channels */
	if (wdt_disable(wdt) != 0) {
		return false;
	}

	sEnabled = false;
	return true;
}

bool Nrf::Watchdog::InstallSource(WatchdogSource &source)
{
	/* Check whether the watchdog peripheral is enabled and ready */
	if (!device_is_ready(wdt)) {
		return false;
	}

	/* Check whether the new source can be created */
	if (sys_slist_len(&sWatchdogList) == kMaximumAvailableSources) {
		return false;
	}

	/* Stop the Global Watchdog before further operations */
	bool startAutomatically = false;
	if (sEnabled) {
		if (!Disable()) {
			return false;
		}
		/* Watchdog was enabled so after doing all opreations for adding a new source start it automatically */
		startAutomatically = true;

		/* If the watchdog was enabled previously, restore all disabled sources */
		sys_snode_t *node;
		SYS_SLIST_FOR_EACH_NODE (&sWatchdogList, node) {
			/* Restore all other Watchdog sources from the list */
			static_cast<WatchdogSource *>(node)->Install();
		}
	}

	/* Try to install a new source */
	if (source.Install()) {
		/* Register timer in the list */
		sys_slist_append(&sWatchdogList, &source);

		/* Timer for feeding should be initialized only if feeding callback has been provided */
		if (source.mCallback) {
			k_timer_init(&source.mTimer, &WatchdogSource::TimerTimeoutCallback, nullptr);
			k_timer_user_data_set(&source.mTimer, &source);
			k_timer_start(&source.mTimer, K_MSEC(source.mFeedingTime), K_MSEC(source.mFeedingTime));
		}

		LOG_DBG("Registered a watchdog source %d with feed interval %u ms", source.mChannel,
			source.mFeedingTime);
	}

	if (startAutomatically) {
		return Enable();
	}

	return true;
}

void WatchdogSource::Feed()
{
	if (sEnabled) {
		if (mChannel == kInvalidChannel) {
			LOG_ERR("Call for Watchdog FEED, but the watchdog has not been initialized");
		}

		wdt_feed(wdt, mChannel);
	}
}

bool WatchdogSource::Install()
{
	if (sEnabled) {
		return false;
	}

	/* Install a new timeout */
	wdt_timeout_cfg wdtConfig = {};
	/* Reset SoC when watchdog timer expires. */
	wdtConfig.flags = WDT_FLAG_RESET_SOC;
	/* The minimum window time must be 0 */
	wdtConfig.window.min = 0;
	/* The maximum window time must be equal for all channels */
	wdtConfig.window.max = CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT;
	/* The callback has no sense, because watchdog interrupt takes only ~64 us and CPU
	 * cannot do anything useful */
	wdtConfig.callback = nullptr;

	mChannel = wdt_install_timeout(wdt, &wdtConfig);
	if (-ENOMEM == mChannel) {
		LOG_ERR("Cannot create a new Watchdog source because there is no more available Watchdog channels");
		return false;
	}

	if (mChannel < 0) {
		return false;
	}

	return true;
}

void WatchdogSource::TimerTimeoutCallback(k_timer *timer)
{
	if (sEnabled) {
		WatchdogSource *watchdog = reinterpret_cast<WatchdogSource *>(k_timer_user_data_get(timer));
		// Feeding using timer only of callback is set
		if (watchdog && watchdog->mCallback) {
			watchdog->mCallback(watchdog);
		}
	}
}

WatchdogSource::~WatchdogSource()
{
	/* Stop the global timer */
	Disable();

	/* Remove this object from the list and stop its timer if it was enabled */
	sys_slist_find_and_remove(&sWatchdogList, this);
	k_timer_stop(&mTimer);

	/* Restore all other Watchdog sources from the list */
	sys_snode_t *node, *tmpNodeSafe;
	SYS_SLIST_FOR_EACH_NODE_SAFE (&sWatchdogList, node, tmpNodeSafe) {
		static_cast<WatchdogSource *>(node)->Install();
	}

	/* Enable the global timer */
	Enable();
}
