/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Platform implementation of the OpenThread Alternate PHY (HDR) capability API.
 *
 * This is a configuration-driven implementation:
 * it reports the Alternate PHYs and timing parameters selected through Kconfig.
 */

#include <zephyr/kernel.h>

#include <openthread/platform/alternate_phy.h>

uint8_t otPlatAlternatePhyGetCapabilities(otInstance *aInstance, otAlternatePhyCapability *aCaps,
					  uint8_t aMaxCount)
{
	uint8_t count = 0;

	ARG_UNUSED(aInstance);

	if (aCaps == NULL) {
		return 0;
	}

#if defined(CONFIG_OPENTHREAD_ALTERNATE_PHY_GFSK)
	if (count < aMaxCount) {
		aCaps[count].mPhyId = OT_ALTERNATE_PHY_ID_TL3_GFSK;
		aCaps[count].mParameters[OT_ALTERNATE_PHY_TL3_GFSK_PARAMETER_SETTLING_DELAY] =
			CONFIG_OPENTHREAD_ALTERNATE_PHY_SETTLING_DELAY;
		aCaps[count].mParameters[OT_ALTERNATE_PHY_TL3_GFSK_PARAMETER_AIFS] =
			CONFIG_OPENTHREAD_ALTERNATE_PHY_AIFS;
		aCaps[count].mFlags =
			IS_ENABLED(CONFIG_OPENTHREAD_ALTERNATE_PHY_CONCURRENT_LISTENING)
				? OT_ALTERNATE_PHY_TL3_GFSK_FLAG_CONCURRENT_LISTENING
				: 0;
		count++;
	}
#endif /* CONFIG_OPENTHREAD_ALTERNATE_PHY_GFSK */

	return count;
}
