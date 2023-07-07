/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "onoff_light_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

void OnOffLightDataProvider::Init()
{
	k_timer_init(&mTimer, OnOffLightDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kOnOffIntervalMs), K_MSEC(kOnOffIntervalMs));
}

void OnOffLightDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					       size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, data,
					 dataSize);
	}
}

CHIP_ERROR OnOffLightDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
					       uint8_t *buffer)
{
	if (clusterId != Clusters::OnOff::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		mOnOff = *buffer;
		NotifyUpdateState(clusterId, attributeId, &mOnOff, sizeof(mOnOff));
		return CHIP_NO_ERROR;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return CHIP_NO_ERROR;
}

void OnOffLightDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	OnOffLightDataProvider *provider = reinterpret_cast<OnOffLightDataProvider *>(timer->user_data);

	/* Toggle light state to emulate user's manual interactions. */
	provider->mOnOff = !provider->mOnOff;

	LOG_INF("OnOffLightDataProvider: Updated light state to %d", provider->mOnOff);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

void OnOffLightDataProvider::NotifyAttributeChange(intptr_t context)
{
	OnOffLightDataProvider *provider = reinterpret_cast<OnOffLightDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id, &provider->mOnOff,
				    sizeof(provider->mOnOff));
}
