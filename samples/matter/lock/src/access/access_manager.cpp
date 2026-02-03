/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "access_manager.h"
#include "access_storage.h"

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

#include <utility>

LOG_MODULE_REGISTER(cr_manager, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace DoorLockData;

namespace
{
/*
 * For each itemIndex in indexList: calls AccessStorage::Remove(itemType, [extraArgs...,] itemIndex)
 * Finally: calls AccessStorage::Remove(indexesType[, extraArgs...])
 */
template <uint16_t N, class... ExtraArgs>
void RemoveCollection(const DoorLockData::IndexList<N> &indexList, AccessStorage::Type itemType,
		      AccessStorage::Type indexesType, ExtraArgs &&...extraArgs)
{
	for (size_t i = 0; i < indexList.mList.mLength; i++) {
		uint16_t itemIndex = indexList.mList.mIndexes[i];

		if (!AccessStorage::Instance().Remove(itemType, std::forward<ExtraArgs>(extraArgs)..., itemIndex)) {
			LOG_ERR("Cannot remove item %u with type %u", itemIndex, static_cast<unsigned>(itemType));
		}
	}

	if (indexList.mList.mLength > 0) {
		if (!AccessStorage::Instance().Remove(indexesType, std::forward<ExtraArgs>(extraArgs)...)) {
			LOG_ERR("Cannot remove indexes with type %u", static_cast<unsigned>(indexesType));
		}
	}
}
} /* namespace */

template <CredentialsBits CRED_BIT_MASK>
void AccessManager<CRED_BIT_MASK>::Init(SetCredentialCallback setCredentialClbk,
					ClearCredentialCallback clearCredentialClbk,
					ValidateCredentialCallback validateCredentialClbk)
{
	InitializeAllCredentials();
	mSetCredentialCallback = setCredentialClbk;
	mClearCredentialCallback = clearCredentialClbk;
	mValidateCredentialCallback = validateCredentialClbk;
	AccessStorage::Instance().Init();
	LoadUsersFromPersistentStorage();
	LoadCredentialsFromPersistentStorage();
#ifdef CONFIG_LOCK_SCHEDULES
	LoadSchedulesFromPersistentStorage();
#endif /* CONFIG_LOCK_SCHEDULES */
}

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::FactoryReset()
{
	/* Remove users */
	RemoveCollection(mUsersIndexes, AccessStorage::Type::User, AccessStorage::Type::UsersIndexes);

	/* Remove credentials */
	for (uint8_t type = CredentialTypeIndex::Pin; type <= CredentialTypeIndex::Max; ++type) {
		RemoveCollection(mCredentialsIndexes.Get(static_cast<CredentialTypeIndex>(type)),
				 AccessStorage::Type::Credential, AccessStorage::Type::CredentialsIndexes, type);
	}

#ifdef CONFIG_LOCK_SCHEDULES
	/* Remove schedules */
	for (uint16_t userIndex = 1; userIndex <= CONFIG_LOCK_MAX_NUM_USERS; userIndex++) {
		RemoveCollection(mWeekDayScheduleIndexes.Get(userIndex), AccessStorage::Type::WeekDaySchedule,
				 AccessStorage::Type::WeekDayScheduleIndexes, userIndex);
		RemoveCollection(mYearDayScheduleIndexes.Get(userIndex), AccessStorage::Type::YearDaySchedule,
				 AccessStorage::Type::YearDayScheduleIndexes, userIndex);
	}

	RemoveCollection(mHolidayScheduleIndexes, AccessStorage::Type::HolidaySchedule,
			 AccessStorage::Type::HolidayScheduleIndexes);
#endif

	/* Remove other data */
	if (!AccessStorage::Instance().Remove(AccessStorage::Type::RequirePIN)) {
		LOG_ERR("Cannot remove RequirePINforRemoteOperation");
	}

	/* Reinitialize to clear removed users/credentials/schedules */
	InitializeAllCredentials();
}

template <CredentialsBits CRED_BIT_MASK>
bool AccessManager<CRED_BIT_MASK>::ValidateCustom(CredentialTypeEnum type, chip::MutableByteSpan &secret)
{
	/* Run a custom verification within the application layer for RFID credential type */
	if (mValidateCredentialCallback && type == CredentialTypeEnum::kRfid && secret.size() > 0) {
		mValidateCredentialCallback(type, secret);
		return true;
	}
	return false;
}

template <CredentialsBits CRED_BIT_MASK>
bool AccessManager<CRED_BIT_MASK>::ValidatePIN(const Optional<ByteSpan> &pinCode, OperationErrorEnum &err,
					       Nullable<ValidatePINResult> &result)
{
	/* Optionality of the PIN code is validated by the caller, so assume it is OK not to provide the PIN
	 * code. */
	if (!pinCode.HasValue()) {
		return true;
	}

	EmberAfPluginDoorLockCredentialInfo credential;

	/* Check the PIN code */
	for (size_t index = 1; index <= CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE; ++index) {
		if (CHIP_NO_ERROR != mCredentials.GetCredentials(CredentialTypeEnum::kPin, credential, index)) {
			err = OperationErrorEnum::kInvalidCredential;
			continue;
		}

		if (credential.status == DlCredentialStatus::kAvailable) {
			continue;
		}

		if (!credential.credentialData.data_equal(pinCode.Value())) {
			err = OperationErrorEnum::kInvalidCredential;
			continue;
		}

		uint32_t credentialUserId;
		if (GetCredentialUserId(index, CredentialTypeEnum::kPin, credentialUserId) == CHIP_NO_ERROR) {
			result = ValidatePINResult{
				.mUserId = static_cast<uint16_t>(credentialUserId),
				.mCredential =
					LockOpCredentials{ CredentialTypeEnum::kPin, static_cast<uint16_t>(index) },
			};
		} else {
			result = {};
		}

		LOG_DBG("Valid lock PIN code provided");
		err = OperationErrorEnum::kUnspecified;
		return true;
	}
	LOG_DBG("Invalid lock PIN code provided");
	return false;
}

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::InitializeUsers()
{
	for (size_t userIndex = 0; userIndex < CONFIG_LOCK_MAX_NUM_USERS; ++userIndex) {
		auto &user = mUsers[userIndex];

		user.mName.mSize = 0;
		memset(user.mName.mValue, 0, DOOR_LOCK_USER_NAME_BUFFER_SIZE);
		for (size_t credIdx = 0; credIdx < CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER; ++credIdx) {
			user.mOccupiedCredentials.mData[credIdx] = {};
		}
		user.mOccupiedCredentials.mSize = 0;
		memset(user.mInfo.mRaw, 0, sizeof(user.mInfo));
		/* max better than 0 which is the special home user unique ID*/
		user.mInfo.mFields.mUserUniqueId = std::numeric_limits<uint32_t>::max();
	}
}

#ifdef CONFIG_LOCK_SCHEDULES
template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::InitializeSchedules()
{
	for (size_t userIndex = 0; userIndex < CONFIG_LOCK_MAX_NUM_USERS; ++userIndex) {
		for (size_t weekDayIndex = 0; weekDayIndex < CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER;
		     ++weekDayIndex) {
			auto &schedule = mWeekDaySchedule[userIndex][weekDayIndex];
			memset(schedule.mData.mRaw, 0, sizeof(schedule.mData));
		}
		for (size_t yearDayIndex = 0; yearDayIndex < CONFIG_LOCK_MAX_YEARDAY_SCHEDULES_PER_USER;
		     ++yearDayIndex) {
			auto &schedule = mYearDaySchedule[userIndex][yearDayIndex];
			memset(schedule.mData.mRaw, 0, sizeof(schedule.mData));
		}
	}
	for (size_t holidayIndex = 0; holidayIndex < CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER; ++holidayIndex) {
		auto &schedule = mHolidaySchedule[holidayIndex];
		memset(schedule.mData.mRaw, 0, sizeof(schedule.mData));
	}
}
#endif /* CONFIG_LOCK_SCHEDULES */

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::InitializeAllCredentials()
{
	mCredentials.Initialize();
	InitializeUsers();
#ifdef CONFIG_LOCK_SCHEDULES
	InitializeSchedules();
#endif /* CONFIG_LOCK_SCHEDULES */
}

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::SetRequirePIN(bool require)
{
	if (mRequirePINForRemoteOperation != require) {
		mRequirePINForRemoteOperation = require;
		if (!AccessStorage::Instance().Store(AccessStorage::Type::RequirePIN, &mRequirePINForRemoteOperation,
						     sizeof(mRequirePINForRemoteOperation))) {
			LOG_ERR("Cannot store RequirePINforRemoteOperation.");
		}
	}
}

/* Explicitly instantiate supported template variants to avoid linker errors. */
template class AccessManager<DoorLockData::PIN>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID | DoorLockData::FINGER>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID | DoorLockData::FINGER | DoorLockData::VEIN>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID | DoorLockData::FINGER | DoorLockData::VEIN |
			     DoorLockData::FACE>;
