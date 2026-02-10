/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Class for storing and retrieving door lock access items from the persistent storage.
 *
 * There are several access item types that can be stored using this class. Each of them may use up to two indexes.
 * For example, a credential is referenced by its:
 * - type: PIN, RFID or fingerprint
 * - credential index: from 1 to CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE.
 * On the other hand, a user is referenced by its user index: from 1 to CONFIG_LOCK_MAX_NUM_USERS.
 *
 * Each user has a list of credentials that are assigned to them.
 *
 * The new access item should be written with the new index if the previous one is already occupied.
 */

class AccessStorage {
public:
	enum class Type : uint8_t {
		User,
		UsersIndexes,
		Credential,
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
};
