/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "access_data_types.h"

namespace DoorLockData
{

void pack(void *buff, const void *data, size_t dataSize, size_t &offset)
{
	memcpy(reinterpret_cast<uint8_t *>(buff) + offset, data, dataSize);
	offset += dataSize;
}

void unpack(void *data, size_t dataSize, const void *buff, size_t &offset)
{
	memcpy(data, reinterpret_cast<const uint8_t *>(buff) + offset, dataSize);
	offset += dataSize;
}

CHIP_ERROR Credential::FillFromPlugin(EmberAfPluginDoorLockCredentialInfo &credentialInfo)
{
	if (credentialInfo.credentialData.size() < kMaxCredentialLength) {
		mInfo.mFields.mStatus = static_cast<uint8_t>(credentialInfo.status);
		mInfo.mFields.mCredentialType = static_cast<uint8_t>(credentialInfo.credentialType);
		mInfo.mFields.mCreationSource = static_cast<uint8_t>(credentialInfo.creationSource);
		mInfo.mFields.mCreatedBy = static_cast<uint8_t>(credentialInfo.createdBy);
		mInfo.mFields.mModificationSource = static_cast<uint8_t>(credentialInfo.modificationSource);
		mInfo.mFields.mLastModifiedBy = static_cast<uint8_t>(credentialInfo.lastModifiedBy);
		mSecret.mDataLength = credentialInfo.credentialData.size();
		memcpy(mSecret.mData, credentialInfo.credentialData.data(), credentialInfo.credentialData.size());
		return CHIP_NO_ERROR;
	}
	return CHIP_ERROR_BUFFER_TOO_SMALL;
}

CHIP_ERROR Credential::ConvertToPlugin(EmberAfPluginDoorLockCredentialInfo &credentialInfo) const
{
	if (mSecret.mDataLength < RequiredBufferSize()) {
		credentialInfo.status = static_cast<DlCredentialStatus>(mInfo.mFields.mStatus);
		credentialInfo.credentialType = static_cast<CredentialTypeEnum>(mInfo.mFields.mCredentialType);
		credentialInfo.credentialData = chip::ByteSpan(mSecret.mData, mSecret.mDataLength);
		credentialInfo.creationSource = static_cast<DlAssetSource>(mInfo.mFields.mCreationSource);
		credentialInfo.createdBy = static_cast<chip::FabricIndex>(mInfo.mFields.mCreatedBy);
		credentialInfo.modificationSource = static_cast<DlAssetSource>(mInfo.mFields.mModificationSource);
		credentialInfo.lastModifiedBy = static_cast<chip::FabricIndex>(mInfo.mFields.mLastModifiedBy);
		return CHIP_NO_ERROR;
	}
	return CHIP_ERROR_BUFFER_TOO_SMALL;
}

size_t Credential::Serialize(void *buff, size_t buffSize)
{
	if (!buff || buffSize < RequiredBufferSize()) {
		return 0;
	}
	size_t offset = 0;

	pack(buff, mInfo.mRaw, sizeof(mInfo.mRaw), offset);
	pack(buff, &mSecret.mDataLength, sizeof(mSecret.mDataLength), offset);
	pack(buff, mSecret.mData, mSecret.mDataLength, offset);

	return offset;
}

CHIP_ERROR Credential::Deserialize(const void *buff, size_t buffSize)
{
	if (!buff) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (buffSize > RequiredBufferSize()) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	size_t offset = 0;

	unpack(mInfo.mRaw, sizeof(mInfo.mRaw), buff, offset);
	unpack(&mSecret.mDataLength, sizeof(mSecret.mDataLength), buff, offset);
	if (kMaxCredentialLength < mSecret.mDataLength) {
		/* Read data length cannot be parsed because is too big */
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}
	unpack(mSecret.mData, mSecret.mDataLength, buff, offset);

	return CHIP_NO_ERROR;
}

CHIP_ERROR User::FillFromPlugin(EmberAfPluginDoorLockUserInfo &userInfo)
{
	if (DOOR_LOCK_USER_NAME_BUFFER_SIZE > userInfo.userName.size() &&
	    CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER >= (userInfo.credentials.size() / sizeof(CredentialStruct))) {
		mName.mSize = userInfo.userName.size();
		memcpy(mName.mValue, userInfo.userName.data(), userInfo.userName.size());
		mOccupiedCredentials.mSize = userInfo.credentials.size() * sizeof(CredentialStruct);
		memcpy(mOccupiedCredentials.mData, userInfo.credentials.data(), mOccupiedCredentials.mSize);
		mInfo.mFields.mUserUniqueId = userInfo.userUniqueId;
		mInfo.mFields.mUserStatus = static_cast<uint8_t>(userInfo.userStatus);
		mInfo.mFields.mUserType = static_cast<uint8_t>(userInfo.userType);
		mInfo.mFields.mCredentialRule = static_cast<uint8_t>(userInfo.credentialRule);
		mInfo.mFields.mCreationSource = static_cast<uint8_t>(userInfo.creationSource);
		mInfo.mFields.mCreatedBy = static_cast<uint8_t>(userInfo.createdBy);
		mInfo.mFields.mModificationSource = static_cast<uint8_t>(userInfo.modificationSource);
		mInfo.mFields.mLastModifiedBy = static_cast<uint8_t>(userInfo.lastModifiedBy);

		return CHIP_NO_ERROR;
	}
	return CHIP_ERROR_INVALID_ARGUMENT;
}

CHIP_ERROR User::ConvertToPlugin(EmberAfPluginDoorLockUserInfo &userInfo) const
{
	if (CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER >= (mOccupiedCredentials.mSize / sizeof(CredentialStruct)) &&
	    DOOR_LOCK_USER_NAME_BUFFER_SIZE >= mName.mSize) {
		userInfo.userName = chip::CharSpan(mName.mValue, mName.mSize);
		userInfo.credentials = chip::Span<const CredentialStruct>(
			mOccupiedCredentials.mData, mOccupiedCredentials.mSize / sizeof(CredentialStruct));
		userInfo.userUniqueId = mInfo.mFields.mUserUniqueId;
		userInfo.userStatus = static_cast<UserStatusEnum>(mInfo.mFields.mUserStatus);
		userInfo.userType = static_cast<UserTypeEnum>(mInfo.mFields.mUserType);
		userInfo.credentialRule = static_cast<CredentialRuleEnum>(mInfo.mFields.mCredentialRule);
		userInfo.creationSource = static_cast<DlAssetSource>(mInfo.mFields.mCreationSource);
		userInfo.createdBy = static_cast<chip::FabricIndex>(mInfo.mFields.mCreatedBy);
		userInfo.modificationSource = static_cast<DlAssetSource>(mInfo.mFields.mModificationSource);
		userInfo.lastModifiedBy = static_cast<chip::FabricIndex>(mInfo.mFields.mLastModifiedBy);
		return CHIP_NO_ERROR;
	}
	return CHIP_ERROR_INTERNAL;
}

size_t User::Serialize(void *buff, size_t buffSize)
{
	if (!buff || buffSize < RequiredBufferSize()) {
		return 0;
	}

	size_t offset = 0;
	pack(buff, mInfo.mRaw, sizeof(mInfo.mRaw), offset);
	pack(buff, &mOccupiedCredentials.mSize, sizeof(mOccupiedCredentials.mSize), offset);
	pack(buff, mOccupiedCredentials.mData, mOccupiedCredentials.mSize, offset);
	pack(buff, &mName.mSize, sizeof(mName.mSize), offset);
	pack(buff, mName.mValue, mName.mSize, offset);

	return offset;
}

CHIP_ERROR User::Deserialize(const void *buff, size_t buffSize)
{
	if (!buff) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (buffSize > RequiredBufferSize()) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	size_t offset = 0;

	unpack(mInfo.mRaw, sizeof(mInfo.mRaw), buff, offset);
	unpack(&mOccupiedCredentials.mSize, sizeof(mOccupiedCredentials.mSize), buff, offset);
	if ((mOccupiedCredentials.mSize / sizeof(CredentialStruct)) > CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER) {
		/* Read data length cannot be parsed because is too big */
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}
	unpack(mOccupiedCredentials.mData, mOccupiedCredentials.mSize, buff, offset);
	unpack(&mName.mSize, sizeof(mName.mSize), buff, offset);
	if (mName.mSize >= DOOR_LOCK_USER_NAME_BUFFER_SIZE) {
		/* Read data length cannot be parsed because is too big. */
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}
	unpack(mName.mValue, mName.mSize, buff, offset);

	return CHIP_NO_ERROR;
}

#ifdef CONFIG_LOCK_SCHEDULES

CHIP_ERROR WeekDaySchedule::FillFromPlugin(EmberAfPluginDoorLockWeekDaySchedule &plugin)
{
	static_assert(sizeof(EmberAfPluginDoorLockWeekDaySchedule) == RequiredBufferSize());

	mData.mFields.mDaysMask = static_cast<uint8_t>(plugin.daysMask);
	mData.mFields.mStartHour = plugin.startHour;
	mData.mFields.mStartMinute = plugin.startMinute;
	mData.mFields.mEndHour = plugin.endHour;
	mData.mFields.mEndMinute = plugin.endMinute;

	return CHIP_NO_ERROR;
}

CHIP_ERROR YearDaySchedule::FillFromPlugin(EmberAfPluginDoorLockYearDaySchedule &plugin)
{
	static_assert(sizeof(EmberAfPluginDoorLockYearDaySchedule) == RequiredBufferSize());

	mData.mFields.mLocalStartTime = plugin.localStartTime;
	mData.mFields.mLocalEndTime = plugin.localEndTime;

	return CHIP_NO_ERROR;
}

CHIP_ERROR HolidaySchedule::FillFromPlugin(EmberAfPluginDoorLockHolidaySchedule &plugin)
{
	static_assert(sizeof(EmberAfPluginDoorLockHolidaySchedule) == RequiredBufferSize());

	mData.mFields.mLocalStartTime = plugin.localStartTime;
	mData.mFields.mLocalEndTime = plugin.localEndTime;
	mData.mFields.mOperatingMode = static_cast<uint8_t>(plugin.operatingMode);

	return CHIP_NO_ERROR;
}

CHIP_ERROR WeekDaySchedule::ConvertToPlugin(EmberAfPluginDoorLockWeekDaySchedule &plugin) const
{
	static_assert(sizeof(EmberAfPluginDoorLockWeekDaySchedule) == RequiredBufferSize());

	plugin.daysMask = static_cast<DaysMaskMap>(mData.mFields.mDaysMask);
	plugin.startHour = mData.mFields.mStartHour;
	plugin.startMinute = mData.mFields.mStartMinute;
	plugin.endHour = mData.mFields.mEndHour;
	plugin.endMinute = mData.mFields.mEndMinute;

	return CHIP_NO_ERROR;
}

CHIP_ERROR YearDaySchedule::ConvertToPlugin(EmberAfPluginDoorLockYearDaySchedule &plugin) const
{
	static_assert(sizeof(EmberAfPluginDoorLockYearDaySchedule) == RequiredBufferSize());

	plugin.localStartTime = mData.mFields.mLocalStartTime;
	plugin.localEndTime = mData.mFields.mLocalEndTime;

	return CHIP_NO_ERROR;
}

CHIP_ERROR HolidaySchedule::ConvertToPlugin(EmberAfPluginDoorLockHolidaySchedule &plugin) const
{
	static_assert(sizeof(EmberAfPluginDoorLockHolidaySchedule) == RequiredBufferSize());

	plugin.localStartTime = mData.mFields.mLocalStartTime;
	plugin.localEndTime = mData.mFields.mLocalEndTime;
	plugin.operatingMode = static_cast<OperatingModeEnum>(mData.mFields.mOperatingMode);

	return CHIP_NO_ERROR;
}

size_t WeekDaySchedule::Serialize(void *buff, size_t buffSize)
{
	if (!buff || buffSize < RequiredBufferSize()) {
		return 0;
	}

	memcpy(buff, mData.mRaw, sizeof(mData.mRaw));

	return sizeof(mData.mRaw);
}

size_t YearDaySchedule::Serialize(void *buff, size_t buffSize)
{
	if (!buff || buffSize < RequiredBufferSize()) {
		return 0;
	}

	memcpy(buff, mData.mRaw, sizeof(mData.mRaw));

	return sizeof(mData.mRaw);
}

size_t HolidaySchedule::Serialize(void *buff, size_t buffSize)
{
	if (!buff || buffSize < RequiredBufferSize()) {
		return 0;
	}

	memcpy(buff, mData.mRaw, sizeof(mData.mRaw));

	return sizeof(mData.mRaw);
}

CHIP_ERROR WeekDaySchedule::Deserialize(const void *buff, size_t buffSize)
{
	if (!buff) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (buffSize > RequiredBufferSize()) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(mData.mRaw, buff, sizeof(mData.mRaw));

	return CHIP_NO_ERROR;
}

CHIP_ERROR YearDaySchedule::Deserialize(const void *buff, size_t buffSize)
{
	if (!buff) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (buffSize > RequiredBufferSize()) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(mData.mRaw, buff, sizeof(mData.mRaw));

	return CHIP_NO_ERROR;
}

CHIP_ERROR HolidaySchedule::Deserialize(const void *buff, size_t buffSize)
{
	if (!buff) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (buffSize > RequiredBufferSize()) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(mData.mRaw, buff, sizeof(mData.mRaw));

	return CHIP_NO_ERROR;
}

#endif /* CONFIG_LOCK_SCHEDULES */

} /* namespace DoorLockData */
