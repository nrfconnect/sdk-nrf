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
#include <zephyr/retention/retention.h>
#endif

#include "diagnostic_logs_provider.h"

#include <app/clusters/diagnostic-logs-server/diagnostic-logs-server.h>

using namespace chip;
using namespace chip::app::Clusters::DiagnosticLogs;
using namespace Nrf;
using namespace Nrf::Matter;

namespace
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
const struct device *kCrashDataMem = DEVICE_DT_GET(DT_NODELABEL(crash_retention));
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */

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

CHIP_ERROR DiagnosticLogProvider::StartLogCollection(IntentEnum intent, LogSessionHandle &outHandle,
						     Optional<uint64_t> &outTimeStamp,
						     Optional<uint64_t> &outTimeSinceBoot)
{
	VerifyOrReturnError(IsValidIntent(intent), CHIP_ERROR_INVALID_ARGUMENT);

	uint16_t freeSlot = mIntentMap.GetFirstFreeSlot();
	VerifyOrReturnError(freeSlot < kMaxLogSessionHandle, CHIP_ERROR_NO_MEMORY);

	mIntentMap.Insert(freeSlot, std::move(intent));
	outHandle = freeSlot;

	ChipLogDetail(Zcl, "Starting Log collection for %s with session handle %d", GetIntentStr(intent), outHandle);

	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogProvider::EndLogCollection(LogSessionHandle sessionHandle)
{
	VerifyOrReturnError(sessionHandle < kMaxLogSessionHandle, CHIP_ERROR_NO_MEMORY);
	VerifyOrReturnError(mIntentMap.Contains(sessionHandle), CHIP_ERROR_INTERNAL);

	CHIP_ERROR err = CHIP_NO_ERROR;

	IntentEnum intent = mIntentMap[sessionHandle];

	/* Old intent is no needed anymore*/
	mIntentMap.Erase(sessionHandle);

	/* Do the specific action for the removing intent */
	switch (intent) {
	case IntentEnum::kEndUserSupport:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for End User Intent */
		return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kNetworkDiag:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for Network Diagnostics */
		return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kCrashLogs:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
		err = FinishCrashLogs();
		break;
#else
		return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	ChipLogDetail(Zcl, "Ending Log collection for %s with session handle %d",
		      GetIntentStr(mIntentMap[sessionHandle]), sessionHandle);

	return err;
}

CHIP_ERROR DiagnosticLogProvider::CollectLog(LogSessionHandle sessionHandle, MutableByteSpan &outBuffer,
					     bool &outIsEndOfLog)
{
	CHIP_ERROR err = CHIP_NO_ERROR;

	VerifyOrReturnError(sessionHandle < kMaxLogSessionHandle, CHIP_ERROR_NO_MEMORY);
	VerifyOrReturnError(mIntentMap.Contains(sessionHandle), CHIP_ERROR_INTERNAL);

	switch (mIntentMap[sessionHandle]) {
	case IntentEnum::kEndUserSupport:
#ifndef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		/* TODO: add support for End User Intent */
		return CHIP_ERROR_NOT_IMPLEMENTED;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
	case IntentEnum::kNetworkDiag:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
		err = GetTestingLogs(mIntentMap[sessionHandle], outBuffer, outIsEndOfLog);
		break;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST */
		/* TODO: add support for Network Diag Intent */
		return CHIP_ERROR_NOT_IMPLEMENTED;
	case IntentEnum::kCrashLogs:
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
		err = GetCrashLogs(outBuffer, outIsEndOfLog);
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
	case IntentEnum::kCrashLogs:
		logSize = GetCrashLogsSize();
		break;
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
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS
	ReturnErrorOnFailure(MoveCrashLogsToNVS());
#endif

	return CHIP_NO_ERROR;
}

void emberAfDiagnosticLogsClusterInitCallback(chip::EndpointId endpoint)
{
	auto &logProvider = DiagnosticLogProvider::GetInstance();

	DiagnosticLogsServer::Instance().SetDiagnosticLogsProviderDelegate(endpoint, &logProvider);
}

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS

CHIP_ERROR DiagnosticLogProvider::LoadCrashData(CrashData *crashData)
{
	VerifyOrReturnError(crashData, CHIP_ERROR_INVALID_ARGUMENT);

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS
	VerifyOrReturnError(Nrf::GetPersistentStorage().NonSecureHasEntry(&mCrashLogsStorageNode),
			    CHIP_ERROR_NOT_FOUND);
	size_t outSize = 0;
	VerifyOrReturnError(Nrf::GetPersistentStorage().NonSecureLoad(
				    &mCrashLogsStorageNode, reinterpret_cast<void *>(crashData->GetDescription()),
				    sizeof(*crashData->GetDescription()), outSize),
			    CHIP_ERROR_INTERNAL);
	VerifyOrReturnError(outSize == sizeof(*crashData->GetDescription()), CHIP_ERROR_INTERNAL);
#else
	int ret = retention_is_valid(kCrashDataMem);
	VerifyOrReturnError(ret == 1, CHIP_ERROR_NOT_FOUND);
	ret = retention_read(kCrashDataMem, 0, reinterpret_cast<uint8_t *>(crashData->GetDescription()),
			     sizeof(*crashData->GetDescription()));
	VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS */

	return CHIP_NO_ERROR;
}

size_t DiagnosticLogProvider::GetCrashLogsSize()
{
	CrashData *crashData = Platform::New<CrashData>();
	if (!crashData) {
		return 0;
	}

	size_t outSize = 0;

	VerifyOrExit(CHIP_NO_ERROR == LoadCrashData(crashData), );

	outSize = crashData->CalculateSize();

exit:
	Platform::Delete(crashData);

	return outSize;
}

CHIP_ERROR DiagnosticLogProvider::GetCrashLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog)
{
	size_t convertedSize = 0;
	CHIP_ERROR err = CHIP_NO_ERROR;

	/* if mCrashData is not Null then it means that it is the following data bunch */
	if (!mCrashData) {
		/* mCrashData is not initialized, so this is the first bunch - allocate the memory */
		mCrashData = Platform::New<CrashData>();
		if (!mCrashData) {
			return CHIP_ERROR_NO_MEMORY;
		}
		err = LoadCrashData(mCrashData);
		VerifyOrExit(CHIP_NO_ERROR == err, );
	}

	convertedSize = mCrashData->ProcessConversionToLog(reinterpret_cast<char *>(outBuffer.data()), outBuffer.size(),
							   outIsEndOfLog);
	outBuffer.reduce_size(convertedSize);

	ChipLogDetail(Zcl, "Getting log with size %zu. Is end of log: %s", convertedSize,
		      outIsEndOfLog ? "true" : "false");

exit:
	/* If it is the end of the log message we can release allocated memory, otherwise, we need to keep Esf data for
	 * the following bunch */
	if (outIsEndOfLog || convertedSize == 0 || err != CHIP_NO_ERROR) {
		Platform::Delete(mCrashData);
		mCrashData = nullptr;
	}

	return err;
}

CHIP_ERROR DiagnosticLogProvider::FinishCrashLogs()
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS
	VerifyOrReturnError(Nrf::GetPersistentStorage().NonSecureRemove(&mCrashLogsStorageNode), CHIP_ERROR_INTERNAL);
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS */
	int ret = retention_clear(kCrashDataMem);
	VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ */

	return CHIP_NO_ERROR;
}

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS
CHIP_ERROR DiagnosticLogProvider::MoveCrashLogsToNVS()
{
	/* Check if something is in the retained memory */
	CrashDescription description;
	if (1 == retention_is_valid(kCrashDataMem)) {
		int ret = retention_read(kCrashDataMem, 0, reinterpret_cast<uint8_t *>(&description),
					 sizeof(description));
		VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
		/* Check whether the entry already exists */
		CrashDescription descriptionToCompare;
		size_t outSize = 0;
		if (Nrf::GetPersistentStorage().NonSecureHasEntry(&mCrashLogsStorageNode)) {
			VerifyOrReturnError(Nrf::GetPersistentStorage().NonSecureLoad(
						    &mCrashLogsStorageNode,
						    reinterpret_cast<void *>(&descriptionToCompare),
						    sizeof(descriptionToCompare), outSize),
					    CHIP_ERROR_READ_FAILED);
			if (0 == memcmp(&descriptionToCompare, &description, sizeof(description))) {
				ret = retention_clear(kCrashDataMem);
				VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));
				return CHIP_NO_ERROR;
			}
		}

		VerifyOrReturnError(Nrf::GetPersistentStorage().NonSecureStore(&mCrashLogsStorageNode, &description,
									       sizeof(description)),
				    CHIP_ERROR_WRITE_FAILED);
	}

	return CHIP_NO_ERROR;
}
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_SAVE_CRASH_TO_SETTINGS */
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS */

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
