/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/core/ClusterEnums.h>

namespace DoorLockData
{

static constexpr size_t kMaxCredentialLength = CONFIG_LOCK_MAX_CREDENTIAL_LENGTH;

/**
 * @brief Packing to buffer
 *
 * Given `data` with size `dataSize` will be packed to the `buff` and the `offset` will be
 * increased by `dataSize` bytes.
 *
 * This function expects that `buff` and `data` pointers are valid and not NULL.
 *
 */
void pack(void *buff, const void *data, size_t dataSize, size_t &offset);

/**
 * @brief Unpacking from buffer to object
 *
 * `dataSize` amount of bytes from `buff` will be unpacked to the `data` and the `offset` will be
 * increased by `dataSize` bytes.
 *
 * This function expects that `buff` and `data` pointers are valid and not NULL.
 *
 */
void unpack(void *data, size_t dataSize, const void *buff, size_t &offset);

struct Credential {
	union Info {
		struct __attribute__((__packed__)) Fields {
			uint8_t mStatus;
			uint8_t mCredentialType;
			uint8_t mCreationSource;
			uint8_t mCreatedBy;
			uint8_t mModificationSource;
			uint8_t mLastModifiedBy;
		} mFields;
		uint8_t mRaw[sizeof(mFields)];
	};
	static_assert(sizeof(Info) == 6);

	struct Secret {
		size_t mDataLength;
		uint8_t mData[kMaxCredentialLength];
	};

	Info mInfo;
	Secret mSecret;

	/**
	 * @brief Get required size for single credential entry.
	 *
	 * This method can be used to prepare a proper serialization buffer.
	 *
	 * @return size of single credential entry.
	 */
	constexpr static size_t RequiredBufferSize() { return sizeof(Info) + sizeof(Secret); }

	/**
	 * @brief fill Credential entry with data from EmberAfPluginDoorLockCredentialInfo struct.
	 *
	 * Creating an Credential entry from EmberAfPluginDoorLockCredentialInfo struct is possible only if
	 * given index is lower than CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE and size of
	 * EmberAfPluginDoorLockCredentialInfo::credentialData is lower than kMaxCredentialLength.
	 *
	 *
	 * @param credentialInfo a reference to the plugin.
	 * @param index index within Credential object.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if provided credentialInfo contains incorrect data.
	 * @return CHIP_NO_ERROR if credential has been filled properly.
	 */
	CHIP_ERROR FillFromPlugin(EmberAfPluginDoorLockCredentialInfo &credentialInfo);

	/**
	 * @brief Convert whole object to EmberAfPluginDoorLockCredentialInfo structure.
	 *
	 * @param credentialInfo a reference to the plugin.
	 * @param index index within Credential object.
	 * @return CHIP_ERROR_INTERNAL if saved credential does not met requirements.
	 * @return CHIP_NO_ERROR if conversion has been finished successfully.
	 */
	CHIP_ERROR ConvertToPlugin(EmberAfPluginDoorLockCredentialInfo &credentialInfo) const;

	/**
	 * @brief Serialize all fields into data buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer to store serialized data.
	 * @param buffSize size of input buffer.
	 * @return size_t output serialized data length, 0 value means error.
	 */
	size_t Serialize(void *buff, size_t buffSize);

	/**
	 * @brief Deserialize all fields from given buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer containing serialized data (by invoking serialized method).
	 * @param buffSize size of output buffer.
	 * @param index current index of credential.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if provided buffSize is too small.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if arguments are wrong.
	 * @return CHIP_NO_ERROR if deserialization has been finished successfully.
	 */
	CHIP_ERROR Deserialize(const void *buff, size_t buffSize);
};

struct WeekDaySchedule {
	union Data {
		struct Fields {
			uint8_t mDaysMask;
			uint8_t mStartHour;
			uint8_t mStartMinute;
			uint8_t mEndHour;
			uint8_t mEndMinute;
		} mFields;
		uint8_t mRaw[sizeof(mFields)];
	};
	static_assert(sizeof(Data) == 5);

	Data mData;
	bool mAvailable = true;

