/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "app_task.h"

#include <logging/log.h>

#include <platform/CHIPDeviceLayer.h>
#include <support/CHIPMem.h>

LOG_MODULE_REGISTER(app);

using namespace ::chip::DeviceLayer;

int main()
{
	int ret = chip::Platform::MemoryInit();

	if (ret != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		return ret;
	}

	ret = PlatformMgr().InitChipStack();
	if (ret != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().InitChipStack() failed");
		return ret;
	}

	ret = ThreadStackMgr().InitThreadStack();
	if (ret != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed");
		return ret;
	}

	ret = PlatformMgr().StartEventLoopTask();
	if (ret != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		return ret;
	}

	return GetAppTask().StartApp();
}
