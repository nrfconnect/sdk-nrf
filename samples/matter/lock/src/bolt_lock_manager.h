/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/core/ClusterEnums.h>

#include <zephyr/kernel.h>

#include <cstdint>

class AppEvent;

class BoltLockManager {
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
		DlCredential mCredentials[CONFIG_LOCK_NUM_CREDENTIALS_PER_USER];
	};

	struct CredentialData {
		chip::Platform::ScopedMemoryBuffer<uint8_t> mSecret;
	};

	using OperationSource = chip::app::Clusters::DoorLock::DlOperationSource;
	using StateChangeCallback = void (*)(State, OperationSource);

	static constexpr uint32_t kActuatorMovementTimeMs = 2000;

	void Init(StateChangeCallback callback);

	State GetState() const { return mState; }
	bool IsLocked() const { return mState == State::kLockingCompleted; }

	bool GetUser(uint16_t userIndex, EmberAfPluginDoorLockUserInfo &user) const;
	bool SetUser(uint16_t userIndex, chip::FabricIndex creator, chip::FabricIndex modifier,
		     const chip::CharSpan &userName, uint32_t uniqueId, DlUserStatus userStatus, DlUserType userType,
		     DlCredentialRule credentialRule, const DlCredential *credentials, size_t totalCredentials);

	bool GetCredential(uint16_t credentialIndex, DlCredentialType credentialType,
			   EmberAfPluginDoorLockCredentialInfo &credential) const;
	bool SetCredential(uint16_t credentialIndex, chip::FabricIndex creator, chip::FabricIndex modifier,
			   DlCredentialStatus credentialStatus, DlCredentialType credentialType,
			   const chip::ByteSpan &secret);

	bool ValidatePIN(const Optional<chip::ByteSpan> &pinCode, DlOperationError &err) const;

	void Lock(OperationSource source);
	void Unlock(OperationSource source);

private:
	friend class AppTask;

	void SetState(State state, OperationSource source);

	static void ActuatorTimerEventHandler(k_timer *timer);
	static void ActuatorAppEventHandler(const AppEvent &aEvent);
	friend BoltLockManager &BoltLockMgr();

	State mState = State::kLockingCompleted;
	StateChangeCallback mStateChangeCallback = nullptr;
	OperationSource mActuatorOperationSource = OperationSource::kButton;
	k_timer mActuatorTimer = {};

	UserData mUserData[CONFIG_LOCK_NUM_USERS];
	EmberAfPluginDoorLockUserInfo mUsers[CONFIG_LOCK_NUM_USERS] = {};

	CredentialData mCredentialData[CONFIG_LOCK_NUM_CREDENTIALS];
	EmberAfPluginDoorLockCredentialInfo mCredentials[CONFIG_LOCK_NUM_CREDENTIALS] = {};

	static BoltLockManager sLock;
};

inline BoltLockManager &BoltLockMgr()
{
	return BoltLockManager::sLock;
}