	/**
	 * @brief Get required size for single credential entry.
	 *
	 * This method can be used to prepare a proper serialization buffer.
	 *
	 * @return size of single credential entry.
	 */
	constexpr static size_t RequiredBufferSize() { return sizeof(Data); }

	/**
	 * @brief fill User entry with data from the EmberAfPluginDoorLockWeekDaySchedule plugin
	 *
	 * @param plugin a reference to the plugin.
	 * @return CHIP_NO_ERROR if whole struct has been filled properly.
	 */
	CHIP_ERROR FillFromPlugin(EmberAfPluginDoorLockWeekDaySchedule &plugin);

	/**
	 * @brief Convert whole object to the EmberAfPluginDoorLockWeekDaySchedule plugin.
	 *
	 * @param plugin a reference to the plugin.
	 * @return CHIP_NO_ERROR if conversion has been finished successfully.
	 */
	CHIP_ERROR ConvertToPlugin(EmberAfPluginDoorLockWeekDaySchedule &plugin) const;

	/**
	 * @brief Serialize all fields into data buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer to store serialized data.
	 * @param buffSize size of input buffer.
	 * @return size_t output serialized data length, 0 value means error.
	 */
	size_t Serialize(void *buff, size_t buffSize);

	/**
	 * @brief Deserialize all fields from given buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer containing serialized data (by invoking serialized method).
	 * @param buffSize size of output buffer.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if provided buffSize is too small.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if arguments are wrong.
	 * @return CHIP_NO_ERROR if deserialization has been finished successfully.
	 */
	CHIP_ERROR Deserialize(const void *buff, size_t buffSize);
};

struct YearDaySchedule {
	union Data {
		struct Fields {
			uint32_t mLocalStartTime;
			uint32_t mLocalEndTime;
		} mFields;
		uint8_t mRaw[sizeof(mFields)];
	};
	static_assert(sizeof(Data) == 8);

	Data mData;
	bool mAvailable = true;

	/**
	 * @brief Get required size for single credential entry.
	 *
	 * This method can be used to prepare a proper serialization buffer.
	 *
	 * @return size of single credential entry.
	 */
	constexpr static size_t RequiredBufferSize() { return sizeof(Data); }

	/**
	 * @brief fill User entry with data from one  the EmberAfPluginDoorLockYearDaySchedule plugin.
	 *
	 * @param plugin a reference to the plugin.
	 * @return CHIP_NO_ERROR if whole struct has been filled properly.
	 */
	CHIP_ERROR FillFromPlugin(EmberAfPluginDoorLockYearDaySchedule &plugin);

	/**
	 * @brief Convert whole object to the EmberAfPluginDoorLockYearDaySchedule plugin.
	 *
	 * @param plugin a reference to the plugin.
	 * @return CHIP_NO_ERROR if conversion has been finished successfully.
	 */
	CHIP_ERROR ConvertToPlugin(EmberAfPluginDoorLockYearDaySchedule &plugin) const;

	/**
	 * @brief Serialize all fields into data buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer to store serialized data.
	 * @param buffSize size of input buffer.
	 * @return size_t output serialized data length, 0 value means error.
	 */
	size_t Serialize(void *buff, size_t buffSize);

	/**
	 * @brief Deserialize all fields from given buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer containing serialized data (by invoking serialized method).
	 * @param buffSize size of output buffer.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if provided buffSize is too small.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if arguments are wrong.
	 * @return CHIP_NO_ERROR if deserialization has been finished successfully.
	 */
	CHIP_ERROR Deserialize(const void *buff, size_t buffSize);
};

struct HolidaySchedule {
	union Data {
		struct Fields {
			uint32_t mLocalStartTime;
			uint32_t mLocalEndTime;
			uint32_t mOperatingMode;
		} mFields;
		uint8_t mRaw[sizeof(mFields)];
	};
	static_assert(sizeof(Data) == 12);

	Data mData;
	bool mAvailable = true;

	/**
	 * @brief Get required size for single credential entry.
	 *
	 * This method can be used to prepare a proper serialization buffer.
	 *
	 * @return size of single credential entry.
	 */
	constexpr static size_t RequiredBufferSize() { return sizeof(Data); }

