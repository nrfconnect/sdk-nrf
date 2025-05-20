/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "access_manager.h"
#include "access_storage.h"

#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(cr_manager, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace DoorLockData;

template <CredentialsBits CRED_BIT_MASK>
bool AccessManager<CRED_BIT_MASK>::GetUserInfo(uint16_t userIndex, EmberAfPluginDoorLockUserInfo &user)
{
	/* userIndex is guaranteed by the caller to be between 1 and CONFIG_LOCK_MAX_NUM_USERS */
	VerifyOrReturnError(userIndex > 0 && userIndex <= CONFIG_LOCK_MAX_NUM_USERS, false);
	if (CHIP_NO_ERROR != mUsers[userIndex - 1].ConvertToPlugin(user)) {
		return false;
	}

	return true;
}

template <CredentialsBits CRED_BIT_MASK>
bool AccessManager<CRED_BIT_MASK>::SetUser(uint16_t userIndex, FabricIndex creator, FabricIndex modifier,
					   const CharSpan &userName, uint32_t uniqueId, UserStatusEnum userStatus,
					   UserTypeEnum userType, CredentialRuleEnum credentialRule,
					   const CredentialStruct *credentials, size_t totalCredentials)
{
	/* userIndex is guaranteed by the caller to be between 1 and CONFIG_LOCK_MAX_NUM_USERS */
	VerifyOrReturnError(userIndex > 0 && userIndex <= CONFIG_LOCK_MAX_NUM_USERS, false);
	auto &user = mUsers[userIndex - 1];

	VerifyOrReturnError(userName.size() <= DOOR_LOCK_MAX_USER_NAME_SIZE, false);
	VerifyOrReturnError(totalCredentials <= CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER, false);

	user.mName.mSize = userName.size();
	memcpy(user.mName.mValue, userName.data(), userName.size());

	for (size_t i = 0; i < totalCredentials; ++i) {
		auto *currentCredentials = credentials + i;
		memcpy(&user.mOccupiedCredentials.mData[i], currentCredentials, sizeof(CredentialStruct));
	}
	user.mOccupiedCredentials.mSize = totalCredentials * sizeof(CredentialStruct);

	user.mInfo.mFields.mUserUniqueId = uniqueId;
	user.mInfo.mFields.mUserStatus = static_cast<uint8_t>(userStatus);
	user.mInfo.mFields.mUserType = static_cast<uint8_t>(userType);
	user.mInfo.mFields.mCredentialRule = static_cast<uint8_t>(credentialRule);
	user.mInfo.mFields.mCreationSource = static_cast<uint8_t>(DlAssetSource::kMatterIM);
	user.mInfo.mFields.mCreatedBy = static_cast<uint8_t>(creator);
	user.mInfo.mFields.mModificationSource = static_cast<uint8_t>(DlAssetSource::kMatterIM);
	user.mInfo.mFields.mLastModifiedBy = static_cast<uint8_t>(modifier);

	uint8_t userSerialized[DoorLockData::User::RequiredBufferSize()] = { 0 };
	size_t serializedSize = user.Serialize(userSerialized, sizeof(userSerialized));

	uint8_t userIndexesSerialized[UsersIndexes::RequiredBufferSize()] = { 0 };
	size_t serializedIndexesSize{ 0 };

	/* Process the user indexes only if the user itself was properly serialized. */
	if (0 < serializedSize) {
		if (CHIP_ERROR_BUFFER_TOO_SMALL == mUsersIndexes.AddIndex(userIndex)) {
			return false;
		}
		serializedIndexesSize = mUsersIndexes.Serialize(userIndexesSerialized, sizeof(userIndexesSerialized));
	}

	/* Store to persistent storage */
	if (0 < serializedSize && 0 < serializedIndexesSize) {
		if (!AccessStorage::Instance().Store(AccessStorage::Type::User, userSerialized,
							  serializedSize, userIndex)) {
			LOG_ERR("Cannot store given User Idx: %d", userIndex);
			return false;
		} else {
			if (!AccessStorage::Instance().Store(AccessStorage::Type::UsersIndexes,
								  userIndexesSerialized, serializedIndexesSize, 0)) {
				LOG_ERR("Cannot store users counter. The persistent database will be corrupted.");
			}
		}
	} else {
		LOG_ERR("Cannot serialize given User Idx: %d", userIndex);
	}

	LOG_INF("Setting lock user %u: %s", static_cast<unsigned>(userIndex),
		userStatus == UserStatusEnum::kAvailable ? "available" : "occupied");

	return true;
}

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::LoadUsersFromPersistentStorage()
{
	/* Load all Users from persistent storage */
	uint8_t userIndexesSerialized[UsersIndexes::RequiredBufferSize()] = { 0 };
	uint8_t userData[DoorLockData::User::RequiredBufferSize()] = { 0 };
	bool usersFound{ false };
	size_t outSize{ 0 };

	if (!AccessStorage::Instance().Load(AccessStorage::Type::UsersIndexes, userIndexesSerialized,
						 sizeof(userIndexesSerialized), outSize, 0)) {
		LOG_INF("No users indexes stored");
	} else {
		if (CHIP_NO_ERROR == mUsersIndexes.Deserialize(userIndexesSerialized, outSize)) {
			usersFound = true;
		}
	}

	if (usersFound) {
		for (size_t idx = 0; idx < mUsersIndexes.mList.mLength; idx++) {
			/* Read the actual index from the indexList */
			uint16_t userIndex = mUsersIndexes.mList.mIndexes[idx];
			outSize = 0;
			if (AccessStorage::Instance().Load(AccessStorage::Type::User, userData,
								sizeof(userData), outSize, userIndex)) {
				if (CHIP_NO_ERROR != mUsers[userIndex - 1].Deserialize(userData, outSize)) {
					LOG_ERR("Cannot deserialize User index: %d", userIndex);
				}
			}
		}
	}
}

