/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file access_storage_psa_ps.cpp
 *
 * Provides the implementation of AccessStorage class that utilizes the PSA Protected Storage API.
 *
 * This implementation uses the PSA PS UID range of size 2^32, which starts at a configurable offset.
 * The offset within the range is composed of a type and two optional indexes, which are packed into a uint32_t.
 * The type is encoded within a uint8_t as follows:
 * - 0: User
 * - 1: UsersIndexes
 * - 2: Credential
 * - 3: CredentialsIndexes
 * - 4: RequirePIN
 * - 5: WeekDaySchedule
 * - 6: WeekDayScheduleIndexes
 * - 7: YearDaySchedule
 * - 8: YearDayScheduleIndexes
 * - 9: HolidaySchedule
 * - 10: HolidayScheduleIndexes
 *
 * For example, the UID offset for the credential with type Fingerprint (3) and index 10 is 0x0103000a.
 */

#include "access_storage.h"

#include "access_data_types.h"
#include "access_storage_print.h"

#include <zephyr/sys/__assert.h>

#include <psa/protected_storage.h>

namespace
{
constexpr psa_storage_uid_t kUIDRangeStart = CONFIG_LOCK_ACCESS_STORAGE_PROTECTED_STORAGE_UID_OFFSET;

static_assert(static_cast<uint32_t>(kUIDRangeStart) == 0, "UID offset must fit in the upper 32 bits of the 64-bit UID");

template <size_t Offset> uint32_t PackIntegers()
{
	return 0;
}

template <size_t Offset = sizeof(uint32_t), class Integer, class... Integers>
uint32_t PackIntegers(Integer current, Integers... others)
{
	static_assert(Offset >= sizeof(Integer), "Cannot pack integers into uint32_t");

	constexpr size_t NewOffset = Offset - sizeof(Integer);

	return (static_cast<uint32_t>(current) << (NewOffset * 8)) | PackIntegers<NewOffset>(others...);
}

psa_storage_uid_t MakeUIDOffset(AccessStorage::Type type, uint16_t index, uint16_t subindex)
{
	using UserIndex = uint16_t;
	using CredentialType = DoorLockData::CredentialTypeIndex;
	using CredentialIndex = uint16_t;
#ifdef CONFIG_LOCK_SCHEDULES
	using ScheduleIndex = uint8_t;
#endif

	switch (type) {
	case AccessStorage::Type::User:
		return PackIntegers(type, static_cast<UserIndex>(index));
	case AccessStorage::Type::UsersIndexes:
		return PackIntegers(type);
	case AccessStorage::Type::Credential:
		return PackIntegers(type, static_cast<CredentialType>(index), static_cast<CredentialIndex>(subindex));
	case AccessStorage::Type::CredentialsIndexes:
		return PackIntegers(type, static_cast<CredentialType>(index));
	case AccessStorage::Type::RequirePIN:
		return PackIntegers(type);
#ifdef CONFIG_LOCK_SCHEDULES
	case AccessStorage::Type::WeekDaySchedule:
	case AccessStorage::Type::YearDaySchedule:
		return PackIntegers(type, static_cast<UserIndex>(index), static_cast<ScheduleIndex>(subindex));
	case AccessStorage::Type::WeekDayScheduleIndexes:
	case AccessStorage::Type::YearDayScheduleIndexes:
		return PackIntegers(type, static_cast<UserIndex>(index));
	case AccessStorage::Type::HolidaySchedule:
		return PackIntegers(type, static_cast<ScheduleIndex>(index));
	case AccessStorage::Type::HolidayScheduleIndexes:
		return PackIntegers(type);
#endif /* CONFIG_LOCK_SCHEDULES */
	default:
		__ASSERT_NO_MSG(false);
		return 0;
	}
}

psa_storage_uid_t MakeUID(AccessStorage::Type type, uint16_t index, uint16_t subindex)
{
	return kUIDRangeStart + MakeUIDOffset(type, index, subindex);
}
} /* namespace */

bool AccessStorage::Init()
{
	return true;
}

bool AccessStorage::Store(Type type, const void *data, size_t dataSize, uint16_t index, uint16_t subindex)
{
	if (data == nullptr) {
		return false;
	}

	psa_storage_uid_t uid = MakeUID(type, index, subindex);
	psa_status_t status = psa_ps_set(uid, dataSize, data, PSA_STORAGE_FLAG_NONE);

#ifdef CONFIG_LOCK_PRINT_STORAGE_STATUS
	PrintAccessDataStored(type, dataSize, status == PSA_SUCCESS);
#endif

	return status == PSA_SUCCESS;
}

bool AccessStorage::Load(Type type, void *data, size_t dataSize, size_t &outSize, uint16_t index, uint16_t subindex)
{
	if (data == nullptr) {
		return false;
	}

	psa_storage_uid_t uid = MakeUID(type, index, subindex);
	psa_status_t status = psa_ps_get(uid, 0, dataSize, data, &outSize);

	if (status != PSA_SUCCESS) {
		outSize = 0;
		return false;
	}

	return true;
}

bool AccessStorage::Remove(Type type, uint16_t index, uint16_t subindex)
{
	psa_storage_uid_t uid = MakeUID(type, index, subindex);
	psa_status_t status = psa_ps_remove(uid);

	return status == PSA_SUCCESS;
}
