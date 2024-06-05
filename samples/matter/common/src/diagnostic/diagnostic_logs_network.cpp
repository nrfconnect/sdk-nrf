/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "diagnostic_logs_network.h"

using namespace chip;

CHIP_ERROR DiagnosticLogsNetwork::Init()
{
	return mDiagnosticLogsRetention.Init();
}

CHIP_ERROR DiagnosticLogsNetwork::Clear()
{
	return mDiagnosticLogsRetention.Clear();
}

CHIP_ERROR DiagnosticLogsNetwork::PushLog(const void *data, size_t size)
{
	return mDiagnosticLogsRetention.PushLog(data, size);
}

size_t DiagnosticLogsNetworkReader::GetLogsSize()
{
	return DiagnosticLogsNetwork::Instance().mDiagnosticLogsRetention.GetLogsSize();
}

CHIP_ERROR DiagnosticLogsNetworkReader::GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog)
{
	return DiagnosticLogsNetwork::Instance().mDiagnosticLogsRetention.GetLogs(outBuffer, outIsEndOfLog, mReadOffset,
										  mReadInProgress);
}

CHIP_ERROR DiagnosticLogsNetworkReader::FinishLogs()
{
	/* Do nothing. */
	return CHIP_NO_ERROR;
}
