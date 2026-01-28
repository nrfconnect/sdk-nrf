/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "access_storage_print.h"

#ifdef CONFIG_SETTINGS_NVS
#include <zephyr/fs/nvs.h>
#elif CONFIG_SETTINGS_ZMS || CONFIG_SETTINGS_ZMS_LEGACY
#include <zephyr/fs/zms.h>
#endif /* CONFIG_SETTINGS_NVS */
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(storage_manager, CONFIG_CHIP_APP_LOG_LEVEL);

static bool GetStorageFreeSpace(size_t &freeBytes)
{
	void *storage = nullptr;
	int status = settings_storage_get(&storage);
	if (status != 0 || !storage) {
		LOG_ERR("AccessStorage: Cannot read free space: %d", status);
		return false;
	}
#ifdef CONFIG_SETTINGS_NVS
	freeBytes = nvs_calc_free_space(static_cast<nvs_fs *>(storage));
#elif CONFIG_SETTINGS_ZMS || CONFIG_SETTINGS_ZMS_LEGACY
	freeBytes = zms_calc_free_space(static_cast<zms_fs *>(storage));
#endif /* CONFIG_SETTINGS_NVS */
	return true;
}

static const char *TypeToString(AccessStorage::Type type)
{
	switch (type) {
	case AccessStorage::Type::User:
		return "user";
	case AccessStorage::Type::Credential:
		return "credential";
	case AccessStorage::Type::UsersIndexes:
		return "user idx";
	case AccessStorage::Type::CredentialsIndexes:
		return "credential idx";
#ifdef CONFIG_LOCK_SCHEDULES
	case AccessStorage::Type::WeekDaySchedule:
	case AccessStorage::Type::YearDaySchedule:
	case AccessStorage::Type::HolidaySchedule:
		return "schedule";
	case AccessStorage::Type::WeekDayScheduleIndexes:
	case AccessStorage::Type::YearDayScheduleIndexes:
	case AccessStorage::Type::HolidayScheduleIndexes:
		return "schedule idx";
#endif /* CONFIG_LOCK_SCHEDULES */
	default:
		return "other";
	}
}

void PrintAccessDataStored(AccessStorage::Type type, size_t dataSize, bool success)
{
	if (!success) {
		return;
	}

	LOG_DBG("AccessStorage: Stored %s of size: %d bytes", TypeToString(type), dataSize);

	size_t storageFreeSpace;
	if (GetStorageFreeSpace(storageFreeSpace)) {
		LOG_DBG("AccessStorage: Free space: %d bytes", storageFreeSpace);
	}
}
