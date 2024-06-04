/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app/util/config.h>

#ifndef MATTER_DM_DIAGNOSTIC_LOGS_CLUSTER_SERVER_ENDPOINT_COUNT
#error "Diagnostics cluster is not enabled in the root endpoint"
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
#include "diagnostic_logs_crash.h"
#endif

#include "diagnostic_logs_provider.h"

#include <app/clusters/diagnostic-logs-server/diagnostic-logs-server.h>

using namespace chip;
using namespace chip::app::Clusters::DiagnosticLogs;
using namespace Nrf;
using namespace Nrf::Matter;

namespace
{

bool IsValidIntent(IntentEnum intent)
{
	return intent < IntentEnum::kUnknownEnumValue;
}

#ifdef CONFIG_LOG
const char *GetIntentStr(IntentEnum intent)
{
	switch (intent) {
	case IntentEnum::kEndUserSupport:
		return "end user";
	case IntentEnum::kNetworkDiag:
		return "network";
	case IntentEnum::kCrashLogs:
		return "crash";
	default:
		break;
	}

	return "unknown";
}
#endif /* CONFIG_LOG */

} /* namespace */

void IntentData::Clear()
{
	if (!mIntentImpl) {
		return;
	}

	switch (mIntent) {
	case chip::app::Clusters::DiagnosticLogs::IntentEnum::kCrashLogs:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
		chip::Platform::Delete(reinterpret_cast<CrashData *>(mIntentImpl));
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
	case chip::app::Clusters::DiagnosticLogs::IntentEnum::kNetworkDiag:
	case chip::app::Clusters::DiagnosticLogs::IntentEnum::kEndUserSupport:
	default:
		break;
	}
}

