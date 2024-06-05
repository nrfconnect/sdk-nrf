/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "diagnostic_logs_provider.h"
#include "diagnostic_logs_retention.h"
#include <lib/core/CHIPError.h>

class DiagnosticLogsNetworkReader : public Nrf::Matter::DiagnosticLogsIntentIface {
public:
	CHIP_ERROR GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) override;
	CHIP_ERROR FinishLogs() override;
	size_t GetLogsSize() override;

private:
	uint32_t mReadOffset = 0;
	bool mReadInProgress = false;
};

class DiagnosticLogsNetwork {
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

	static DiagnosticLogsNetwork &Instance()
	{
		static DiagnosticLogsNetwork sInstance;
		return sInstance;
	}

private:
	friend class DiagnosticLogsNetworkReader;

	DiagnosticLogsNetwork() : mDiagnosticLogsRetention(DEVICE_DT_GET(DT_NODELABEL(network_logs_retention))) {}

	DiagnosticLogsRetention mDiagnosticLogsRetention;
};
