/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "diagnostic_logs_provider.h"
#include <lib/core/CHIPError.h>

class DiagnosticLogsEndUserReader : public Nrf::Matter::DiagnosticLogsIntentIface {
public:
	CHIP_ERROR GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) override;
	CHIP_ERROR FinishLogs() override;
	size_t GetLogsSize() override;

private:
	uint32_t mReadOffset = 0;
	bool mReadInProgress = false;
};

class DiagnosticLogsEndUser {
public:
	/**
	 * @brief Validates the retention RAM content and initializes the module.
	 *
	 * @return CHIP_NO_ERROR on success, the other error code on failure.
	 */
	CHIP_ERROR Init();

	/**
	 * @brief Stores given logs in the retention RAM.
	 *
	 * If the buffer is full, the new data overrides the oldest one.
	 *
	 * @param data address of logs data to be stored in the buffer
	 * @param size size of data to be stored in the buffer
	 *
	 * @return CHIP_NO_ERROR on success, the other error code on failure.
	 */
	CHIP_ERROR PushLog(const void *data, size_t size);

	/**
	 * @brief Clear the logs stored in the retention memory.
	 *
	 * @return CHIP_NO_ERROR on success, the other error code on failure.
	 */
	CHIP_ERROR Clear();

	/**
	 * @brief Get the stored logs size.
	 *
	 * @return size of stored logs in bytes.
	 */
	size_t GetLogsSize() { return mCurrentSize; }

	/**
	 * @brief Get an offset of the logs beginning in the buffer.
	 */
	uint32_t GetLogsBegin() { return mDataBegin; }

	/**
	 * @brief Get number of bytes that will wrap around the buffer after saving the given size of data.
	 *
	 * @param inputSize size of data to be stored in the buffer
	 *
	 * @return number of bytes that will be wrapped around
	 */
	size_t GetBytesToWrapNumber(size_t inputSize) { return inputSize > mCapacity ? inputSize - mCapacity : 0; }

	/**
	 * @brief Get the logs buffer capacity.
	 *
	 * @return effective size of buffer to store logs (reduced by the data header size)
	 */
	size_t GetCapacity() { return mCapacity; }

	/**
	 * @brief Get the address of retention memory to write or read data.
	 *
	 * @param logsOffset the desired logs offset in the buffer.
	 *
	 * @return the retention memory offset, which is logs offset shifted by the data header size
	 */
	uint32_t GetRetentionAddress(uint32_t logsOffset) { return logsOffset + kHeaderSize; }

	/**
	 * @brief Checks if the given offset matches the logs data end.
	 *
	 * @param offset the desired logs offset in the buffer.
	 *
	 * @return true if the offset matches the data end, false otherwise.
	 */
	bool IsEnd(uint32_t offset) { return offset == mDataEnd; }

	static DiagnosticLogsEndUser &Instance()
	{
		static DiagnosticLogsEndUser sInstance;
		return sInstance;
	}

private:
	const size_t kHeaderSize = sizeof(mCurrentSize) + sizeof(mDataBegin);

	bool mIsInitialized = false;
	size_t mCurrentSize = 0;
	uint32_t mDataBegin = 0;
	uint32_t mDataEnd = 0;
	size_t mCapacity = 0;
};
