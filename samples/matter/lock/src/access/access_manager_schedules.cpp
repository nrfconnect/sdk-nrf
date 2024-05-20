/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "access_data_types.h"
#include "access_manager.h"
#include "access_storage.h"

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(cr_manager, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace DoorLockData;

template <CredentialsBits CRED_BIT_MASK>
DlStatus AccessManager<CRED_BIT_MASK>::GetWeekDaySchedule(uint8_t weekdayIndex, uint16_t userIndex,
							  EmberAfPluginDoorLockWeekDaySchedule &schedule)
{
	VerifyOrReturnError(userIndex > 0 && userIndex <= CONFIG_LOCK_MAX_NUM_USERS, DlStatus::kFailure);
	VerifyOrReturnError(weekdayIndex > 0 && weekdayIndex <= CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER,
			    DlStatus::kFailure);
	VerifyOrReturnError(!mWeekDaySchedule[userIndex - 1][weekdayIndex - 1].mAvailable, DlStatus::kNotFound);

	if (CHIP_NO_ERROR != mWeekDaySchedule[userIndex - 1][weekdayIndex - 1].ConvertToPlugin(schedule)) {
		return DlStatus::kNotFound;
	}

	return DlStatus::kSuccess;
}

template <CredentialsBits CRED_BIT_MASK>
DlStatus AccessManager<CRED_BIT_MASK>::SetWeekDaySchedule(uint8_t weekdayIndex, uint16_t userIndex,
							  DlScheduleStatus status, DaysMaskMap daysMask,
							  uint8_t startHour, uint8_t startMinute, uint8_t endHour,
							  uint8_t endMinute)
{
	VerifyOrReturnError(userIndex > 0 && userIndex <= CONFIG_LOCK_MAX_NUM_USERS, DlStatus::kFailure);
	VerifyOrReturnError(weekdayIndex > 0 && weekdayIndex <= CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER,
			    DlStatus::kFailure);

	auto &schedule = mWeekDaySchedule[userIndex - 1][weekdayIndex - 1];

	if (DlScheduleStatus::kAvailable == status && !schedule.mAvailable) {
		schedule.mAvailable = true;
		memset(schedule.mData.mRaw, 0, DoorLockData::WeekDaySchedule::RequiredBufferSize());
		if (!AccessStorage::Instance().Remove(AccessStorage::Type::WeekDaySchedule, userIndex, weekdayIndex)) {
			LOG_ERR("Cannot remove the WeekDay schedule");
			return DlStatus::kFailure;
		}
		return DlStatus::kSuccess;
	}

	VerifyOrReturnError(schedule.mAvailable, DlStatus::kOccupied);

	schedule.mData.mFields.mDaysMask = static_cast<uint8_t>(daysMask);
	schedule.mData.mFields.mStartHour = startHour;
	schedule.mData.mFields.mStartMinute = startMinute;
	schedule.mData.mFields.mEndHour = endHour;
	schedule.mData.mFields.mEndMinute = endMinute;
	schedule.mAvailable = false;

	uint8_t scheduleSerialized[DoorLockData::WeekDaySchedule::RequiredBufferSize()] = { 0 };
	size_t serializedSize = schedule.Serialize(scheduleSerialized, sizeof(scheduleSerialized));

	uint8_t scheduleIndexesSerialized[WeekDayScheduleIndexes::ScheduleList::RequiredBufferSize()] = { 0 };
	size_t serializedIndexesSize{ 0 };

	if (0 < serializedSize) {
		auto &scheduleIndexes = mWeekDayScheduleIndexes.Get(userIndex);
		if (CHIP_ERROR_BUFFER_TOO_SMALL == scheduleIndexes.AddIndex(weekdayIndex)) {
			return DlStatus::kFailure;
		}

		serializedIndexesSize =
			scheduleIndexes.Serialize(scheduleIndexesSerialized, sizeof(scheduleIndexesSerialized));
	}

	/* Store to persistent storage */
	if (0 < serializedSize && 0 < serializedIndexesSize) {
		if (!AccessStorage::Instance().Store(AccessStorage::Type::WeekDaySchedule, scheduleSerialized,
						     serializedSize, userIndex, weekdayIndex)) {
			LOG_ERR("Cannot store WeekDaySchedule");
			return DlStatus::kFailure;
		} else if (!AccessStorage::Instance().Store(AccessStorage::Type::WeekDayScheduleIndexes,
							    scheduleIndexesSerialized, serializedIndexesSize,
							    userIndex)) {
			LOG_ERR("Cannot store WeekDaySchedule counter. The persistent database will be corrupted.");
			return DlStatus::kFailure;
		}
	}

	return DlStatus::kSuccess;
}

template <CredentialsBits CRED_BIT_MASK>
DlStatus AccessManager<CRED_BIT_MASK>::GetYearDaySchedule(uint8_t yearDayIndex, uint16_t userIndex,
							  EmberAfPluginDoorLockYearDaySchedule &schedule)
{
	VerifyOrReturnError(userIndex > 0 && userIndex <= CONFIG_LOCK_MAX_NUM_USERS, DlStatus::kFailure);
	VerifyOrReturnError(yearDayIndex > 0 && yearDayIndex <= CONFIG_LOCK_MAX_YEARDAY_SCHEDULES_PER_USER,
			    DlStatus::kFailure);
	VerifyOrReturnError(!mYearDaySchedule[userIndex - 1][yearDayIndex - 1].mAvailable, DlStatus::kNotFound);

	if (CHIP_NO_ERROR != mYearDaySchedule[userIndex - 1][yearDayIndex - 1].ConvertToPlugin(schedule)) {
		return DlStatus::kNotFound;
	}

	return DlStatus::kSuccess;
}

template <CredentialsBits CRED_BIT_MASK>
DlStatus AccessManager<CRED_BIT_MASK>::SetYearDaySchedule(uint8_t yeardayIndex, uint16_t userIndex,
							  DlScheduleStatus status, uint32_t localStartTime,
							  uint32_t localEndTime)
{
	VerifyOrReturnError(userIndex > 0 && userIndex <= CONFIG_LOCK_MAX_NUM_USERS, DlStatus::kFailure);
	VerifyOrReturnError(yeardayIndex > 0 && yeardayIndex <= CONFIG_LOCK_MAX_YEARDAY_SCHEDULES_PER_USER,
			    DlStatus::kFailure);

	auto &schedule = mYearDaySchedule[userIndex - 1][yeardayIndex - 1];

	if (DlScheduleStatus::kAvailable == status && !schedule.mAvailable) {
		schedule.mAvailable = true;
		memset(schedule.mData.mRaw, 0, DoorLockData::YearDaySchedule::RequiredBufferSize());
		if (!AccessStorage::Instance().Remove(AccessStorage::Type::YearDaySchedule, userIndex, yeardayIndex)) {
			LOG_ERR("Cannot remove the YearDay schedule");
			return DlStatus::kFailure;
		}
		return DlStatus::kSuccess;
	}

	VerifyOrReturnError(schedule.mAvailable, DlStatus::kOccupied);

	schedule.mData.mFields.mLocalStartTime = localStartTime;
	schedule.mData.mFields.mLocalEndTime = localEndTime;
	schedule.mAvailable = false;

	uint8_t scheduleSerialized[DoorLockData::YearDaySchedule::RequiredBufferSize()] = { 0 };
	size_t serializedSize = schedule.Serialize(scheduleSerialized, sizeof(scheduleSerialized));

	uint8_t scheduleIndexesSerialized[YearDayScheduleIndexes::ScheduleList::RequiredBufferSize()] = { 0 };
	size_t serializedIndexesSize{ 0 };

	if (0 < serializedSize) {
		auto &scheduleIndexes = mYearDayScheduleIndexes.Get(userIndex);
		if (CHIP_ERROR_BUFFER_TOO_SMALL == scheduleIndexes.AddIndex(yeardayIndex)) {
			return DlStatus::kFailure;
		}

		serializedIndexesSize =
			scheduleIndexes.Serialize(scheduleIndexesSerialized, sizeof(scheduleIndexesSerialized));
	}

	/* Store to persistent storage */
	if (0 < serializedSize && 0 < serializedIndexesSize) {
		if (!AccessStorage::Instance().Store(AccessStorage::Type::YearDaySchedule, scheduleSerialized,
						     serializedSize, userIndex, yeardayIndex)) {
			LOG_ERR("Cannot store YearDaySchedule");
			return DlStatus::kFailure;
		} else if (!AccessStorage::Instance().Store(AccessStorage::Type::YearDayScheduleIndexes,
							    scheduleIndexesSerialized, serializedIndexesSize,
							    userIndex)) {
			LOG_ERR("Cannot store YearDaySchedule counter. The persistent database will be corrupted.");
			return DlStatus::kFailure;
		}
	}

	return DlStatus::kSuccess;
}

template <CredentialsBits CRED_BIT_MASK>
DlStatus AccessManager<CRED_BIT_MASK>::GetHolidaySchedule(uint8_t holidayIndex,
							  EmberAfPluginDoorLockHolidaySchedule &schedule)
{
	VerifyOrReturnError(holidayIndex > 0 && holidayIndex <= CONFIG_LOCK_MAX_HOLIDAY_SCHEDULES, DlStatus::kFailure);
	VerifyOrReturnError(!mHolidaySchedule[holidayIndex - 1].mAvailable, DlStatus::kNotFound);

	if (CHIP_NO_ERROR != mHolidaySchedule[holidayIndex - 1].ConvertToPlugin(schedule)) {
		return DlStatus::kNotFound;
	}

	return DlStatus::kSuccess;
}

template <CredentialsBits CRED_BIT_MASK>
DlStatus AccessManager<CRED_BIT_MASK>::SetHolidaySchedule(uint8_t holidayIndex, DlScheduleStatus status,
							  uint32_t localStartTime, uint32_t localEndTime,
							  OperatingModeEnum operatingMode)
{
	VerifyOrReturnError(holidayIndex > 0 && holidayIndex <= CONFIG_LOCK_MAX_HOLIDAY_SCHEDULES, DlStatus::kFailure);

	auto &schedule = mHolidaySchedule[holidayIndex - 1];

	if (DlScheduleStatus::kAvailable == status && !schedule.mAvailable) {
		schedule.mAvailable = true;
		memset(schedule.mData.mRaw, 0, DoorLockData::HolidaySchedule::RequiredBufferSize());
		if (!AccessStorage::Instance().Remove(AccessStorage::Type::HolidaySchedule, holidayIndex)) {
			LOG_ERR("Cannot remove the Holiday schedule");
			return DlStatus::kFailure;
		}
		return DlStatus::kSuccess;
	}

	VerifyOrReturnError(schedule.mAvailable, DlStatus::kOccupied);

	schedule.mData.mFields.mLocalStartTime = localStartTime;
	schedule.mData.mFields.mLocalEndTime = localEndTime;
	schedule.mData.mFields.mOperatingMode = static_cast<uint8_t>(operatingMode);
	schedule.mAvailable = false;

	uint8_t scheduleSerialized[DoorLockData::HolidaySchedule::RequiredBufferSize()] = { 0 };
	size_t serializedSize = schedule.Serialize(scheduleSerialized, sizeof(scheduleSerialized));

	uint8_t scheduleIndexesSerialized[HolidayScheduleIndexes::RequiredBufferSize()] = { 0 };
	size_t serializedIndexesSize{ 0 };

	if (0 < serializedSize) {
		if (CHIP_ERROR_BUFFER_TOO_SMALL == mHolidayScheduleIndexes.AddIndex(holidayIndex)) {
			return DlStatus::kFailure;
		}

		serializedIndexesSize =
			mHolidayScheduleIndexes.Serialize(scheduleIndexesSerialized, sizeof(scheduleIndexesSerialized));
	}

	/* Store to persistent storage */
	if (0 < serializedSize && 0 < serializedIndexesSize) {
		if (!AccessStorage::Instance().Store(AccessStorage::Type::HolidaySchedule, scheduleSerialized,
						     serializedSize, holidayIndex)) {
			LOG_ERR("Cannot store HolidaySchedule");
			return DlStatus::kFailure;
		} else if (!AccessStorage::Instance().Store(AccessStorage::Type::HolidayScheduleIndexes,
							    scheduleIndexesSerialized, serializedIndexesSize)) {
			LOG_ERR("Cannot store HolidaySchedule counter. The persistent database will be corrupted.");
			return DlStatus::kFailure;
		}
	}

	return DlStatus::kSuccess;
}

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::LoadSchedulesFromPersistentStorage()
{
	/* Load all Schedules from persistent storage */
	bool scheduleFound{ false };
	size_t outSize{ 0 };
	uint16_t scheduleIndex = 0;

	uint8_t scheduleWeekDayIndexesSerialized[WeekDayScheduleIndexes::ScheduleList::RequiredBufferSize()] = { 0 };
	uint8_t scheduleWeekDayData[DoorLockData::WeekDaySchedule::RequiredBufferSize()] = { 0 };

	uint8_t scheduleYearDayIndexesSerialized[YearDayScheduleIndexes::ScheduleList::RequiredBufferSize()] = { 0 };
	uint8_t scheduleYearDayData[DoorLockData::YearDaySchedule::RequiredBufferSize()] = { 0 };

	for (size_t userIndex = 1; userIndex <= CONFIG_LOCK_MAX_NUM_USERS; userIndex++) {
		scheduleFound = false;
		/* Load WeekDay schedules */
		if (AccessStorage::Instance().Load(AccessStorage::Type::WeekDayScheduleIndexes,
						   scheduleWeekDayIndexesSerialized,
						   sizeof(scheduleWeekDayIndexesSerialized), outSize, userIndex)) {
			if (CHIP_NO_ERROR == mWeekDayScheduleIndexes.Get(userIndex).Deserialize(
						     scheduleWeekDayIndexesSerialized, outSize)) {
				scheduleFound = true;
			}
		}

		if (scheduleFound) {
			auto &scheduleIndexes = mWeekDayScheduleIndexes.Get(userIndex);
			for (size_t scheduleIdx = 0; scheduleIdx < scheduleIndexes.mList.mLength; scheduleIdx++) {
				/* Read the actual index from the indexList */
				scheduleIndex = scheduleIndexes.mList.mIndexes[scheduleIdx];
				if (AccessStorage::Instance().Load(AccessStorage::Type::WeekDaySchedule,
								   scheduleWeekDayData, sizeof(scheduleWeekDayData),
								   outSize, userIndex, scheduleIndex)) {
					if (CHIP_NO_ERROR !=
					    mWeekDaySchedule[userIndex - 1][scheduleIndex - 1].Deserialize(
						    scheduleWeekDayData, outSize)) {
						LOG_ERR("Cannot deserialize WeekDay Schedule %d for user index: %d",
							scheduleIndex, userIndex);
					}
#if CONFIG_LOCK_ENABLE_DEBUG
					else {
						PrintSchedule(ScheduleType::WeekDay, scheduleIndex, userIndex);
					}
#endif
				}
			}
		}

		scheduleFound = false;

		/* Load YearDay schedules */
		if (AccessStorage::Instance().Load(AccessStorage::Type::YearDayScheduleIndexes,
						   scheduleYearDayIndexesSerialized,
						   sizeof(scheduleYearDayIndexesSerialized), outSize, userIndex)) {
			if (CHIP_NO_ERROR == mYearDayScheduleIndexes.Get(userIndex).Deserialize(
						     scheduleYearDayIndexesSerialized, outSize)) {
				scheduleFound = true;
			}
		}

		if (scheduleFound) {
			auto &scheduleIndexes = mYearDayScheduleIndexes.Get(userIndex);
			for (size_t scheduleIdx = 0; scheduleIdx < scheduleIndexes.mList.mLength; scheduleIdx++) {
				/* Read the actual index from the indexList */
				scheduleIndex = scheduleIndexes.mList.mIndexes[scheduleIdx];
				if (!AccessStorage::Instance().Load(AccessStorage::Type::YearDaySchedule,
								    scheduleYearDayData, sizeof(scheduleYearDayData),
								    outSize, userIndex, scheduleIndex)) {
				} else {
					if (CHIP_NO_ERROR !=
					    mYearDaySchedule[userIndex - 1][scheduleIndex - 1].Deserialize(
						    scheduleYearDayData, outSize)) {
						LOG_ERR("Cannot deserialize YearDay Schedule %d for user index: %d",
							scheduleIndex, userIndex);
					}
#if CONFIG_LOCK_ENABLE_DEBUG
					else {
						PrintSchedule(ScheduleType::YearDay, scheduleIndex, userIndex);
					}
#endif
				}
			}
		}
	}

	scheduleFound = false;

	/* Load Holiday schedules */
	uint8_t scheduleHolidayIndexesSerialized[HolidayScheduleIndexes::RequiredBufferSize()] = { 0 };
	uint8_t scheduleHolidayData[DoorLockData::HolidaySchedule::RequiredBufferSize()] = { 0 };

	if (AccessStorage::Instance().Load(AccessStorage::Type::HolidayScheduleIndexes,
					   scheduleHolidayIndexesSerialized, sizeof(scheduleHolidayIndexesSerialized),
					   outSize, 0)) {
		if (CHIP_NO_ERROR == mHolidayScheduleIndexes.Deserialize(scheduleHolidayIndexesSerialized, outSize)) {
			scheduleFound = true;
		}
	}
	for (size_t scheduleIdx = 0; scheduleIdx < mHolidayScheduleIndexes.mList.mLength; scheduleIdx++) {
		/* Read the actual index from the indexList */
		scheduleIndex = mHolidayScheduleIndexes.mList.mIndexes[scheduleIdx];
		if (AccessStorage::Instance().Load(AccessStorage::Type::HolidaySchedule, scheduleHolidayData,
						   sizeof(scheduleHolidayData), outSize, scheduleIndex)) {
			if (CHIP_NO_ERROR !=
			    mHolidaySchedule[scheduleIndex - 1].Deserialize(scheduleHolidayData, outSize)) {
				LOG_ERR("Cannot deserialize Holiday Schedule %d ", scheduleIndex);
			}
#if CONFIG_LOCK_ENABLE_DEBUG
			else {
				PrintSchedule(ScheduleType::Holiday, scheduleIndex);
			}
#endif
		}
	}
}

#ifdef CONFIG_LOCK_ENABLE_DEBUG

template <CredentialsBits CRED_BIT_MASK>
void AccessManager<CRED_BIT_MASK>::PrintSchedule(ScheduleType scheduleType, uint16_t scheduleIndex, uint16_t userIndex)
{
	switch (scheduleType) {
	case ScheduleType::WeekDay: {
		auto *schedule = &mWeekDaySchedule[userIndex - 1][scheduleIndex - 1];
		if (schedule) {
			LOG_INF("-- WeekDay Schedule %d for user %d, days %08u, start %d:%d, end %d:%d", scheduleIndex,
				userIndex, static_cast<uint8_t>(schedule->mData.mFields.mDaysMask),
				schedule->mData.mFields.mStartHour, schedule->mData.mFields.mStartMinute,
				schedule->mData.mFields.mEndHour, schedule->mData.mFields.mEndMinute);
		}
	} break;
	case ScheduleType::YearDay: {
		auto *schedule = &mYearDaySchedule[userIndex - 1][scheduleIndex - 1];
		if (schedule) {
			LOG_INF("-- YearDay Schedule %d for user %d, starting time: %u, ending time: %u", scheduleIndex,
				userIndex, schedule->mData.mFields.mLocalStartTime,
				schedule->mData.mFields.mLocalEndTime);
		}
	} break;
	case ScheduleType::Holiday: {
		auto *schedule = &mHolidaySchedule[scheduleIndex - 1];
		if (schedule) {
			LOG_INF("-- Holiday Schedule %d, Local start time: %u, Local end time: %u, Operating mode: %d",
				scheduleIndex, schedule->mData.mFields.mLocalStartTime,
				schedule->mData.mFields.mLocalEndTime,
				static_cast<uint8_t>(schedule->mData.mFields.mOperatingMode));
		}
	} break;
	default:
		break;
	}
}

#endif

/* Explicitly instantiate supported template variants to avoid linker errors. */
template class AccessManager<DoorLockData::PIN>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID | DoorLockData::FINGER>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID | DoorLockData::FINGER | DoorLockData::VEIN>;
template class AccessManager<DoorLockData::PIN | DoorLockData::RFID | DoorLockData::FINGER | DoorLockData::VEIN |
			     DoorLockData::FACE>;
