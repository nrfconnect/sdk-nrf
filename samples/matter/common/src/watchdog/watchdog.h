/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

namespace Nrf
{

/**
 * @brief Watchdog driver wrapper that can be use to detect code blocking and infinite loops
 *
 * It contains a possibility to create a different Watchdog sources that must be fed within the defined timeout.
 *
 * To start the global Watchdog use the Enable() method.
 * This enables and starts all Watchdog source counters and from this point all Watchdog sources must be fed
 * within their feeding windows.
 *
 * Watchdog sources cannot be disabled separately after enabling the Global Watchdog, but you can disable all of
 * them at once using the Disable() static method. After that to re-enable Watchdog sources you need to call
 * Restore() method on each of them and call Enable() method to start the Global Timer once again.
 *
 * Similarly to creating the objects, you must call Restore() method before calling the Enable() method.
 */
namespace Watchdog
{
	/* The maximum available sources is limited by the hardware watchdog */
	constexpr static size_t kMaximumAvailableSources = 8;

	/**
	 * @brief WatchdogSource class dedicated to detect code blocking and infinite loops.
	 *
	 * Watchdog source uses the watchdog peripheral source channel, which must be fed periodically if installed.
	 * Each watchdog source handles works separately and must be fed in a application-specific way.
	 *
	 * If at least one of the created watchdog sources is not fed in the defined time window then the reboot
	 * will occur.
	 *
	 * The watchdog source can be used for a limited time by creating an new object and removing it after it is not
	 * needed anymore. It allows testing the long loops within the specific functions.
	 *
	 * You can register up to 8 Watchdog sources.
	 *
	 * You can feed a Watchdog source in a two ways:
	 * 	- Manually, by calling the Feed() method in a specific context.
	 *	- Automatically, by providing callback function and feed interval value to the constructor as an
	 *    argument. If the callback is provided it will be called in each feed interval time periodically.
	 */
	class WatchdogSource : public sys_snode_t {
	public:
		/**
		 * @brief Callback for feeding purposes
		 *
		 * This callback will be run when feedingInterval timeout occurs.
		 * It must call the Feed method within the specific context.
		 * If this callback cannot be called within the CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT timeout the
		 * reboot occurs.
		 *
		 * For example to feed a Watchdog source dedicated for Main thread
		 * you can use Nrf::PostTask method:
		 *
		 *  Nrf::PostTask([watchdog] { watchdog->Feed(); });
		 */
		using FeedingCallback = void (*)(WatchdogSource *watchdogSource);

		/**
		 * @brief Create new Watchdog feeding source
		 *
		 * Each Watchdog source may contain a callback which is called when the feedingInterval time is elapsed.
		 * If at least one of the watchdog source is not fed within the
		 * CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT then the reboot occurs.
		 *
		 * @param feedingInterval if the callback argument is provided this value is used as an interval [ms]
		 * between subsequent callback occurrences.
		 * @param callback FeedingCallback to be called in the specific place to reset the Watchdog timer
		 */
		WatchdogSource(uint32_t feedingInterval = 0, FeedingCallback callback = nullptr)
			: mFeedingTime(feedingInterval), mCallback(callback)
		{
		}

		/**
		 * @brief Remove the Watchdog source
		 *
		 * The destructor performs a several steps to remove a WatchdogSource:
		 * 1. Disables the Watchdog peripheral.
		 * 2. Removes the Watchdog source.
		 * 3. Restores all other Watchdog sources that are already registered.
		 * 4. Enables the Watchdog peripheral.
		 */
		~WatchdogSource();

		/**
		 * @brief Reset the Watchdog timer from a specific source.
		 *
		 * If the Watchdog peripheral doesn't receive feed signal within the
		 * CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT then the device reboot occurs.
		 *
		 * @note This method can be called within various context, wherever you want to detect high time usage,
		 * or infinite loops. For example you can call this method within a specific thread.
		 */
		void Feed();

		/**
		 * @brief Restore the Watchdog source
		 *
		 * This method must be called in order to re-enable a Watchdog source if the Enable() static method has
		 * been called.
		 *
		 * @warning This method must be called before calling Enable() if the Disable function has been called.
		 *
		 * @return true if a Watchdog source has been restored and started working again.
		 * @return false if the Global Watchdog is in the state that disallows restoring the Watchdog source.
		 */
		bool Install();

		friend bool Enable();
		friend bool Disable();
		friend bool InstallSource(WatchdogSource &source);

	private:
		constexpr static uint8_t kInvalidChannel = -1;

		/* Pause Watchdog when the CPU is in sleep state or is halted by the debugger */
		constexpr static uint8_t kWatchdogOptions = WDT_OPT_PAUSE_HALTED_BY_DBG | WDT_OPT_PAUSE_IN_SLEEP;

		/* Timer for feeding in the required interval */
		static void TimerTimeoutCallback(k_timer *timer);

		/* Channel number */
		int mChannel = kInvalidChannel;
		/* State of the Source initialization */
		bool mIsInitialized = false;
		/* Feeding Time, Timer instance, and callback  to be called to feed a Watchdog source */
		uint32_t mFeedingTime;
		k_timer mTimer;
		FeedingCallback mCallback = nullptr;
	};

	/**
	 * @brief Enable the global Watchdog.
	 *
	 * @warning This method must be called after creating all Watchdog sources.
	 * 			Calling it before initializing the next Watchdog source will cause an assert.
	 *
	 * This method enables the global Watchdog and starts it.
	 *
	 * @return true if the global Watchdog timer has been started.
	 * @return false if the global Watchdog timer has not been started.
	 */
	bool Enable();

	/**
	 * @brief Disable the global Watchdog.
	 *
	 * This method stops and disables the Global Watchdog and removes all assigned channels.
	 * It means that in order to re-enable Watchdog sources after calling this method you need to call
	 * Restore() method on all Watchdog sources and call Enable() static method to start the Global
	 * Watchdog.
	 *
	 * @return true if the global Watchdog timer has been started.
	 * @return false if the global Watchdog timer has not been started.
	 */
	bool Disable();

	/**
	 * @brief Install a new watchdog source
	 *
	 * A new source can be installed any time and it exists as long as the origin object exists.
	 * So to uninstall the source, call its destructor.
	 *
	 * @param source Watchdog Source object
	 * @return true If source has been installed
	 * @return false If source cannot be installed due to installation error, or there is no available watchdog
	 * channel.
	 */
	bool InstallSource(WatchdogSource &source);

} // namespace Watchdog
} /* namespace Nrf */