	/**
	 * @brief fill User entry with data from one  the EmberAfPluginDoorLockHolidaySchedule plugin.
	 *
	 * @param plugin a reference to the plugin.
	 * @return CHIP_NO_ERROR if whole struct has been filled properly.
	 */
	CHIP_ERROR FillFromPlugin(EmberAfPluginDoorLockHolidaySchedule &plugin);

	/**
	 * @brief Convert whole object to the EmberAfPluginDoorLockHolidaySchedule plugin.
	 *
	 * @param plugin a reference to the plugin.
	 * @return CHIP_NO_ERROR if conversion has been finished successfully.
	 */
	CHIP_ERROR ConvertToPlugin(EmberAfPluginDoorLockHolidaySchedule &plugin) const;

	/**
	 * @brief Serialize all fields into data buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer to store serialized data.
	 * @param buffSize size of input buffer.
	 * @return size_t output serialized data length, 0 value means error.
	 */
	size_t Serialize(void *buff, size_t buffSize);

	/**
	 * @brief Deserialize all fields from given buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer containing serialized data (by invoking serialized method).
	 * @param buffSize size of output buffer.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if provided buffSize is too small.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if arguments are wrong.
	 * @return CHIP_NO_ERROR if deserialization has been finished successfully.
	 */
	CHIP_ERROR Deserialize(const void *buff, size_t buffSize);
};

struct User {
	union Info {
		struct __attribute__((__packed__)) Fields {
			uint32_t mUserUniqueId;
			uint8_t mUserStatus;
			uint8_t mUserType;
			uint8_t mCredentialRule;
			uint8_t mCreationSource;
			uint8_t mCreatedBy;
			uint8_t mModificationSource;
			uint8_t mLastModifiedBy;
		} mFields;
		uint8_t mRaw[sizeof(mFields)];
	};
	static_assert(sizeof(Info) == 11);

	struct Credentials {
		size_t mSize;
		CredentialStruct mData[CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER];
	};
	struct Name {
		size_t mSize;
		char mValue[DOOR_LOCK_USER_NAME_BUFFER_SIZE];
	};

	Info mInfo;
	Credentials mOccupiedCredentials;
	Name mName;

	/**
	 * @brief Get required size for single credential entry.
	 *
	 * This method can be used to prepare a proper serialization buffer.
	 *
	 * @return size of single credential entry.
	 */
	constexpr static size_t RequiredBufferSize() { return sizeof(Info) + sizeof(Credentials) + sizeof(Name); }

	/**
	 * @brief fill User entry with data from EmberAfPluginDoorLockUserInfo struct.
	 *
	 * Filling an User entry from EmberAfPluginDoorLockUserInfo struct is possible only if
	 * given size of EmberAfPluginDoorLockUserInfo::userName is lower than DOOR_LOCK_USER_NAME_BUFFER_SIZE
	 * and size of EmberAfPluginDoorLockUserInfo::credentials is lower than
	 * CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_USER.
	 *
	 *
	 * @param userInfo a reference to the plugin.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if provided userInfo contains incorrect data.
	 * @return CHIP_NO_ERROR if whole struct has been filled properly.
	 */
	CHIP_ERROR FillFromPlugin(EmberAfPluginDoorLockUserInfo &userInfo);

	/**
	 * @brief Convert whole object to EmberAfPluginDoorLockUserInfo structure.
	 *
	 * @param credentialInfo a reference to the plugin.
	 * @return CHIP_ERROR_INTERNAL if saved user does not met requirements.
	 * @return CHIP_NO_ERROR if conversion has been finished successfully.
	 */
	CHIP_ERROR ConvertToPlugin(EmberAfPluginDoorLockUserInfo &userInfo) const;

	/**
	 * @brief Serialize all fields into data buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer to store serialized data.
	 * @param buffSize size of input buffer.
	 * @return size_t output serialized data length, 0 value means error.
	 */
	size_t Serialize(void *buff, size_t buffSize);

