/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridged_device_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

CHIP_ERROR BridgedDeviceDataProvider::NotifyReachableStatusChange(bool isReachable)
{
	auto reachableContext = chip::Platform::New<ReachableContext>();
	if (!reachableContext) {
		return CHIP_ERROR_NO_MEMORY;
	}

	reachableContext->mIsReachable = isReachable;
	reachableContext->mProvider = this;

	CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
		[](intptr_t context) {
			auto ctx = reinterpret_cast<ReachableContext *>(context);
			ctx->mProvider->NotifyUpdateState(
				chip::app::Clusters::BridgedDeviceBasicInformation::Id,
				chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id,
				&ctx->mIsReachable, sizeof(ctx->mIsReachable));
			chip::Platform::Delete(ctx);
		},
		reinterpret_cast<intptr_t>(reachableContext));

	if (CHIP_NO_ERROR != err) {
		chip::Platform::Delete(reachableContext);
		return err;
	}

	return CHIP_NO_ERROR;
}
