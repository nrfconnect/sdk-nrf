/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file access_storage.cpp
 *
 * Provides the implementation of AccessStorage class that utilizes the persistent storage interface.
 * The persistent storage backend is chosen based on the Kconfig settings:
 * - NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND or
 * - NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND [DEPRECATED].
 *
 * This implementation uses the following persistent storage keys for the access items:
 *
 * /cr
 *     /<credential type (uint8_t)>
 *         /cr                                                   = <credential indices>
 *         /<credential index (uint16_t)>                        = <credential data>
 *     /usr_idxs                                                 = <user indices>
 *     /usr
 *         /<user index (uint16_t)>                              = <user data>
 *     /sch_idxs_<type (w - weekday, y - yearday)>
 *         /<user index (uint16_t)>                              = <schedule indices>
 *     /sch_<type (w - weekday, y - yearday)>
 *         /<user index (uint16_t)>
 *             /<schedule index (uint8_t)>                       = <schedule data>
 *     /sch_idxs_<type (h - holiday)>                            = <schedule indices>
 *     /sch_<type (h - holiday)>
 *         /<schedule index (uint8_t)>                           = <schedule data>
 * /pin_req                                                      = <requires PIN?>
 *
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

namespace
{
constexpr auto kAccessPrefix = "cr";
constexpr auto kAccessCounterPrefix = "cr";
constexpr auto kUserPrefix = "usr";
constexpr auto kUserCounterPrefix = "usr_idxs";
constexpr auto kRequirePinPrefix = "pin_req";
#ifdef CONFIG_LOCK_SCHEDULES
constexpr auto kSchedulePrefix = "sch";
constexpr auto kScheduleWeekDaySuffix = "_w";
constexpr auto kScheduleYearDaySuffix = "_y";
constexpr auto kScheduleHolidaySuffix = "_h";
constexpr auto kScheduleCounterPrefix = "sch_idxs";
#endif /* CONFIG_LOCK_SCHEDULES */
constexpr auto kMaxAccessName = Nrf::PersistentStorageNode::kMaxKeyNameLength;

char keyName[kMaxAccessName];

bool PrepareKeyName(AccessStorage::Type storageType, uint16_t index, uint16_t subindex)
{
	memset(keyName, '\0', sizeof(keyName));

	uint8_t limitedIndex = static_cast<uint8_t>(index);
	uint8_t limitedSubindex = static_cast<uint8_t>(subindex);

	switch (storageType) {
	case AccessStorage::Type::User:
		if (0 == limitedIndex && 0 == limitedSubindex) {
			(void)snprintf(keyName, kMaxAccessName, "%s/%s", kAccessPrefix, kUserPrefix);
		} else if (0 == limitedSubindex) {
			(void)snprintf(keyName, kMaxAccessName, "%s/%s/%u", kAccessPrefix, kUserPrefix, limitedIndex);
		} else {
			(void)snprintf(keyName, kMaxAccessName, "%s/%s/%u/%u", kAccessPrefix, kUserPrefix, limitedIndex,
				       limitedSubindex);
		}
		return true;
	case AccessStorage::Type::Credential:
		if (0 == limitedIndex && 0 == limitedSubindex) {
			(void)snprintf(keyName, kMaxAccessName, "%s", kAccessPrefix);
		} else if (0 == limitedSubindex) {
			(void)snprintf(keyName, kMaxAccessName, "%s/%u", kAccessPrefix, limitedIndex);
		} else {
			(void)snprintf(keyName, kMaxAccessName, "%s/%u/%u", kAccessPrefix, limitedIndex,
				       limitedSubindex);
		}
		return true;
	case AccessStorage::Type::UsersIndexes:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s", kAccessPrefix, kUserCounterPrefix);
		return true;
	case AccessStorage::Type::CredentialsIndexes:
		(void)snprintf(keyName, kMaxAccessName, "%s/%u/%s", kAccessPrefix, limitedIndex, kAccessCounterPrefix);
		return true;
	case AccessStorage::Type::RequirePIN:
		(void)snprintf(keyName, kMaxAccessName, "%s", kRequirePinPrefix);
		return true;
#ifdef CONFIG_LOCK_SCHEDULES
	case AccessStorage::Type::WeekDaySchedule:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s%s/%u/%u", kAccessPrefix, kSchedulePrefix,
			       kScheduleWeekDaySuffix, limitedIndex, limitedSubindex);
		return true;
	case AccessStorage::Type::YearDaySchedule:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s%s/%u/%u", kAccessPrefix, kSchedulePrefix,
			       kScheduleYearDaySuffix, limitedIndex, limitedSubindex);
		return true;
	case AccessStorage::Type::HolidaySchedule:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s%s/%u", kAccessPrefix, kSchedulePrefix,
			       kScheduleHolidaySuffix, limitedIndex);
		return true;
	case AccessStorage::Type::WeekDayScheduleIndexes:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s%s/%u", kAccessPrefix, kScheduleCounterPrefix,
			       kScheduleWeekDaySuffix, limitedIndex);
		return true;
	case AccessStorage::Type::YearDayScheduleIndexes:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s%s/%u", kAccessPrefix, kScheduleCounterPrefix,
			       kScheduleYearDaySuffix, limitedIndex);
		return true;
	case AccessStorage::Type::HolidayScheduleIndexes:
		(void)snprintf(keyName, kMaxAccessName, "%s/%s%s", kAccessPrefix, kScheduleCounterPrefix,
			       kScheduleHolidaySuffix);
		return true;
#endif /* CONFIG_LOCK_SCHEDULES */
	default:
		break;
	}

	return false;
}
} /* namespace */

bool AccessStorage::Init()
{
	static Nrf::PersistentStorageNode rootNode{ kAccessPrefix, strlen(kAccessPrefix) };

	return Nrf::PSErrorCode::Success == Nrf::GetPersistentStorage().PSInit(&rootNode);
}

void AccessStorage::FactoryReset()
{
	Nrf::GetPersistentStorage().PSFactoryReset();
}

bool AccessStorage::Store(Type storageType, const void *data, size_t dataSize, uint16_t index, uint16_t subindex)
{
	if (data == nullptr || !PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ keyName, strlen(keyName) + 1 };
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

	Nrf::PersistentStorageNode node{ keyName, strlen(keyName) + 1 };
	Nrf::PSErrorCode result = Nrf::GetPersistentStorage().PSLoad(&node, data, dataSize, outSize);

	return (Nrf::PSErrorCode::Success == result);
}

bool AccessStorage::Remove(Type storageType, uint16_t index, uint16_t subindex)
{
	if (!PrepareKeyName(storageType, index, subindex)) {
		return false;
	}

	Nrf::PersistentStorageNode node{ keyName, strlen(keyName) + 1 };
	Nrf::PSErrorCode result = Nrf::GetPersistentStorage().PSRemove(&node);

	return (Nrf::PSErrorCode::Success == result);
}
