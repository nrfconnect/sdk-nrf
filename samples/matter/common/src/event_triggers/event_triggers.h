/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/TestEventTriggerDelegate.h>
#include <lib/core/CHIPError.h>

#include "util/finite_map.h"

#include <cstddef>
#include <cstdint>

namespace Nrf::Matter
{
class TestEventTrigger : public chip::TestEventTriggerDelegate {
public:
	using EventTriggerId = uint64_t;
	using TriggerValueMask = uint32_t;
	using TriggerValue = uint32_t;
	using TriggerSlot = uint16_t;

	/* A function callback recipe for handling the event triggers
	To match the event trigger requirements, you can use the following CHIP_ERROR codes inside.
	All error codes (except CHIP_NO_ERROR) send "INVALID_COMMAND" response to Matter Controller.
	*/
	using EventTriggerCallback = CHIP_ERROR (*)(TriggerValue);

	/* Define the maximum available event triggers that can be registered. */
	constexpr static TriggerSlot kMaxEventTriggers = CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX;
	constexpr static TriggerSlot kInvalidTriggerSlot = CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX + 1;
	constexpr static TriggerSlot kMaxEventTriggersHandlers = CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX_TRIGGERS_DELEGATES;
	constexpr static uint8_t kDisableEventTriggersKey[chip::TestEventTriggerDelegate::kEnableKeyLength] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	constexpr static EventTriggerId kEventTriggerMask = 0xFFFFFFFFFFFF0000;

	/**
	 * @brief Struct of event trigger
	 *
	 * The Id field represents an unique ID of the event trigger, and it should be provided as 64-bits value with
	 32-bits right shift. For example 0xFFFFFFFF00000000
	 * The Mask field represents a mask of specific event trigger value. It should be provided as a 32-bits mask
	 value. All values beyond this mask will be ignored.
	 * The Callback field represents a function callback that will be invoked when the appropriate event trigger ID
	 is received.
	 */
	struct EventTrigger {
		TriggerValueMask Mask = 0;
		EventTriggerCallback Callback = nullptr;
	};

	/**
	 * @brief Use Instance static method to reach TestEventTrigger class methods.
	 *
	 * @return TestEventTrigger& TestEventTrigger object.
	 */
	static TestEventTrigger &Instance()
	{
		static TestEventTrigger sInstance;
		return sInstance;
	}

	~TestEventTrigger() = default;

	/**
	 * @brief Register new event trigger for a specific purpose
	 *
	 * The trigger.Callback function is called in not thread-safe manner.
	 * It means that you must ensure thread-safety and run all operations within the callback in the proper context.
	 *
	 * @param trigger The @ref EventTrigger structure that contains ID, value Mask and Callback of the new event
	 * trigger.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if the trigger.Callback field is not assigned (nullptr)
	 * @return CHIP_ERROR_INVALID_ARGUMENT if the provided eventTrigger ID is already registered.
	 * @return CHIP_ERROR_NO_MEMORY if there is no memory for registering the new event trigger.
	 * @return CHIP_NO_ERROR on success.
	 */
	CHIP_ERROR RegisterTestEventTrigger(EventTriggerId id, EventTrigger trigger);

	/**
	 * @brief Unregister the existing event trigger.
	 *
	 * @param id the ID of the test event to be unregistered.
	 */
	void UnregisterTestEventTrigger(EventTriggerId id);

	/**
	 * @brief Register an implementation of the TestEventTriggerDelegate
	 *
	 * The implementation will be called if the received event trigger ID has not been found in
	 * mTriggersDict dictionary.
	 *
	 * @param triggerDelegate pointer to the TestEventTriggerDelegate implementation.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if the triggerDelegate is NULL.
	 * @return CHIP_ERROR_NO_MEMORY if the triggerDelegate list is full.
	 * @return CHIP_NO_ERROR on success.
	 */
	CHIP_ERROR RegisterTestEventTriggerHandler(chip::TestEventTriggerHandler *triggerDelegate);

	/**
	 * @brief Set the new Enable Key read out from an external source.
	 *
	 * This method is required for all further operations.
	 *
	 * @param newEnableKey the new enable key to be set.
	 * @return CHIP_NO_ERROR on success.
	 */
	CHIP_ERROR SetEnableKey(const chip::ByteSpan &newEnableKey);

	/* TestEventTriggerDelegate implementations */
	bool DoesEnableKeyMatch(const chip::ByteSpan &enableKey) const override;
	CHIP_ERROR HandleEventTriggers(uint64_t eventTrigger) override;

private:
	TestEventTrigger() = default;

	uint16_t GetTriggerSlot(uint64_t eventTrigger);
	bool IsEventTriggerEnabled()
	{
		return memcmp(kDisableEventTriggersKey, mEnableKeyData,
			      chip::TestEventTriggerDelegate::kEnableKeyLength) != 0;
	}

	uint8_t mEnableKeyData[chip::TestEventTriggerDelegate::kEnableKeyLength] = {};
	chip::MutableByteSpan mEnableKey{ mEnableKeyData };
	Nrf::FiniteMap<EventTriggerId, EventTrigger, kMaxEventTriggers> mTriggersMap;
	chip::TestEventTriggerHandler* mHandlers[kMaxEventTriggersHandlers];
};
}; // namespace Nrf::Matter
