/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "access_storage.h"

#include "access_storage_print.h"

#include <persistent_storage/persistent_storage.h>

#include <zephyr/sys/cbprintf.h>

/* Currently the secure storage is available only for non-Wi-Fi builds,
   because NCS Wi-Fi implementation does not support PSA API yet. */
#if defined(CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND) && defined(CONFIG_CHIP_WIFI)
#error CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND is currently not available if CONFIG_CHIP_WIFI is set.
#endif

/* Prefer to use CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND if both backends are set simultaneously */
#ifdef CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND
#define PSInit SecureInit
#define PSStore SecureStore
#define PSRemove SecureRemove
#define PSLoad SecureLoad
#define PSFactoryReset SecureFactoryReset
#elif defined(CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND)
#define PSInit NonSecureInit
#define PSStore NonSecureStore
#define PSLoad NonSecureLoad
#define PSRemove NonSecureRemove
#define PSFactoryReset NonSecureFactoryReset
#endif

bool AccessStorage::Init()
{
	return Nrf::PSErrorCode::Success == Nrf::GetPersistentStorage().PSInit(&mRootNode);
}

void AccessStorage::FactoryReset()
{
	Nrf::GetPersistentStorage().PSFactoryReset();
}

bool AccessStorage::PrepareKeyName(Type storageType, uint16_t index, uint16_t subindex)
{
	memset(mKeyName, '\0', sizeof(mKeyName));

	uint8_t limitedIndex = static_cast<uint8_t>(index);
	uint8_t limitedSubindex = static_cast<uint8_t>(subindex);

	switch (storageType) {
	case Type::User:
		if (0 == limitedIndex && 0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxAccessName, "%s/%s", kAccessPrefix, kUserPrefix);
		} else if (0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxAccessName, "%s/%s/%u", kAccessPrefix, kUserPrefix, limitedIndex);
		} else {
			(void)snprintf(mKeyName, kMaxAccessName, "%s/%s/%u/%u", kAccessPrefix, kUserPrefix,
				       limitedIndex, limitedSubindex);
		}
		return true;
	case Type::Credential:
		if (0 == limitedIndex && 0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxAccessName, "%s", kAccessPrefix);
		} else if (0 == limitedSubindex) {
			(void)snprintf(mKeyName, kMaxAccessName, "%s/%u", kAccessPrefix, limitedIndex);
		} else {
			(void)snprintf(mKeyName, kMaxAccessName, "%s/%u/%u", kAccessPrefix, limitedIndex,
				       limitedSubindex);
		}
		return true;
	case Type::UsersIndexes:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s", kAccessPrefix, kUserCounterPrefix);
		return true;
	case Type::CredentialsIndexes:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%u/%s", kAccessPrefix, limitedIndex, kAccessCounterPrefix);
		return true;
	case Type::RequirePIN:
		(void)snprintf(mKeyName, kMaxAccessName, "%s", kRequirePinPrefix);
		return true;
#ifdef CONFIG_LOCK_SCHEDULES
	case Type::WeekDaySchedule:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s%s/%u/%u", kAccessPrefix, kSchedulePrefix,
			       kScheduleWeekDaySuffix, limitedIndex, limitedSubindex);
		return true;
	case Type::YearDaySchedule:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s%s/%u/%u", kAccessPrefix, kSchedulePrefix,
			       kScheduleYearDaySuffix, limitedIndex, limitedSubindex);
		return true;
	case Type::HolidaySchedule:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s%s/%u", kAccessPrefix, kSchedulePrefix,
			       kScheduleHolidaySuffix, limitedIndex);
		return true;
	case Type::WeekDayScheduleIndexes:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s%s/%u", kAccessPrefix, kScheduleCounterPrefix,
			       kScheduleWeekDaySuffix, limitedIndex);
		return true;
	case Type::YearDayScheduleIndexes:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s%s/%u", kAccessPrefix, kScheduleCounterPrefix,
			       kScheduleYearDaySuffix, limitedIndex);
		return true;
	case Type::HolidayScheduleIndexes:
		(void)snprintf(mKeyName, kMaxAccessName, "%s/%s%s", kAccessPrefix, kScheduleCounterPrefix,
			       kScheduleHolidaySuffix);
		return true;
#endif /* CONFIG_LOCK_SCHEDULES */
	default:
		break;
	}

	return false;
}

bool AccessStorage::Store(Type storageType, const void *data, size_t dataSize, uint16_t index, uint16_t subindex)
{
	if (data == nullptr || !PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ mKeyName, strlen(mKeyName) + 1 };
	bool ret = (Nrf::PSErrorCode::Success == Nrf::GetPersistentStorage().PSStore(&node, data, dataSize));

#ifdef CONFIG_LOCK_PRINT_STORAGE_STATUS
	PrintAccessDataStored(storageType, dataSize, ret);
#endif

	return ret;
}

bool AccessStorage::Load(Type storageType, void *data, size_t dataSize, size_t &outSize, uint16_t index,
			 uint16_t subindex)
{
	if (data == nullptr || !PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ mKeyName, strlen(mKeyName) + 1 };
	Nrf::PSErrorCode result = Nrf::GetPersistentStorage().PSLoad(&node, data, dataSize, outSize);

	return (Nrf::PSErrorCode::Success == result);
}

bool AccessStorage::Remove(Type storageType, uint16_t index, uint16_t subindex)
{
	if (!PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ mKeyName, strlen(mKeyName) + 1 };
	Nrf::PSErrorCode result = Nrf::GetPersistentStorage().PSRemove(&node);

	return (Nrf::PSErrorCode::Success == result);
}
