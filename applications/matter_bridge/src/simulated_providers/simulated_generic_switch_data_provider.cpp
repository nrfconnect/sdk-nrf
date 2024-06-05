/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_generic_switch_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace Nrf;

void SimulatedGenericSwitchDataProvider::Init()
{
	k_timer_init(&mTimer, SimulatedGenericSwitchDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kSwitchChangePositionIntervalMs), K_MSEC(kSwitchChangePositionIntervalMs));
}

void SimulatedGenericSwitchDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							   void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::Switch::Id, Clusters::Switch::Attributes::CurrentPosition::Id,
					 data, dataSize);
	}
}

CHIP_ERROR SimulatedGenericSwitchDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							   uint8_t *buffer)
{
	if (clusterId != Clusters::Switch::Id && clusterId != Clusters::BridgedDeviceBasicInformation::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	LOG_INF("Updating state of the SimulatedGenericSwitchDataProvider, cluster ID: %u, attribute ID: %u.",
		clusterId, attributeId);

	switch (attributeId) {
	case Clusters::Switch::Attributes::CurrentPosition::Id: {
		memcpy(&mCurrentSwitchPosition, buffer, sizeof(mCurrentSwitchPosition));
		NotifyUpdateState(clusterId, attributeId, &mCurrentSwitchPosition, sizeof(mCurrentSwitchPosition));
		return CHIP_NO_ERROR;
	}
	case Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id:
		/* Node label is just updated locally and there is no need to propagate the information to the end
		 * device. */
		break;
	default:
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

	return CHIP_NO_ERROR;
}

void SimulatedGenericSwitchDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	SimulatedGenericSwitchDataProvider *provider =
		reinterpret_cast<SimulatedGenericSwitchDataProvider *>(timer->user_data);

	/* Toggle switch state to emulate user's manual interactions. */
	provider->mCurrentSwitchPosition = !provider->mCurrentSwitchPosition;

	LOG_INF("SimulatedGenericSwitchDataProvider: Updated switch position to %d", provider->mCurrentSwitchPosition);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

void SimulatedGenericSwitchDataProvider::NotifyAttributeChange(intptr_t context)
{
	SimulatedGenericSwitchDataProvider *provider = reinterpret_cast<SimulatedGenericSwitchDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::Switch::Id, Clusters::Switch::Attributes::CurrentPosition::Id,
				    &provider->mCurrentSwitchPosition, sizeof(provider->mCurrentSwitchPosition));
}