CHIP_ERROR DiagnosticLogProvider::SetIntentImplementation(IntentEnum intent, IntentData &intentData)
{
	switch (intent) {
	case IntentEnum::kCrashLogs:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
		intentData.mIntentImpl = Platform::New<CrashData>();
		if (!intentData.mIntentImpl) {
			return CHIP_ERROR_NO_MEMORY;
		}
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
	case IntentEnum::kNetworkDiag:
	case IntentEnum::kEndUserSupport:
	default:
		intentData.mIntentImpl = nullptr;
		break;
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogProvider::StartLogCollection(IntentEnum intent, LogSessionHandle &outHandle,
						     Optional<uint64_t> &outTimeStamp,
						     Optional<uint64_t> &outTimeSinceBoot)
{
	VerifyOrReturnError(IsValidIntent(intent), CHIP_ERROR_INVALID_ARGUMENT);

	uint16_t freeSlot = mIntentMap.GetFirstFreeSlot();
	VerifyOrReturnError(freeSlot < kMaxLogSessionHandle, CHIP_ERROR_NO_MEMORY);

	IntentData intentData;
	intentData.mIntent = intent;
	CHIP_ERROR err = SetIntentImplementation(intent, intentData);

	VerifyOrReturnError(err == CHIP_NO_ERROR, err);

	mIntentMap.Insert(freeSlot, std::move(intentData));
	outHandle = freeSlot;

	ChipLogDetail(Zcl, "Starting Log collection for %s with session handle %d", GetIntentStr(intent), outHandle);

	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogProvider::EndLogCollection(LogSessionHandle sessionHandle)
{
	VerifyOrReturnError(sessionHandle < kMaxLogSessionHandle, CHIP_ERROR_NO_MEMORY);
	VerifyOrReturnError(mIntentMap.Contains(sessionHandle), CHIP_ERROR_INTERNAL);

	CHIP_ERROR err = CHIP_NO_ERROR;

	/* Do the specific action for the removing intent */
	switch (mIntentMap[sessionHandle].mIntent) {
	case IntentEnum::kEndUserSupport:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for End User Intent */
		err = CHIP_ERROR_NOT_IMPLEMENTED;
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kNetworkDiag:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for Network Diagnostics */
		err = CHIP_ERROR_NOT_IMPLEMENTED;
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kCrashLogs:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
		if (mIntentMap[sessionHandle].mIntentImpl) {
			err = mIntentMap[sessionHandle].mIntentImpl->FinishLogs();
		}
		break;
#else
		err = CHIP_ERROR_NOT_IMPLEMENTED;
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
	default:
		err = CHIP_ERROR_INVALID_ARGUMENT;
		break;
	}

	ChipLogDetail(Zcl, "Ending Log collection for %s with session handle %d",
		      GetIntentStr(mIntentMap[sessionHandle].mIntent), sessionHandle);

	/* Old intent is not needed anymore*/
	mIntentMap.Erase(sessionHandle);

	return err;
}

CHIP_ERROR DiagnosticLogProvider::CollectLog(LogSessionHandle sessionHandle, MutableByteSpan &outBuffer,
					     bool &outIsEndOfLog)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

	VerifyOrReturnError(sessionHandle < kMaxLogSessionHandle, CHIP_ERROR_NO_MEMORY);
	VerifyOrReturnError(mIntentMap.Contains(sessionHandle), CHIP_ERROR_INTERNAL);

	switch (mIntentMap[sessionHandle].mIntent) {
	case IntentEnum::kEndUserSupport:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for End User Intent */
		return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kNetworkDiag:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		err = GetTestingLogs(mIntentMap[sessionHandle].mIntent, outBuffer, outIsEndOfLog);
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
		/* TODO: add support for Network Diag Intent */
		return CHIP_ERROR_NOT_IMPLEMENTED;
	case IntentEnum::kCrashLogs:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
		err = mIntentMap[sessionHandle].mIntentImpl->GetLogs(outBuffer, outIsEndOfLog);
		break;
#else
		return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return err;
}

size_t DiagnosticLogProvider::GetSizeForIntent(IntentEnum intent)
{
	size_t logSize = 0;

	switch (intent) {
	case IntentEnum::kEndUserSupport:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for End User Intent */
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kNetworkDiag:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		logSize = GetTestingLogsSize(intent);
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
		/* TODO: add support for Network Diag Intent */
		break;
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
	case IntentEnum::kCrashLogs: {
		Platform::UniquePtr<CrashData> crashData(Platform::New<CrashData>());
		logSize = crashData->GetLogsSize();
		break;
	}
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
	default:
		break;
	}

	ChipLogDetail(Zcl, "Current Log size for Intent: %s is %zu", GetIntentStr(intent), logSize);

	return logSize;
}

CHIP_ERROR DiagnosticLogProvider::GetLogForIntent(IntentEnum intent, MutableByteSpan &outBuffer,
						  Optional<uint64_t> &outTimeStamp,
						  Optional<uint64_t> &outTimeSinceBoot)
{
	LogSessionHandle sessionHandle = kInvalidLogSessionHandle;

	CHIP_ERROR err = StartLogCollection(intent, sessionHandle, outTimeStamp, outTimeSinceBoot);
	VerifyOrExit(CHIP_NO_ERROR == err, );

	bool unusedOutIsEndOfLog;
	err = CollectLog(sessionHandle, outBuffer, unusedOutIsEndOfLog);
	VerifyOrExit(CHIP_NO_ERROR == err, );

	err = EndLogCollection(sessionHandle);

exit:
	if (CHIP_NO_ERROR != err) {
		outBuffer.reduce_size(0);
	}

	return err;
}

CHIP_ERROR DiagnosticLogProvider::Init()
{
	return CHIP_NO_ERROR;
}

void emberAfDiagnosticLogsClusterInitCallback(chip::EndpointId endpoint)
{
	auto &logProvider = DiagnosticLogProvider::GetInstance();

	DiagnosticLogsServer::Instance().SetDiagnosticLogsProviderDelegate(endpoint, &logProvider);
}

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST

bool DiagnosticLogProvider::StoreTestingLog(IntentEnum intent, char *text, size_t textSize)
{
	if (!text) {
		return false;
	}

	if (intent != IntentEnum::kEndUserSupport && intent != IntentEnum::kNetworkDiag) {
		return false;
	}

	uint8_t *bufferToWrite =
		(intent == IntentEnum::kEndUserSupport) ? mTestingUserLogsBuffer : mTestingNetworkLogsBuffer;
	size_t *bufferSize = (intent == IntentEnum::kEndUserSupport) ? &mCurrentUserLogsSize : &mCurrentNetworkLogsSize;

	if (textSize + *bufferSize > kTestingBufferLen) {
		return false;
	}

	memcpy(bufferToWrite + *bufferSize, text, textSize);
	*bufferSize += textSize;

	return true;
}

void DiagnosticLogProvider::ClearTestingBuffer(IntentEnum intent)
{
	if (intent != IntentEnum::kEndUserSupport && intent != IntentEnum::kNetworkDiag) {
		return;
	}

	uint8_t *bufferToWrite =
		(intent == IntentEnum::kEndUserSupport) ? mTestingUserLogsBuffer : mTestingNetworkLogsBuffer;
	size_t *bufferSize = (intent == IntentEnum::kEndUserSupport) ? &mCurrentUserLogsSize : &mCurrentNetworkLogsSize;
	size_t *readOffset = (intent == IntentEnum::kEndUserSupport) ? &mReadUserLogsOffset : &mReadNetworkLogsOffset;

	memset(bufferToWrite, 0, kTestingBufferLen);
	*bufferSize = 0;
	*readOffset = 0;
}

CHIP_ERROR DiagnosticLogProvider::GetTestingLogs(IntentEnum intent, chip::MutableByteSpan &outBuffer,
						 bool &outIsEndOfLog)
{
	if (intent != IntentEnum::kEndUserSupport && intent != IntentEnum::kNetworkDiag) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	uint8_t *bufferToWrite =
		(intent == IntentEnum::kEndUserSupport) ? mTestingUserLogsBuffer : mTestingNetworkLogsBuffer;
	size_t *currentBufferSize =
		(intent == IntentEnum::kEndUserSupport) ? &mCurrentUserLogsSize : &mCurrentNetworkLogsSize;
	size_t *readOffset = (intent == IntentEnum::kEndUserSupport) ? &mReadUserLogsOffset : &mReadNetworkLogsOffset;

	size_t sizeToRead = *currentBufferSize > outBuffer.size() ? outBuffer.size() : *currentBufferSize;

	memcpy(outBuffer.data(), bufferToWrite + *readOffset, sizeToRead);
	*currentBufferSize -= sizeToRead;
	*readOffset += sizeToRead;

	if (*currentBufferSize == 0) {
		outIsEndOfLog = true;
		ClearTestingBuffer(intent);
	} else {
		outIsEndOfLog = false;
	}

	ChipLogDetail(Zcl, "Sending %zu B of logs, left bytes: %zu, is the end of logs: %d", sizeToRead,
		      *currentBufferSize, outIsEndOfLog);

	outBuffer.reduce_size(sizeToRead);

	return CHIP_NO_ERROR;
}

size_t DiagnosticLogProvider::GetTestingLogsSize(IntentEnum intent)
{
	return (intent == IntentEnum::kEndUserSupport) ? mCurrentUserLogsSize : mCurrentNetworkLogsSize;
}

#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
