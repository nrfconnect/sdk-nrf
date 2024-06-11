/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

namespace Nrf::Matter
{

class DiagnosticLogsIntentIface {
public:
	/**
	 * @brief Get the captured logs size.
	 *
	 * @return size of captured logs in bytes
	 */
	virtual size_t GetLogsSize() = 0;

	/**
	 * @brief Get the captured logs.
	 *
	 * @param outBuffer output buffer to store the logs
	 * @param outIsEndOfLog flag informing whether the stored data is complete log or one of the few chunks
	 *
	 * @return size of captured logs in bytes
	 */
	virtual CHIP_ERROR GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) = 0;

	/**
	 * @brief Finish the log capturing. This method is used to perform potential clean up after session.
	 */
	virtual CHIP_ERROR FinishLogs() = 0;
};

} /* namespace Nrf::Matter */
