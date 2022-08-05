/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "thread_util.h"

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/zephyr.h>

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
#include <openthread/platform/radio.h>
#include <platform/OpenThread/OpenThreadUtils.h>
#endif

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
CHIP_ERROR SetDefaultThreadOutputPower()
{
	CHIP_ERROR err;
	/* set output power when FEM is active */
	chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
	err = chip::DeviceLayer::Internal::MapOpenThreadError(
		otPlatRadioSetTransmitPower(chip::DeviceLayer::ThreadStackMgrImpl().OTInstance(),
					    static_cast<int8_t>(CONFIG_OPENTHREAD_DEFAULT_TX_POWER)));
	chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
	return err;
}
#endif
