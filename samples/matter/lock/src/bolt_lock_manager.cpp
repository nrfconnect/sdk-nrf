/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bolt_lock_manager.h"

#include "app_task.h"

#include <logging/log.h>
#include <zephyr.h>

LOG_MODULE_DECLARE(app);

static k_timer sLockTimer;

BoltLockManager BoltLockManager::sLock;

void BoltLockManager::Init()
{
	k_timer_init(&sLockTimer, &BoltLockManager::ActuatorTimerHandler, nullptr);
	k_timer_user_data_set(&sLockTimer, this);

	mState = State::LockingCompleted;
	mChipInitiatedAction = false;
}

bool BoltLockManager::IsActionInProgress()
{
	return mState == State::LockingInitiated || mState == State::UnlockingInitiated;
}

bool BoltLockManager::IsUnlocked()
{
	return mState == State::UnlockingCompleted;
}

bool BoltLockManager::InitiateAction(Action action, bool chipInitiated)
{
	/* Initiate Lock/Unlock Action only when the previous one is complete. */
	switch (action) {
	case Action::Lock:
		if (mState != State::UnlockingCompleted) {
			return false;
		}
		LOG_INF("Lock Action has been initiated");
		mState = State::LockingInitiated;
		break;
	case Action::Unlock:
		if (mState != State::LockingCompleted) {
			return false;
		}
		LOG_INF("Unlock Action has been initiated");
		mState = State::UnlockingInitiated;
		break;
	default:
		return false;
	}

	mChipInitiatedAction = chipInitiated;
	StartTimer(kActuatorMovementPeriodMs);
	return true;
}

bool BoltLockManager::CompleteCurrentAction(bool &chipInitiated)
{
	switch (mState) {
	case State::LockingInitiated:
		LOG_INF("Lock Action has been completed");
		mState = State::LockingCompleted;
		chipInitiated = mChipInitiatedAction;
		return true;
	case State::UnlockingInitiated:
		LOG_INF("Unlock Action has been completed");
		mState = State::UnlockingCompleted;
		chipInitiated = mChipInitiatedAction;
		return true;
	default:
		return false;
	}
}

void BoltLockManager::StartTimer(uint32_t aTimeoutMs)
{
	k_timer_start(&sLockTimer, K_MSEC(aTimeoutMs), K_NO_WAIT);
}

void BoltLockManager::CancelTimer()
{
	k_timer_stop(&sLockTimer);
}

void BoltLockManager::ActuatorTimerHandler(k_timer *timer)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::CompleteLockAction, false });
}