	/**
	 * @brief Deserialize all fields from given buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer containing serialized data (by invoking serialized method).
	 * @param buffSize size of output buffer.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if provided buffSize is too small.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if arguments are wrong.
	 * @return CHIP_NO_ERROR if deserialization has been finished successfully.
	 */
	CHIP_ERROR Deserialize(const void *buff, size_t buffSize);
};

template <uint16_t N> struct IndexList {
	struct __attribute__((__packed__)) List {
		uint16_t mLength{ 0 }; /* Number of elements. */
		uint16_t mIndexes[N] = { std::numeric_limits<uint16_t>::max() };
	} mList;

	static constexpr size_t kElementSize{ sizeof(mList.mIndexes[0]) };

	/**
	 * @brief Get required size for single IndexList entry.
	 *
	 * This method can be used to prepare a proper serialization buffer.
	 *
	 * @return size of single credential entry.
	 */
	constexpr static size_t RequiredBufferSize() { return sizeof(List); }

	/**
	 * @brief Serialize all fields into data buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer to store serialized data.
	 * @param buffSize size of input buffer.
	 * @return size_t output serialized data length, 0 value means error.
	 */
	size_t Serialize(void *buff, size_t buffSize);

	/**
	 * @brief Deserialize all fields from given buffer.
	 *
	 * `buffSize` must be set to at least the RequiredBufferSize size.
	 *
	 * @param buff buffer containing serialized data (by invoking serialized method).
	 * @param buffSize size of output buffer.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if provided buffSize is too small.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if arguments are wrong.
	 * @return CHIP_NO_ERROR if deserialization has been finished successfully.
	 */
	CHIP_ERROR Deserialize(const void *buff, size_t buffSize);

	/**
	 * @brief Add index to the list
	 *
	 * @param index index to be added.
	 * @return CHIP_ERROR_BUFFER_TOO_SMALL if there is no space to add another index.
	 * @return CHIP_ERROR_INVALID_ARGUMENT if the given `index` already exists in the container.
	 * @return CHIP_NO_ERROR if the index has been added successfully.
	 */
	CHIP_ERROR AddIndex(uint16_t index);
};

template <uint16_t N> inline size_t IndexList<N>::Serialize(void *buff, size_t buffSize)
{
	if (!buff || buffSize < RequiredBufferSize()) {
		return 0;
	}

	size_t offset = 0;
	pack(buff, mList.mIndexes, kElementSize * mList.mLength, offset);

	return offset;
}

template <uint16_t N> inline CHIP_ERROR IndexList<N>::Deserialize(const void *buff, size_t buffSize)
{
	if (!buff) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	if (buffSize > RequiredBufferSize()) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}

	size_t offset = 0;
	mList.mLength = buffSize / kElementSize;
	unpack(mList.mIndexes, buffSize, buff, offset);

	return CHIP_NO_ERROR;
}

template <uint16_t N> inline CHIP_ERROR IndexList<N>::AddIndex(uint16_t index)
{
	/* Do not allow for duplicates. */
	for (uint16_t idx = 0; idx < N; ++idx) {
		if (mList.mIndexes[idx] == index) {
			return CHIP_ERROR_INVALID_ARGUMENT;
		}
	}

	if (mList.mLength >= N) {
		return CHIP_ERROR_BUFFER_TOO_SMALL;
	}
	mList.mIndexes[mList.mLength] = index;
	mList.mLength++;

	return CHIP_NO_ERROR;
}

/* This utility enumeration is implemented based on the CredentialTypeEnum
   (matter/zzz_generated/app-common/app-common/zap-generated/cluster-enums.h).

   It has the following use cases:
   - creating the subsequent CredentialsBits determining which credential type is supported,
   - accessing the genetic array of all supported credentials by index with subscription operator.
*/
enum CredentialTypeIndex : uint8_t {
	/* Programming PIN (CredentialTypeEnum::kProgrammingPIN) is not currently supported. */
	Pin = static_cast<uint8_t>(CredentialTypeEnum::kPin),
	Rfid = static_cast<uint8_t>(CredentialTypeEnum::kRfid),
	Fingerprint = static_cast<uint8_t>(CredentialTypeEnum::kFingerprint),
	FingerVein = static_cast<uint8_t>(CredentialTypeEnum::kFingerVein),
	Face = static_cast<uint8_t>(CredentialTypeEnum::kFace),
	Max = Face
};

