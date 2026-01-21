/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app/task_executor.h"
#include "board/board.h"

#include <app/DefaultTimerDelegate.h>
#include <app/clusters/identify-server/IdentifyCluster.h>
#include <app/server-cluster/ServerClusterInterfaceRegistry.h>
#include <data-model-providers/codegen/CodegenDataModelProvider.h>
#include <lib/support/TimerDelegate.h>

#include <lib/support/logging/CHIPLogging.h>

#include <functional>

namespace Nrf
{
namespace Matter
{
	/**
	 * @brief Implementation of the Identify delegate for nRF development kits.
	 *
	 * This class is used to implement the Identify delegate for nRF development
	 * kits. It is used to blink the LED2 on an nRF development kit.
	 *
	 * This class is designed to be used as a base class for custom identify
	 * delegate implementations. Derived classes can override the virtual methods
	 * to customize the identify behavior.
	 *
	 * @note By default, trigger effects are disabled. Use the constructor
	 * parameter or SetTriggerEffectEnabled() to enable them.
	 */
	class IdentifyDelegateImplNrf : public chip::app::Clusters::IdentifyDelegate {
	public:
		/**
		 * @brief Construct a new IdentifyDelegateImplNrf object.
		 *
		 * @param isTriggerEffectEnabled Whether trigger effects are enabled.
		 *                               Defaults to false.
		 */
		explicit IdentifyDelegateImplNrf(bool isTriggerEffectEnabled = false,
						 std::function<void()> customIdentifyStopCallback = nullptr)
			: mIsTriggerEffectEnabled(isTriggerEffectEnabled),
			  mCustomIdentifyStopCallback(customIdentifyStopCallback)
		{
		}

		void OnIdentifyStart(chip::app::Clusters::IdentifyCluster &cluster) override
		{
			Nrf::PostTask([] {
				Nrf::GetBoard()
					.GetLED(Nrf::DeviceLeds::LED2)
					.Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms);
			});
		}

		void OnIdentifyStop(chip::app::Clusters::IdentifyCluster &cluster) override
		{
			if (mCustomIdentifyStopCallback) {
				mCustomIdentifyStopCallback();
			} else {
				Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
			}
		}

		void OnTriggerEffect(chip::app::Clusters::IdentifyCluster &cluster) override
		{
			switch (cluster.GetEffectIdentifier()) {
			case chip::app::Clusters::Identify::EffectIdentifierEnum::kBlink:
			case chip::app::Clusters::Identify::EffectIdentifierEnum::kBreathe:
				Nrf::PostTask([] {
					Nrf::GetBoard()
						.GetLED(Nrf::DeviceLeds::LED2)
						.Blink(Nrf::LedConsts::kTriggerEffectStart_ms,
						       Nrf::LedConsts::kTriggerEffectFinish_ms);
				});
				break;
			case chip::app::Clusters::Identify::EffectIdentifierEnum::kOkay:
			case chip::app::Clusters::Identify::EffectIdentifierEnum::kChannelChange:
				Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(true); });
				break;
			case chip::app::Clusters::Identify::EffectIdentifierEnum::kFinishEffect:
			case chip::app::Clusters::Identify::EffectIdentifierEnum::kStopEffect:
				if (mCustomIdentifyStopCallback) {
					mCustomIdentifyStopCallback();
				} else {
					Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
				}
				break;
			default:
				return;
			}
		}

		bool IsTriggerEffectEnabled() const override { return mIsTriggerEffectEnabled; }

	protected:
		bool mIsTriggerEffectEnabled = false;
		std::function<void()> mCustomIdentifyStopCallback;
	};

	/**
	 * @brief Identify Cluster implementation for nRF Connect SDK.
	 *
	 */
	class IdentifyCluster {
	public:
		/**
		 * @brief Construct a new Identify Cluster with a custom timer delegate and identify delegate.
		 *
		 * @param endpoint The endpoint of the identify cluster
		 * @param identifyDelegate Custom identify delegate to use.
		 * @param timerDelegate Custom timer delegate to use.
		 * @param identifyType The type of identify to use. By default, the visible indicator is used.
		 */
		IdentifyCluster(chip::EndpointId endpoint, chip::app::Clusters::IdentifyDelegate &identifyDelegate,
				chip::TimerDelegate &timerDelegate,
				chip::app::Clusters::Identify::IdentifyTypeEnum identifyType =
					chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator)
			: mEndpointId(endpoint),
			  mIdentifyCluster(chip::app::Clusters::IdentifyCluster::Config(endpoint, timerDelegate)
						   .WithIdentifyType(identifyType)
						   .WithDelegate(&identifyDelegate))
		{
		}

		/**
		 * @brief Construct a new Identify Cluster with the default behavior for nRF development
		 * kits.
		 *
		 * The default behavior is to blink the LED2 on an nRF development kit.
		 *
		 * @note This constructor uses the nRF development kit identify delegate and the default timer
		 * delegate by default.
		 *
		 * @param endpoint The endpoint of the identify cluster
		 * @param identifyType The type of identify to use. By default, the visible indicator is used.
		 */
		explicit IdentifyCluster(chip::EndpointId endpoint, bool isTriggerEffectEnabled = false,
					 std::function<void()> customIdentifyStopCallback = nullptr,
					 chip::app::Clusters::Identify::IdentifyTypeEnum identifyType =
						 chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator)
			: mEndpointId(endpoint), mIdentifyDelegate(isTriggerEffectEnabled, customIdentifyStopCallback),
			  mIdentifyCluster(chip::app::Clusters::IdentifyCluster::Config(endpoint, mTimerDelegate)
						   .WithIdentifyType(identifyType)
						   .WithDelegate(&mIdentifyDelegate))
		{
		}

		/**
		 * @brief Initialize and register the Identify cluster with the Matter server.
		 *
		 * This method registers the Identify cluster with the Matter server's
		 * cluster registry. It is safe to call this method multiple times - if the
		 * cluster is already registered, it will return CHIP_NO_ERROR without
		 * performing any action.
		 *
		 * @note This method is not thread-safe. It must be called from the main thread.
		 *
		 * @return CHIP_NO_ERROR if the cluster was successfully registered or
		 *         already exists, or an appropriate error code if registration failed.
		 */
		CHIP_ERROR Init()
		{
			if (chip::app::CodegenDataModelProvider::Instance().Registry().Get(
				    { mEndpointId, chip::app::Clusters::Identify::Id }) != nullptr) {
				// Already registered
				return CHIP_NO_ERROR;
			}
			return chip::app::CodegenDataModelProvider::Instance().Registry().Register(
				mIdentifyCluster.Registration());
		}

	private:
		chip::EndpointId mEndpointId;
		IdentifyDelegateImplNrf mIdentifyDelegate;
		chip::app::RegisteredServerCluster<chip::app::Clusters::IdentifyCluster> mIdentifyCluster;
		chip::app::DefaultTimerDelegate mTimerDelegate;
	};

} // namespace Matter
} // namespace Nrf
