/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bolt_lock_manager.h"
#include "app/task_executor.h"

#include "app_task.h"

using namespace chip;

BoltLockManager BoltLockManager::sLock;

void BoltLockManager::Init(StateChangeCallback callback)
{
	mStateChangeCallback = callback;

	k_timer_init(&mActuatorTimer, &BoltLockManager::ActuatorTimerEventHandler, nullptr);
	k_timer_user_data_set(&mActuatorTimer, this);

	PinManager::Instance().Init();

	/* Set the default state */
	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(IsLocked());
}

bool BoltLockManager::GetUser(uint16_t userIndex, EmberAfPluginDoorLockUserInfo &user)
{
	return PinManager::Instance().GetUserInfo(userIndex, user);
}

bool BoltLockManager::SetUser(uint16_t userIndex, FabricIndex creator, FabricIndex modifier, const CharSpan &userName,
			      uint32_t uniqueId, UserStatusEnum userStatus, UserTypeEnum userType,
			      CredentialRuleEnum credentialRule, const CredentialStruct *credentials,
			      size_t totalCredentials)
{
	return PinManager::Instance().SetUser(userIndex, creator, modifier, userName, uniqueId, userStatus, userType,
					      credentialRule, credentials, totalCredentials);
}

bool BoltLockManager::GetCredential(uint16_t credentialIndex, CredentialTypeEnum credentialType,
				    EmberAfPluginDoorLockCredentialInfo &credential)
{
	return PinManager::Instance().GetCredential(credentialIndex, credentialType, credential);
}

bool BoltLockManager::SetCredential(uint16_t credentialIndex, FabricIndex creator, FabricIndex modifier,
				    DlCredentialStatus credentialStatus, CredentialTypeEnum credentialType,
				    const ByteSpan &secret)
{
	return PinManager::Instance().SetCredential(credentialIndex, creator, modifier, credentialStatus,
						    credentialType, secret);
}

bool BoltLockManager::ValidatePIN(const Optional<ByteSpan> &pinCode, OperationErrorEnum &err)
{
	return PinManager::Instance().ValidatePIN(pinCode, err);
}

void BoltLockManager::SetRequirePIN(bool require)
{
	return PinManager::Instance().SetRequirePIN(require);
}
bool BoltLockManager::GetRequirePIN()
{
	return PinManager::Instance().GetRequirePIN();
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
	/*
	 * The timer event handler is called in the context of the system clock ISR.
	 * Post an event to the application task queue to process the event in the
	 * context of the application thread.
	 */

	BoltLockManagerEvent event;
	event.manager = static_cast<BoltLockManager *>(k_timer_user_data_get(timer));
	Nrf::PostTask([event] { ActuatorAppEventHandler(event); });
}

void BoltLockManager::ActuatorAppEventHandler(const BoltLockManagerEvent &event)
{
	BoltLockManager *lock = reinterpret_cast<BoltLockManager *>(event.manager);

	switch (lock->mState) {
	case State::kLockingInitiated:
		lock->SetState(State::kLockingCompleted, lock->mActuatorOperationSource);
		break;
	case State::kUnlockingInitiated:
		lock->SetState(State::kUnlockingCompleted, lock->mActuatorOperationSource);
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
