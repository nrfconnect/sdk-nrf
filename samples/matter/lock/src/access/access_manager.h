/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/core/ClusterEnums.h>

#include "access_data_types.h"

template <DoorLockData::CredentialsBits CRED_BIT_MASK> class AccessManager {
public:
	/**
	 * @brief Signature of the callback fired when the credential is set.
	 *
	 * The execution of the callback code might need to be scheduled to another thread
	 * to avoid blocking of the Matter messaging.
	 */
	using SetCredentialCallback = void (*)(uint16_t credentialIndex, CredentialTypeEnum credentialType,
					       uint32_t userId);

	/**
	 * @brief Signature of the callback fired when the credential is cleared.
	 *
	 * Warning: This callback is not thread-safe.
	 * The credentialData chip::ByteSpan object that contains a pointer to the credential data
	 * will be deleted after invoking this callback (the pointer will point to cleared buffer).
	 * The caller must ensure that the credentialData is being copied in the same thread as invoking
	 * this callback, or copy the data to a local buffer before delegating an operation on credentialData to other
	 * threads.
	 *
	 * The execution of the callback code might need to be scheduled to another thread
	 * to avoid blocking of the Matter messaging.
	 */
	using ClearCredentialCallback = void (*)(uint16_t credentialIndex, CredentialTypeEnum credentialType,
						 uint32_t userId, chip::ByteSpan credentialData);

	/**
	 * @brief Signature of the callback fired when the custom validation of the credential should be performed.
	 *
	 * This callback allows to deliver incoming credential data to the Application layer in order to perform
	 * application-specific validation of credential.
	 *
	 * The execution of the callback code should be scheduled to another thread
	 * to avoid blocking of the Matter messaging.
	 */
	using ValidateCredentialCallback = void (*)(CredentialTypeEnum credentialType,
						    chip::MutableByteSpan credentialData);

	/**
	 * @brief CredentialManager singleton
	 *
	 * @return CredentialManager single instance
	 */
	static AccessManager &Instance()
	{
		static AccessManager sInstance;
		return sInstance;
	}

	/**
	 * @brief Initialize the CredentialManager.
	 *
	 * Initializes internal credential database and loads persistently stored data.
	 *
	 * @param setCredentialClbk user callback triggered whenever the credential is being set.
	 * @param clearCredentialCallback user callback triggered whenever the credential is being cleared.
	 */
	void Init(SetCredentialCallback setCredentialClbk = nullptr,
		  ClearCredentialCallback clearCredentialCallback = nullptr,
		  ValidateCredentialCallback validateCredentialCallback = nullptr);

	/**
	 * @brief Get door lock user information.
	 *
	 * @param userIndex user index starting from 1.
	 * @param user user data.
	 * @return true on success, false otherwise.
	 */
	bool GetUserInfo(uint16_t userIndex, EmberAfPluginDoorLockUserInfo &user);

	/**
	 * @brief Set door lock user information.
	 *
	 * Note that the Programming PIN credential type (CredentialTypeEnum::kProgrammingPIN)
	 * is not currently supported.
	 *
	 * @param userIndex user index starting from 1.
	 * @param creator an idex of the fabric which is creating the user.
	 * @param modifier an idex of the fabric which is modifying the user.
	 * @param userName a name of the user.
	 * @param uniqueId an unique user identifier.
	 * @param userStatus a status of the user (e.g. available, occupied).
	 * @param userType a type of the user (e.g. user with unrestricted access, scheduled access etc.).
	 * @param credentialRule credential rule (how many credentials are required for lock operation).
	 * @param credentials credential data assigned to the user.
	 * @param totalCredentials overall size of the passed credentials.
	 * @return true on success, false otherwise.
	 */
	bool SetUser(uint16_t userIndex, chip::FabricIndex creator, chip::FabricIndex modifier,
		     const chip::CharSpan &userName, uint32_t uniqueId, UserStatusEnum userStatus,
		     UserTypeEnum userType, CredentialRuleEnum credentialRule, const CredentialStruct *credentials,
		     size_t totalCredentials);

	/**
	 * @brief Get door lock credential.
	 *
	 * @param credentialIndex credential index starting from 1.
	 * @param credentialType type of the credential (PIN, RFID, etc.).
	 * @param credential credential data to be retrieved.
	 * @return true on success, false otherwise.
	 */
	bool GetCredential(uint16_t credentialIndex, CredentialTypeEnum credentialType,
			   EmberAfPluginDoorLockCredentialInfo &credential);

	/**
	 * @brief Set door lock credential.
	 *
	 * Note that the Programming PIN credential type (CredentialTypeEnum::kProgrammingPIN)
	 * is not currently supported.
	 *
	 * @param credentialIndex credential index starting from 1.
	 * @param creator an idex of the fabric which is creating the credential.
	 * @param modifier an idex of the fabric which is modifying the credential.
	 * @param credentialStatus a status of the credential (e.g. available, occupied).
	 * @param credentialType a type of the credential (PIN, RFID etc.).
	 * @param secret a secret row data.
	 * @return true on success, false otherwise.
	 */
	bool SetCredential(uint16_t credentialIndex, chip::FabricIndex creator, chip::FabricIndex modifier,
			   DlCredentialStatus credentialStatus, CredentialTypeEnum credentialType,
			   const chip::ByteSpan &secret);

#ifdef CONFIG_LOCK_SCHEDULES

	/**
	 * @brief Get the Week Day Schedule.
	 *
	 * @param weekdayIndex schedule index that was saved utilizing setWeekDaySchedule method.
	 * @param userIndex user index that contains the specific Week day schedule.
	 * @param schedule Schedule data to be retrieved.
	 * @retval DlStatus::kNotFound if schedule has not been found.
	 * @retval DlStatus::kFailure if userIndex or weekdayIndex is out of scope.
	 * @retval DlStatus::kSuccess if schedule has been retrieved.
	 */
	DlStatus GetWeekDaySchedule(uint8_t weekdayIndex, uint16_t userIndex,
				    EmberAfPluginDoorLockWeekDaySchedule &schedule);

	/**
	 * @brief Set the Week Day Schedule object
	 *
	 * @param weekdayIndex Schedule index to be set starting from 1.
	 * @param userIndex User index to be set starting from 1
	 * @param status New status of the schedule slot (occupied/available).
	 * @param daysMask Indicates the days of the week the Week Day schedule applies for.
	 * @param startHour Starting hour for the Week Day schedule.
	 * @param startMinute Starting minute for the Week Day schedule
	 * @param endHour Ending hour for the Week Day schedule.
	 * @param endMinute Ending minute for the Week Day schedule.
	 * @retval DlStatus::kFailure if userIndex or weekdayIndex is out of scope.
	 * @retval DlStatus::kSuccess if schedule has been set.
	 */
	DlStatus SetWeekDaySchedule(uint8_t weekdayIndex, uint16_t userIndex, DlScheduleStatus status,
				    DaysMaskMap daysMask, uint8_t startHour, uint8_t startMinute, uint8_t endHour,
				    uint8_t endMinute);

	/**
	 * @brief Get the Year Day Schedule.
	 *
	 * @param yearDayIndex ndex of the year day schedule to access. It is guaranteed to be within limits declared
	 * in the spec for year day schedule (from 1 up to NumberOfYearDaySchedulesSupportedPerUser)
	 * @param userIndex Index of the user to get year day schedule.
	 * @param schedule Schedule data to be retrieved.
	 * @retval DlStatus::kNotFound if schedule has not been found.
	 * @retval DlStatus::kFailure if userIndex or weekdayIndex is out of scope.
	 * @retval DlStatus::kSuccess if schedule has been retrieved.
	 */
	DlStatus GetYearDaySchedule(uint8_t yearDayIndex, uint16_t userIndex,
				    EmberAfPluginDoorLockYearDaySchedule &schedule);

	/**
	 * @brief Set the Year Day Schedule object
	 *
	 * @param yearDayIndex Index of the year day schedule to access. It is guaranteed to be within limits declared
	 * in the spec for year day schedule (from 1 up to NumberOfYearDaySchedulesSupportedPerUser)
	 * @param userIndex Index of the user to get year day schedule.
	 * @param status New status of the schedule slot (occupied/available).
	 * @param localStartTime The starting time for the Year Day schedule in Epoch Time in Seconds with local time
	 * offset based on the local timezone and DST offset on the day represented by the value.
	 * @param localEndTime The ending time for the Year Day schedule in Epoch Time in Seconds with local time offset
	 * based on the local timezone and DST offset on the day represented by the value
	 * @retval DlStatus::kFailure if userIndex or weekdayIndex is out of scope.
	 * @retval DlStatus::kSuccess if schedule has been set.
	 */
	DlStatus SetYearDaySchedule(uint8_t yearDayIndex, uint16_t userIndex, DlScheduleStatus status,
				    uint32_t localStartTime, uint32_t localEndTime);

	/**
	 * @brief Get the Holiday Schedule object
	 *
	 * @param holidayIndex Index of the holiday schedule to access. It is guaranteed to be within limits declared in
	 * the spec for holiday schedule (from 1 up to NumberOfHolidaySchedulesSupported)
	 * @param schedule Schedule data to be retrieved.
	 * @retval DlStatus::kNotFound if schedule has not been found.
	 * @retval DlStatus::kFailure if weekdayIndex is out of scope.
	 * @retval DlStatus::kSuccess if schedule has been retrieved.
	 */
	DlStatus GetHolidaySchedule(uint8_t holidayIndex, EmberAfPluginDoorLockHolidaySchedule &schedule);

	/**
	 * @brief Set the Holiday Schedule object
	 *
	 * @param holidayIndex Index of the holiday schedule to access. It is guaranteed to be within limits declared in
	 * the spec for holiday schedule (from 1 up to NumberOfHolidaySchedulesSupported).
	 * @param status New status of the schedule slot (occupied/available).
	 * @param localStartTime The starting time for the Year Day schedule in Epoch Time in Seconds with local time
	 * offset based on the local timezone and DST offset on the day represented by the value.
	 * @param localEndTime The ending time for the Year Day schedule in Epoch Time in Seconds with local time offset
	 * based on the local timezone and DST offset on the day represented by the value.
	 * @param operatingMode The operating mode to use during this Holiday schedule start/end time.
	 * @retval DlStatus::kFailure if weekdayIndex is out of scope.
	 * @retval DlStatus::kSuccess if schedule has been set.
	 */
	DlStatus SetHolidaySchedule(uint8_t holidayIndex, DlScheduleStatus status, uint32_t localStartTime,
				    uint32_t localEndTime, OperatingModeEnum operatingMode);

#endif /* CONFIG_LOCK_SCHEDULES */

	/**
	 * @brief PIN code validator.
	 *
	 * @param pinCode PIN code data.
	 * @param err specific error code enumeration.
	 * @return true on success, false otherwise.
	 */
	bool ValidatePIN(const Optional<chip::ByteSpan> &pinCode, OperationErrorEnum &err);
	bool ValidateCustom(CredentialTypeEnum type, chip::MutableByteSpan &secret);

	/**
	 * @brief Initialize credential database.
	 *
	 * Sets all users and credential slots to default values, including empty secret data.
	 *
	 */
	void InitializeAllCredentials();

	/**
	 * @brief Set the RequirePINforRemoteOperation flag local state.
	 *
	 * Sets the local copy of RequirePINforRemoteOperation door lock attribute
	 * and stores it in the persistent storage.
	 *
	 */
	void SetRequirePIN(bool require);

	/**
	 * @brief Get the RequirePINforRemoteOperation flab local state.
	 *
	 * Returns the local copy of RequirePINforRemoteOperation door lock attribute.
	 *
	 */
	bool GetRequirePIN() { return mRequirePINForRemoteOperation; };

#ifdef CONFIG_LOCK_LEAVE_FABRIC_CLEAR_CREDENTIAL
	/**
	 * @brief Clear all credentials from the fabric which is currently being removed
	 *
	 * @return true on success, false otherwise
	 */
	bool ClearAllCredentialsFromFabric();
#endif

#ifdef CONFIG_LOCK_ENABLE_DEBUG
	/* DEBUG API allowing to retrieve internally stored credentials and user data */
	void PrintCredential(CredentialTypeEnum type, uint16_t index);
	const char *GetCredentialName(CredentialTypeEnum type);
	void PrintUser(uint16_t userIndex);
#ifdef CONFIG_LOCK_SCHEDULES
	enum class ScheduleType : uint8_t { WeekDay, YearDay, Holiday };
	/* @param userIndex is only needed for ScheduleType WeekDay and YearDay */
	void PrintSchedule(ScheduleType scheduleType, uint16_t scheduleIndex, uint16_t userIndex = 0);
#endif /* CONFIG_LOCK_SCHEDULES */
#endif /* CONFIG_LOCK_ENABLE_DEBUG */

private:
	struct CredentialsIndexes {
		using CredentialList = DoorLockData::IndexList<CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE>;
		CredentialList mCredentialsIndexes[DoorLockData::CredentialTypeIndex::Max];

		CredentialList &Get(DoorLockData::CredentialTypeIndex type) { return mCredentialsIndexes[type - 1]; }
	};

	using UsersIndexes = DoorLockData::IndexList<CONFIG_LOCK_MAX_NUM_USERS>;

#ifdef CONFIG_LOCK_SCHEDULES
	struct WeekDayScheduleIndexes {
		using ScheduleList = DoorLockData::IndexList<CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER>;
		ScheduleList mScheduleIndexes[CONFIG_LOCK_MAX_NUM_USERS];

		ScheduleList &Get(uint16_t userIndex) { return mScheduleIndexes[userIndex - 1]; }
	};

	struct YearDayScheduleIndexes {
		using ScheduleList = DoorLockData::IndexList<CONFIG_LOCK_MAX_YEARDAY_SCHEDULES_PER_USER>;
		ScheduleList mScheduleIndexes[CONFIG_LOCK_MAX_NUM_USERS];

		ScheduleList &Get(uint16_t userIndex) { return mScheduleIndexes[userIndex - 1]; }
	};

	using HolidayScheduleIndexes = DoorLockData::IndexList<CONFIG_LOCK_MAX_HOLIDAY_SCHEDULES>;
#endif /* CONFIG_LOCK_SCHEDULES */

	AccessManager() = default;
	~AccessManager() = default;
	AccessManager(const AccessManager &) = delete;
	AccessManager(AccessManager &&) = delete;
	AccessManager &operator=(AccessManager &) = delete;

	void InitializeUsers();
	void LoadUsersFromPersistentStorage();
	void LoadCredentialsFromPersistentStorage();
#ifdef CONFIG_LOCK_SCHEDULES
	void InitializeSchedules();
	void LoadSchedulesFromPersistentStorage();
#endif

	static CHIP_ERROR GetCredentialUserId(uint16_t credentialIndex, CredentialTypeEnum credentialType,
					      uint32_t &userId);

	/* Similarly to SetCredential(), credentialIndex starts from 1 */
	static bool DoSetCredential(DoorLockData::Credential &credential, uint16_t credentialIndex,
				    chip::FabricIndex creator, chip::FabricIndex modifier,
				    DlCredentialStatus credentialStatus, CredentialTypeEnum credentialType,
				    const chip::ByteSpan &secret);

#ifdef CONFIG_LOCK_LEAVE_FABRIC_CLEAR_CREDENTIAL
	static bool ClearCredential(DoorLockData::Credential &credential, uint8_t credIdx);
#endif

	SetCredentialCallback mSetCredentialCallback{ nullptr };
	ClearCredentialCallback mClearCredentialCallback{ nullptr };
	ValidateCredentialCallback mValidateCredentialCallback{ nullptr };

	DoorLockData::Credentials<CRED_BIT_MASK> mCredentials{};
	CredentialsIndexes mCredentialsIndexes{};

	DoorLockData::User mUsers[CONFIG_LOCK_MAX_NUM_USERS] = {};
	UsersIndexes mUsersIndexes{};

#ifdef CONFIG_LOCK_SCHEDULES
	DoorLockData::WeekDaySchedule mWeekDaySchedule[CONFIG_LOCK_MAX_NUM_USERS]
						      [CONFIG_LOCK_MAX_WEEKDAY_SCHEDULES_PER_USER] = {};
	WeekDayScheduleIndexes mWeekDayScheduleIndexes = {};

	DoorLockData::YearDaySchedule mYearDaySchedule[CONFIG_LOCK_MAX_NUM_USERS]
						      [CONFIG_LOCK_MAX_YEARDAY_SCHEDULES_PER_USER] = {};
	YearDayScheduleIndexes mYearDayScheduleIndexes = {};

	DoorLockData::HolidaySchedule mHolidaySchedule[CONFIG_LOCK_MAX_HOLIDAY_SCHEDULES];
	HolidayScheduleIndexes mHolidayScheduleIndexes = {};
#endif /* CONFIG_LOCK_SCHEDULES */

	bool mRequirePINForRemoteOperation{ false };
};
