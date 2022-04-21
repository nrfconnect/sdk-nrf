/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bolt_lock_manager.h"

#include "app_event.h"
#include "app_task.h"

BoltLockManager BoltLockManager::sLock;

void BoltLockManager::Init(StateChangeCallback callback)
{
	mStateChangeCallback = callback;

	k_timer_init(&mActuatorTimer, &BoltLockManager::ActuatorTimerEventHandler, nullptr);
	k_timer_user_data_set(&mActuatorTimer, this);
}

void BoltLockManager::Lock(OperationSource source)
{
	VerifyOrReturn(mState != State::kLockingCompleted);
	SetState(State::kLockingInitiated, source);

	mActuatorOperationSource = source;
	k_timer_start(&mActuatorTimer, K_MSEC(kActuatorMovementTimeMs), K_NO_WAIT);
}

void BoltLockManager::Unlock(OperationSource source)
{
	VerifyOrReturn(mState != State::kUnlockingCompleted);
	SetState(State::kUnlockingInitiated, source);

	mActuatorOperationSource = source;
	k_timer_start(&mActuatorTimer, K_MSEC(kActuatorMovementTimeMs), K_NO_WAIT);
}

void BoltLockManager::ActuatorTimerEventHandler(k_timer *timer)
{
	/* The timer event handler is called in the context of the system clock ISR. */
	/* Post an event to the application task queue to process the event in the */
	/* context of the application thread. */

	GetAppTask().PostEvent(AppEvent(AppEvent::CompleteLockAction, BoltLockManager::OperationSource{}));
}

void BoltLockManager::CompleteLockAction()
{
	switch (mState) {
	case State::kLockingInitiated:
		SetState(State::kLockingCompleted, mActuatorOperationSource);
		break;
	case State::kUnlockingInitiated:
		SetState(State::kUnlockingCompleted, mActuatorOperationSource);
		break;
	default:
		break;
	}
}

void BoltLockManager::SetState(State state, OperationSource source)
{
	mState = state;

	if (mStateChangeCallback != nullptr) {
		mStateChangeCallback(state, source);
	}
}
