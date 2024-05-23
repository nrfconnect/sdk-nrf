/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "diagnostic_logs_end_user.h"

#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>
#include <zephyr/retention/retention.h>

using namespace chip;

namespace
{
const struct device *kEndUserDataMem = DEVICE_DT_GET(DT_NODELABEL(end_user_logs_retention));
} /* namespace */

CHIP_ERROR DiagnosticLogsEndUser::Init()
{
	if (mIsInitialized) {
		return CHIP_NO_ERROR;
	}

	mCapacity = retention_size(kEndUserDataMem);
	VerifyOrReturnError(mCapacity > kHeaderSize, CHIP_ERROR_INTERNAL);

	/* The actual capacity to store data is reduced by "header" tracking the stored size and data beginning. */
	mCapacity = mCapacity - kHeaderSize;

	int ret = retention_is_valid(kEndUserDataMem);

	/* If data is invalid just clean the header data, as we are not sure which part is corrupted. */
	if (ret != 1) {
		ret = retention_write(kEndUserDataMem, 0, reinterpret_cast<uint8_t *>(&mCurrentSize),
				      sizeof(mCurrentSize));
		VerifyOrReturnError(ret == 0, System::MapErrorZephyr(ret));
		ret = retention_write(kEndUserDataMem, sizeof(mCurrentSize), reinterpret_cast<uint8_t *>(&mDataBegin),
				      sizeof(mDataBegin));
		VerifyOrReturnError(ret == 0, System::MapErrorZephyr(ret));
	} else {
		/* Load the header data from retention RAM. */
		ret = retention_read(kEndUserDataMem, 0, reinterpret_cast<uint8_t *>(&mCurrentSize),
				     sizeof(mCurrentSize));
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));

		ret = retention_read(kEndUserDataMem, sizeof(mCurrentSize), reinterpret_cast<uint8_t *>(&mDataBegin),
				     sizeof(mDataBegin));
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
	}

	mDataEnd = mDataBegin + mCurrentSize > mCapacity ? mCurrentSize + mDataBegin - mCapacity :
							   mDataBegin + mCurrentSize;

	mIsInitialized = true;

	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogsEndUser::Clear()
{
	return System::MapErrorZephyr(retention_clear(kEndUserDataMem));
}

CHIP_ERROR DiagnosticLogsEndUser::PushLog(const void *data, size_t size)
{
	VerifyOrReturnError(data, CHIP_ERROR_INVALID_ARGUMENT);
	VerifyOrReturnError(mIsInitialized, CHIP_ERROR_INCORRECT_STATE);
	VerifyOrReturnError(size < mCapacity, CHIP_ERROR_BUFFER_TOO_SMALL);

	int ret = 0;

	const size_t freeSpace = mCapacity - mCurrentSize;
	/* There is no place for the new data. Forget the oldest data by moving the offset forward. */
	if (freeSpace <= size) {
		const size_t requiredShift = size - freeSpace;
		mCurrentSize -= requiredShift;

		/* Detect buffer wrapping around. */
		mDataBegin = mDataBegin + requiredShift > mCapacity ? mDataBegin + requiredShift - mCapacity : mDataBegin + requiredShift;

		/* Update data description header. */
		ret = retention_write(kEndUserDataMem, 0, reinterpret_cast<uint8_t *>(&mCurrentSize),
				      sizeof(mCurrentSize));
		ret = retention_write(kEndUserDataMem, sizeof(mCurrentSize), reinterpret_cast<uint8_t *>(&mDataBegin),
				      sizeof(mDataBegin));
	}

	size_t bytesToWrap = GetBytesToWrapNumber(mDataEnd + size);

	/* Detect buffer wrapping around. */
	if (bytesToWrap) {
		/* Save the part that fits the buffer. */
		ret = retention_write(kEndUserDataMem, GetRetentionAddress(mDataEnd),
				      reinterpret_cast<const uint8_t *>(data), size - bytesToWrap);
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		mDataEnd = 0;

		/* Save the part that wrapped around. */
		ret = retention_write(kEndUserDataMem, GetRetentionAddress(mDataEnd),
				      reinterpret_cast<const uint8_t *>(data) + size - bytesToWrap, bytesToWrap);
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		mDataEnd += bytesToWrap;
	} else {
		ret = retention_write(kEndUserDataMem, GetRetentionAddress(mDataEnd),
				      reinterpret_cast<const uint8_t *>(data), size);
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		mDataEnd += size;
	}

	mCurrentSize += size;

	/* Update data description header. */
	ret = retention_write(kEndUserDataMem, 0, reinterpret_cast<uint8_t *>(&mCurrentSize), sizeof(mCurrentSize));

	return CHIP_NO_ERROR;
}

size_t DiagnosticLogsEndUserReader::GetLogsSize()
{
	return DiagnosticLogsEndUser::Instance().GetLogsSize();
}

CHIP_ERROR DiagnosticLogsEndUserReader::GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog)
{
	const size_t currentSize = DiagnosticLogsEndUser::Instance().GetLogsSize();
	const uint32_t dataBegin = DiagnosticLogsEndUser::Instance().GetLogsBegin();
	int ret = 0;

	if (!mReadInProgress) {
		mReadOffset = dataBegin;
		mReadInProgress = true;
	}

	/* Calculate how many bytes left to be read, considering previous iterations. */
	const size_t readSize =
		mReadOffset >= dataBegin ?
			currentSize - mReadOffset + dataBegin :
			currentSize - (DiagnosticLogsEndUser::Instance().GetCapacity() - dataBegin + mReadOffset);

	/* Check if the whole buffer will fit into single chunk or the fragmentation is required. */
	size_t size = outBuffer.size() > readSize ? readSize : outBuffer.size();

	size_t bytesToWrap = DiagnosticLogsEndUser::Instance().GetBytesToWrapNumber(mReadOffset + size);

	if (bytesToWrap) {
		/* Read the part that fits the buffer. */
		ret = retention_read(kEndUserDataMem,
				     DiagnosticLogsEndUser::Instance().GetRetentionAddress(mReadOffset),
				     reinterpret_cast<uint8_t *>(outBuffer.data()), size - bytesToWrap);
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		mReadOffset = 0;

		/* Read the part that wrapped around. */
		ret = retention_read(kEndUserDataMem,
				     DiagnosticLogsEndUser::Instance().GetRetentionAddress(mReadOffset),
				     reinterpret_cast<uint8_t *>(outBuffer.data()) + size - bytesToWrap, bytesToWrap);
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		mReadOffset += bytesToWrap;
	} else {
		ret = retention_read(kEndUserDataMem,
				     DiagnosticLogsEndUser::Instance().GetRetentionAddress(mReadOffset),
				     reinterpret_cast<uint8_t *>(outBuffer.data()), size);
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		mReadOffset += size;
	}

	outBuffer.reduce_size(size);

	if (DiagnosticLogsEndUser::Instance().IsEnd(mReadOffset)) {
		mReadInProgress = false;
		outIsEndOfLog = true;
	} else {
		outIsEndOfLog = false;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogsEndUserReader::FinishLogs()
{
	/* Do nothing. */
	return CHIP_NO_ERROR;
}
