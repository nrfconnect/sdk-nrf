/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "diagnostic_logs_end_user.h"

using namespace chip;

CHIP_ERROR DiagnosticLogsEndUser::Init()
{
	return mDiagnosticLogsRetention.Init();
}

CHIP_ERROR DiagnosticLogsEndUser::Clear()
{
	return mDiagnosticLogsRetention.Clear();
}

CHIP_ERROR DiagnosticLogsEndUser::PushLog(const void *data, size_t size)
{
	return mDiagnosticLogsRetention.PushLog(data, size);
}

size_t DiagnosticLogsEndUserReader::GetLogsSize()
{
	return DiagnosticLogsEndUser::Instance().mDiagnosticLogsRetention.GetLogsSize();
}

CHIP_ERROR DiagnosticLogsEndUserReader::GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog)
{
	return DiagnosticLogsEndUser::Instance().mDiagnosticLogsRetention.GetLogs(outBuffer, outIsEndOfLog, mReadOffset,
										  mReadInProgress);
}

CHIP_ERROR DiagnosticLogsEndUserReader::FinishLogs()
{
	/* Do nothing. */
	return CHIP_NO_ERROR;
}
