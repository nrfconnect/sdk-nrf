/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "event_triggers.h"

#include <zephyr/logging/log.h>

/* Server used only for getting the ICDManager instance */
#ifdef CONFIG_CHIP_ENABLE_ICD_SUPPORT
#include <app/server/Server.h>
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace Nrf::Matter;
using namespace chip;

#ifdef CONFIG_CHIP_ENABLE_ICD_SUPPORT
CHIP_ERROR TestEventTrigger::RegisterICDTestEventTriggers()
{
    /*
     * Register ICDManager as a test event trigger handler.
     * This reuses the existing HandleEventTrigger implementation in ICDManager.cpp
     * which already handles all ICD test event triggers including:
     * - Active mode management (add/remove active mode requirements)
     * - Check-in counter operations (invalidate half/all counter values)
     * - DSLS (Dynamic SIT/LIT Support) mode changes
     * - Force maximum check-in backoff state
     */
    CHIP_ERROR err = RegisterTestEventTriggerHandler(&chip::Server::GetInstance().GetICDManager());
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("Failed to register ICDManager as test event trigger handler: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }

    LOG_INF("Successfully registered ICDManager as test event trigger handler");
    return CHIP_NO_ERROR;
}
#endif

CHIP_ERROR TestEventTrigger::RegisterTestEventTrigger(EventTriggerId id, EventTrigger trigger)
{
	VerifyOrReturnError(trigger.Callback != nullptr, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnError(!mTriggersMap.Contains(id), CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnError(mTriggersMap.Insert(id, std::move(trigger)), CHIP_ERROR_NO_MEMORY);

	LOG_DBG("Registered new test event: 0x%llx", id);

	return CHIP_NO_ERROR;
}

void TestEventTrigger::UnregisterTestEventTrigger(EventTriggerId id)
{
	if (mTriggersMap.Erase(id)) {
		LOG_DBG("Unregistered test event: 0x%llx", id);
	} else {
		LOG_WRN("Cannot unregister test event: 0x%llx", id);
	}
}

CHIP_ERROR TestEventTrigger::RegisterTestEventTriggerHandler(chip::TestEventTriggerHandler *triggerDelegate)
{
	VerifyOrReturnError(triggerDelegate != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

	for (size_t idx = 0; idx < kMaxEventTriggersHandlers; idx++) {
		if (mHandlers[idx] == nullptr) {
			mHandlers[idx] = triggerDelegate;
			return CHIP_NO_ERROR;
		}
	}

	return CHIP_ERROR_NO_MEMORY;
}

CHIP_ERROR TestEventTrigger::SetEnableKey(const chip::ByteSpan &newEnableKey)
{
	VerifyOrReturnError(newEnableKey.size() == chip::TestEventTriggerDelegate::kEnableKeyLength,
			    CHIP_ERROR_INVALID_ARGUMENT);
	return CopySpanToMutableSpan(newEnableKey, mEnableKey);
}

bool TestEventTrigger::DoesEnableKeyMatch(const chip::ByteSpan &enableKey) const
{
	return !mEnableKey.empty() && mEnableKey.data_equal(enableKey);
}

CHIP_ERROR TestEventTrigger::HandleEventTriggers(uint64_t eventTrigger)
{
	/* Check if the Enable Key is set to the "disabled" value. */
	if (!IsEventTriggerEnabled()) {
		return CHIP_ERROR_INCORRECT_STATE;
	}

	EventTriggerId triggerID = eventTrigger & kEventTriggerMask;

	/* Check if the provided event trigger exists in Trigger map */
	if (mTriggersMap.Contains(triggerID)) {
		VerifyOrReturnError(mTriggersMap[triggerID].Callback != nullptr, CHIP_ERROR_INCORRECT_STATE);
		return mTriggersMap[triggerID].Callback(eventTrigger & mTriggersMap[triggerID].Mask);
	}

	/* Event trigger has not been found in the Trigger map so check if one of the registered Event Trigger
	 * Handler Delegates can be called instead.
	 */
	for (size_t idx = 0; idx < kMaxEventTriggersHandlers; idx++) {
		if (mHandlers[idx]) {
			CHIP_ERROR status = mHandlers[idx]->HandleEventTrigger(eventTrigger);
			if (status == CHIP_NO_ERROR) {
				return CHIP_NO_ERROR;
			}
		}
	}

	return CHIP_ERROR_INCORRECT_STATE;
}