#ifdef CONFIG_LOCK_ENABLE_DEBUG

template <CredentialsBits CRED_BIT_MASK> void AccessManager<CRED_BIT_MASK>::PrintUser(uint16_t userIndex)
{
	auto *user = &mUsers[userIndex - 1];

	if (user) {
		LOG_INF("\t -- User %d: %s", userIndex, user->mName.mValue);
		LOG_INF("\t\t -- ID: %d", user->mInfo.mFields.mUserUniqueId);
		LOG_INF("\t\t -- Type: %d", user->mInfo.mFields.mUserType);
		LOG_INF("\t\t -- Status: %s",
			static_cast<UserStatusEnum>(user->mInfo.mFields.mUserStatus) == UserStatusEnum::kAvailable ?
				"available" :
				"occupied");
		LOG_INF("\t\t -- Credential Rule: %d", user->mInfo.mFields.mCredentialRule);
		LOG_INF("\t\t -- Creation Source: %d", user->mInfo.mFields.mCreationSource);
		LOG_INF("\t\t -- Created By: %d", user->mInfo.mFields.mCreatedBy);
		LOG_INF("\t\t -- Modification Source: %d", user->mInfo.mFields.mModificationSource);
		LOG_INF("\t\t -- Modified By: %d", user->mInfo.mFields.mLastModifiedBy);
		LOG_INF("\t\t -- Associated Credentials %d:",
			user->mOccupiedCredentials.mSize / sizeof(CredentialStruct));
		for (uint8_t i = 0; i < user->mOccupiedCredentials.mSize / sizeof(CredentialStruct); i++) {
			switch (user->mOccupiedCredentials.mData[i].credentialType) {
			case CredentialTypeEnum::kPin:
				LOG_INF("\t\t\t -- PIN | index: %d",
					user->mOccupiedCredentials.mData[i].credentialIndex);
				break;
			case CredentialTypeEnum::kRfid:
				LOG_INF("\t\t\t -- RFID | index: %d",
					user->mOccupiedCredentials.mData[i].credentialIndex);
				break;
			case CredentialTypeEnum::kFingerprint:
			case CredentialTypeEnum::kFingerVein:
				LOG_INF("\t\t\t -- Fingerprint | index: %d",
					user->mOccupiedCredentials.mData[i].credentialIndex);
				break;
			default:
				break;
			}
		}
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
