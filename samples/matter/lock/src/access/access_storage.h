/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <persistent_storage/persistent_storage_common.h>

/**
 * @brief Class to manage access storage
 *
 * Eventually access items will be store to the Zephyr settings to be persistent (regardless of the used persistent
 * storage backend). There are several credential types: PIN, RFID, Fingerprint etc. and each of them is declared as an
 * array with the maximum size defined by Kconfig ...MAX_NUM_CREDENTIALS_PER_USER. Each user has a list of credentials
 * that are assign to them. The new access item should be written with the new index if the previous one is already
 * occupied.
 *
 * Agreed the following settings key convention:
 *
 * /cr/
 *    /<credential type (int)>/
 *                           / cr_idxs (bytes)
 *                           / <credential index (int)>/
 *                                                     / <credential data (bytes)>
 *    /usr_idxs (bytes)
 *    /usr/
 *        / <user index (int)> /
 *                             / <user data (bytes)>
 *
 *    /sch_<type (w - weekday, y - yearday, h - holiday)>/
 *    		/sch_idx_<type (w - weekday, y - yearday, h - holiday)> (bytes)/
 *        			/ <user index (int)> /
 *                       		/ <schedule index (int)> /
 *		   		                 			/ <schedule data (bytes)>
 *
 */

class AccessStorage {
public:
	enum class Type : uint8_t {
		User,
		Credential,
		UsersIndexes,
		CredentialsIndexes,
		RequirePIN,
#ifdef CONFIG_LOCK_SCHEDULES
		WeekDaySchedule,
		WeekDayScheduleIndexes,
		YearDaySchedule,
		YearDayScheduleIndexes,
		HolidaySchedule,
		HolidayScheduleIndexes
#endif /* CONFIG_LOCK_SCHEDULES */
	};

	/**
	 * @brief Initialize the persistent storage.
	 *
	 * @return true if success.
	 * @return false otherwise.
	 */
	bool Init();

	/**
	 * @brief Store the entry into the persistent storage.
	 *
	 * @param storageType depending on that there will be a different key set.
	 * @param data data to store under a specific key.
	 * @param dataSize a size of input buffer.
	 * @param index a number to be appended to key (<key>/index/).
	 * @param subindex a number to be appended to key and index (<key>/index/subindex).
	 * @return true if data has been written successfully.
	 * @return false an error occurred.
	 */
	bool Store(Type storageType, const void *data, size_t dataSize, uint16_t index = 0, uint16_t subindex = 0);

	/**
	 * @brief Load an entry from the persistent storage.
	 *
	 * @param storageType depending on that there will be a different key set.
	 * @param data data buffer to load from a specific key.
	 * @param dataSize a size of input buffer.
	 * @param index a number to be appended to key (<key>/index).
	 * @param subindex a number to be appended to key and index (<key>/index/subindex).
	 * @return true if data has been loaded successfully.
	 * @return false an error occurred.
	 */
	bool Load(Type storageType, void *data, size_t dataSize, size_t &outSize, uint16_t index = 0,
		  uint16_t subindex = 0);

	/**
	 * @brief Remove an entry from the persistent storage.
	 *
	 * @param storageType depending on that there will be a different key set.
	 * @param index a number to be appended to key (<key>/index).
	 * @param subindex a number to be appended to key and index (<key>/index/subindex).
	 * @return true if data has been removed successfully.
	 * @return false an error occurred.
	 */
	bool Remove(Type storageType, uint16_t index = 0, uint16_t subindex = 0);

	static AccessStorage &Instance()
	{
		static AccessStorage sInstance;
		return sInstance;
	}

private:
	constexpr static auto kAccessPrefix = "cr";
	constexpr static auto kAccessCounterPrefix = "cr_idxs";
	constexpr static auto kUserPrefix = "usr";
	constexpr static auto kUserCounterPrefix = "usr_idxs";
	constexpr static auto kRequirePinPrefix = "pin_req";
#ifdef CONFIG_LOCK_SCHEDULES
	constexpr static auto kSchedulePrefix = "sch";
	constexpr static auto kScheduleWeekDaySuffix = "_w";
	constexpr static auto kScheduleYearDaySuffix = "_y";
	constexpr static auto kScheduleHolidaySuffix = "_h";
	constexpr static auto kScheduleCounterPrefix = "sch_idxs";
#endif /* CONFIG_LOCK_SCHEDULES */
	constexpr static auto kMaxAccessName = Nrf::PersistentStorageNode::kMaxKeyNameLength;

	char mKeyName[AccessStorage::kMaxAccessName];

	bool PrepareKeyName(Type storageType, uint16_t index, uint16_t subindex);
};
