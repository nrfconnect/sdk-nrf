/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_onoff_light_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace Nrf;

void SimulatedOnOffLightDataProvider::Init()
{
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC
	k_timer_init(&mTimer, SimulatedOnOffLightDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kOnOffIntervalMs), K_MSEC(kOnOffIntervalMs));
#endif
}

void SimulatedOnOffLightDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, data,
					 dataSize);
	}
}

CHIP_ERROR SimulatedOnOffLightDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							uint8_t *buffer)
{
	if (clusterId != Clusters::OnOff::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	LOG_INF("Updating state of the SimulatedOnOffLightDataProvider, cluster ID: %u, attribute ID: %u.", clusterId,
		attributeId);

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		memcpy(&mOnOff, buffer, sizeof(mOnOff));
		NotifyUpdateState(clusterId, attributeId, &mOnOff, sizeof(mOnOff));
		return CHIP_NO_ERROR;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return CHIP_NO_ERROR;
}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC
void SimulatedOnOffLightDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	SimulatedOnOffLightDataProvider *provider =
		reinterpret_cast<SimulatedOnOffLightDataProvider *>(timer->user_data);

	/* Toggle light state to emulate user's manual interactions. */
	provider->mOnOff = !provider->mOnOff;

	LOG_INF("SimulatedOnOffLightDataProvider: Updated light state to %d", provider->mOnOff);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}
#endif

void SimulatedOnOffLightDataProvider::NotifyAttributeChange(intptr_t context)
{
	SimulatedOnOffLightDataProvider *provider = reinterpret_cast<SimulatedOnOffLightDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, &provider->mOnOff,
				    sizeof(provider->mOnOff));
}
