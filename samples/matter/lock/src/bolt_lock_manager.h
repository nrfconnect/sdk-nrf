/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app-common/zap-generated/enums.h>
#include <lib/core/ClusterEnums.h>

#include <zephyr/kernel.h>

#include <cstdint>

class AppEvent;

class BoltLockManager {
public:
	enum class State : uint8_t {
		kLockingInitiated = 0,
		kLockingCompleted,
		kUnlockingInitiated,
		kUnlockingCompleted,
	};

	using OperationSource = chip::app::Clusters::DoorLock::DlOperationSource;
	using StateChangeCallback = void (*)(State, OperationSource);

	static constexpr uint32_t kActuatorMovementTimeMs = 2000;

	void Init(StateChangeCallback callback);

	State GetState() const { return mState; }
	bool IsLocked() const { return mState == State::kLockingCompleted; }

	void Lock(OperationSource source);
	void Unlock(OperationSource source);

private:
	friend class AppTask;

	void SetState(State state, OperationSource source);
	void CompleteLockAction();

	static void ActuatorTimerEventHandler(k_timer *timer);
	friend BoltLockManager &BoltLockMgr();

	State mState = State::kLockingCompleted;
	StateChangeCallback mStateChangeCallback = nullptr;
	OperationSource mActuatorOperationSource = OperationSource::kButton;
	k_timer mActuatorTimer = {};

	static BoltLockManager sLock;
};

inline BoltLockManager &BoltLockMgr()
{
	return BoltLockManager::sLock;
}