using CredentialsBits = uint8_t;

static constexpr CredentialsBits PIN = BIT(Pin);
static constexpr CredentialsBits RFID = BIT(Rfid);
static constexpr CredentialsBits FINGER = BIT(Fingerprint);
static constexpr CredentialsBits VEIN = BIT(FingerVein);
static constexpr CredentialsBits FACE = BIT(Face);
static constexpr CredentialsBits MAX = BIT(Max);

using CredentialArray = std::array<Credential, CONFIG_LOCK_MAX_NUM_CREDENTIALS_PER_TYPE>;

template <CredentialsBits CRED_BIT_MASK> class Credentials {
public:
	using CredentialOperation = bool (*)(DoorLockData::Credential &, uint8_t credentialIndex);

	CredentialArray &GetCredentialsTypes(CredentialTypeEnum type, bool &success)
	{
		static CredentialArray sDummyObj{};
		CredentialTypeIndex credentialTypeIndex = static_cast<CredentialTypeIndex>(type);
		if (CRED_BIT_MASK & BIT(credentialTypeIndex)) {
			success = true;
			/* The CredentialTypeIndex enumeration starts from 1. */
			return mCredentials[credentialTypeIndex - 1];
		}
		success = false;
		return sDummyObj;
	}

	CHIP_ERROR GetCredentials(CredentialTypeEnum type, EmberAfPluginDoorLockCredentialInfo &credential,
				  uint16_t credentialIndex)
	{
		VerifyOrReturnError(credentialIndex > 0, CHIP_ERROR_INVALID_ARGUMENT);
		bool success{ false };
		auto &cred = GetCredentialsTypes(type, success);
		VerifyOrReturnError(success, CHIP_ERROR_INVALID_ARGUMENT);

		/* Indexing starts from 1 in the door lock server. */
		return cred[credentialIndex - 1].ConvertToPlugin(credential);
	}

	void Initialize()
	{
		for (auto &credType : mCredentials) {
			for (auto &cred : credType) {
				cred.mInfo.mFields.mStatus = static_cast<uint8_t>(DlCredentialStatus::kAvailable);
				cred.mInfo.mFields.mCredentialType =
					static_cast<uint8_t>(CredentialTypeEnum::kUnknownEnumValue);
				cred.mSecret.mDataLength = 0;
				memset(cred.mSecret.mData, 0, sizeof(cred.mSecret.mData));
				cred.mInfo.mFields.mCreationSource = static_cast<uint8_t>(DlAssetSource::kUnspecified);
				cred.mInfo.mFields.mCreatedBy = chip::kUndefinedFabricIndex;
				cred.mInfo.mFields.mModificationSource =
					static_cast<uint8_t>(DlAssetSource::kUnspecified);
				cred.mInfo.mFields.mLastModifiedBy = chip::kUndefinedFabricIndex;
			}
		}
	}

	bool ForEach(CredentialOperation operation)
	{
		bool cumulativeError{ true };

		for (auto &credType : mCredentials) {
			/* Array indexes are used as credential indexes (that start from 1 in the door lock
			 * server). */
			uint16_t credIdx{ 0 };
			for (auto &cred : credType) {
				if (operation) {
					bool currOpStatus = operation(cred, credIdx);
					if (!currOpStatus) {
						/* It's user responsibility to handle the operation properly, so
						 * all we can do is to continue */
						continue;
					}
					cumulativeError = cumulativeError && currOpStatus;
				}
				credIdx++;
			}
		}
		return cumulativeError;
	}

private:
	static constexpr uint8_t RequestedNumOfCredTypesSupported() { return POPCOUNT(CRED_BIT_MASK); }

	static_assert(CRED_BIT_MASK <= (PIN | RFID | FINGER | VEIN | FACE), "Unsupported credential type.");
	static_assert(RequestedNumOfCredTypesSupported() <= 5, "Maximum number of credentials exceeded.");

	static constexpr uint8_t sCredentialTypeNumber{ RequestedNumOfCredTypesSupported() };

	std::array<CredentialArray, sCredentialTypeNumber> mCredentials{};
};

} /* namespace DoorLockData */
