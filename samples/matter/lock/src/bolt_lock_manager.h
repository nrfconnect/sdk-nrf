/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "access/access_manager.h"

#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/core/ClusterEnums.h>

#include <zephyr/kernel.h>

#include <cstdint>

struct BoltLockManagerEvent;

class BoltLockManager {
	using AccessMgr = AccessManager<DoorLockData::PIN>;

public:
	static constexpr size_t kMaxCredentialLength = 128;

	enum class State : uint8_t {
		kLockingInitiated = 0,
		kLockingCompleted,
		kUnlockingInitiated,
		kUnlockingCompleted,
	};

	struct UserData {
		char mName[DOOR_LOCK_USER_NAME_BUFFER_SIZE];
		CredentialStruct mCredentials[CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER];
	};

	struct CredentialData {
		chip::Platform::ScopedMemoryBuffer<uint8_t> mSecret;
	};

	using OperationSource = chip::app::Clusters::DoorLock::OperationSourceEnum;
	using ValidatePINResult = AccessMgr::ValidatePINResult;

	struct StateData {
		State mState;
		OperationSource mSource;
		Nullable<chip::FabricIndex> mFabricIdx;
		Nullable<chip::NodeId> mNodeId;
		Nullable<ValidatePINResult> mValidatePINResult;
	};

	using StateChangeCallback = void (*)(const StateData &);

	static constexpr uint32_t kActuatorMovementTimeMs = 2000;

	void Init(StateChangeCallback callback);

	const StateData &GetState() const { return mStateData; }
	bool IsLocked() const { return mStateData.mState == State::kLockingCompleted; }

	bool GetUser(uint16_t userIndex, EmberAfPluginDoorLockUserInfo &user);
	bool SetUser(uint16_t userIndex, chip::FabricIndex creator, chip::FabricIndex modifier,
		     const chip::CharSpan &userName, uint32_t uniqueId, UserStatusEnum userStatus,
		     UserTypeEnum userType, CredentialRuleEnum credentialRule, const CredentialStruct *credentials,
		     size_t totalCredentials);

	bool GetCredential(uint16_t credentialIndex, CredentialTypeEnum credentialType,
			   EmberAfPluginDoorLockCredentialInfo &credential);
	bool SetCredential(uint16_t credentialIndex, chip::FabricIndex creator, chip::FabricIndex modifier,
			   DlCredentialStatus credentialStatus, CredentialTypeEnum credentialType,
			   const chip::ByteSpan &secret);

#ifdef CONFIG_LOCK_SCHEDULES
	DlStatus GetWeekDaySchedule(uint8_t weekdayIndex, uint16_t userIndex,
				    EmberAfPluginDoorLockWeekDaySchedule &schedule);
	DlStatus SetWeekDaySchedule(uint8_t weekdayIndex, uint16_t userIndex, DlScheduleStatus status,
				    DaysMaskMap daysMask, uint8_t startHour, uint8_t startMinute, uint8_t endHour,
				    uint8_t endMinute);
	DlStatus GetYearDaySchedule(uint8_t yearDayIndex, uint16_t userIndex,
				    EmberAfPluginDoorLockYearDaySchedule &schedule);
	DlStatus SetYearDaySchedule(uint8_t yearDayIndex, uint16_t userIndex, DlScheduleStatus status,
				    uint32_t localStartTime, uint32_t localEndTime);
	DlStatus GetHolidaySchedule(uint8_t holidayIndex, EmberAfPluginDoorLockHolidaySchedule &schedule);
	DlStatus SetHolidaySchedule(uint8_t holidayIndex, DlScheduleStatus status, uint32_t localStartTime,
				    uint32_t localEndTime, OperatingModeEnum operatingMode);
#endif /* CONFIG_LOCK_SCHEDULES */

	bool ValidatePIN(const Optional<chip::ByteSpan> &pinCode, OperationErrorEnum &err,
			 Nullable<ValidatePINResult> &result);

	void Lock(const OperationSource source, const Nullable<chip::FabricIndex> &fabricIdx = NullNullable,
		  const Nullable<chip::NodeId> &nodeId = NullNullable,
		  const Nullable<ValidatePINResult> &validatePINResult = NullNullable);
	void Unlock(const OperationSource source, const Nullable<chip::FabricIndex> &fabricIdx = NullNullable,
		    const Nullable<chip::NodeId> &nodeId = NullNullable,
		    const Nullable<ValidatePINResult> &validatePINResult = NullNullable);

	void SetRequirePIN(bool require);
	bool GetRequirePIN();

	void FactoryReset();

private:
	friend class AppTask;

	void SetState(State state);
	void SetStateData(const StateData &stateData);

	static void ActuatorTimerEventHandler(k_timer *timer);
	static void ActuatorAppEventHandler(const BoltLockManagerEvent &event);
	friend BoltLockManager &BoltLockMgr();

	StateData mStateData = { State::kLockingCompleted, OperationSource::kButton, {}, {}, {} };
	StateChangeCallback mStateChangeCallback = nullptr;
	k_timer mActuatorTimer = {};

	static BoltLockManager sLock;
};

inline BoltLockManager &BoltLockMgr()
{
	return BoltLockManager::sLock;
}

struct BoltLockManagerEvent {
	BoltLockManager *manager;
};
