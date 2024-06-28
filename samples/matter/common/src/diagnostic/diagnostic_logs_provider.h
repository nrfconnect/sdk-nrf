/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "diagnostic_logs_intent_iface.h"

#if defined(CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS) ||                                                 \
	defined(CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS)
#include "diagnostic_logs_retention.h"
#endif

#include "util/finite_map.h"

#include <app/clusters/diagnostic-logs-server/DiagnosticLogsProviderDelegate.h>
#include <lib/core/CHIPError.h>

namespace Nrf::Matter
{

struct IntentData {
	chip::app::Clusters::DiagnosticLogs::IntentEnum mIntent =
		chip::app::Clusters::DiagnosticLogs::IntentEnum::kUnknownEnumValue;
	DiagnosticLogsIntentIface *mIntentImpl = nullptr;

	IntentData() {}

	/* Disable copy semantics and implement move, bool and == semantics according to the FiniteMap
	 * requirements. */
	IntentData(const IntentData &other) = delete;
	IntentData &operator=(const IntentData &other) = delete;

	IntentData(IntentData &&other)
	{
		mIntent = other.mIntent;
		mIntentImpl = other.mIntentImpl;
		/* Nullptr the copied data to prevent from deleting it by the source. */
		other.mIntentImpl = nullptr;
	}

	IntentData &operator=(IntentData &&other)
	{
		if (this != &other) {
			Clear();
			mIntent = other.mIntent;
			mIntentImpl = other.mIntentImpl;
			/* Nullptr the copied data to prevent from deleting it by the source. */
			other.mIntentImpl = nullptr;
		}
		return *this;
	}

	operator bool() const { return true; }
	bool operator==(const IntentData &other)
	{
		return mIntent == other.mIntent && mIntentImpl == other.mIntentImpl;
	}

	void Clear();

	~IntentData() { Clear(); }
};

class DiagnosticLogProvider : public chip::app::Clusters::DiagnosticLogs::DiagnosticLogsProviderDelegate {
public:
	/**
	 * @brief Get the Diagnostic Log Provider instance
	 *
	 * @return DiagnosticLogProvider object
	 */
	static inline DiagnosticLogProvider &GetInstance()
	{
		static DiagnosticLogProvider sInstance;
		return sInstance;
	}

	/**
	 * @brief Initialize the diagnostic log provider module
	 *
	 * @retval CHIP_ERROR_READ_FAILED if the NVS storage contains a crash log entry, but it cannot be loaded.
	 * @retval CHIP_ERROR_WRITE_FAILED if a new crash data could not be stored in the NVS storage.
	 * @retval other CHIP_ERROR values that come from internal methods.
	 */
	CHIP_ERROR Init();

	/* DiagnosticsLogsProviderDelegate Interface */
	CHIP_ERROR StartLogCollection(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
				      chip::app::Clusters::DiagnosticLogs::LogSessionHandle &outHandle,
				      chip::Optional<uint64_t> &outTimeStamp,
				      chip::Optional<uint64_t> &outTimeSinceBoot) override;
	CHIP_ERROR EndLogCollection(chip::app::Clusters::DiagnosticLogs::LogSessionHandle sessionHandle) override;
	CHIP_ERROR CollectLog(chip::app::Clusters::DiagnosticLogs::LogSessionHandle sessionHandle,
			      chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) override;
	size_t GetSizeForIntent(chip::app::Clusters::DiagnosticLogs::IntentEnum intent) override;
	CHIP_ERROR GetLogForIntent(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
				   chip::MutableByteSpan &outBuffer, chip::Optional<uint64_t> &outTimeStamp,
				   chip::Optional<uint64_t> &outTimeSinceBoot) override;

	void ClearLogs();

	/**
	 * @brief Push the new log to the logs ring buffer.
	 *
	 * If the buffer is full, the new data overrides the oldest one.
	 * It supports only end user and network intents.
	 * The crash intent is not supported, as crash logs are added asynchronously if failure occurs and cannot be
	 * logged using public API.
	 *
	 * @param intent the type of pushed log
	 * @param data address of logs data to be stored in the buffer
	 * @param size size of data to be stored in the buffer
	 *
	 * @return CHIP_NO_ERROR on success, the other error code on failure.
	 */
	CHIP_ERROR PushLog(chip::app::Clusters::DiagnosticLogs::IntentEnum intent, const void *data, size_t size);

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
	bool StoreTestingLog(chip::app::Clusters::DiagnosticLogs::IntentEnum intent, char *text, size_t textSize);
	void ClearTestingBuffer(chip::app::Clusters::DiagnosticLogs::IntentEnum intent);
#endif

private:
	/* The maximum number of the simultaneous sessions */
	constexpr static uint16_t kMaxLogSessionHandle =
		CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_MAX_SIMULTANEOUS_SESSIONS;

	static_assert(kMaxLogSessionHandle < chip::app::Clusters::DiagnosticLogs::kInvalidLogSessionHandle);

	/* Not copyable */
	DiagnosticLogProvider(const DiagnosticLogProvider &) = delete;
	DiagnosticLogProvider &operator=(const DiagnosticLogProvider &) = delete;
	/* Not movable */
	DiagnosticLogProvider(DiagnosticLogProvider &&) = delete;
	DiagnosticLogProvider &operator=(DiagnosticLogProvider &&) = delete;

#if defined(CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS) &&                                                  \
	defined(CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS)
	DiagnosticLogProvider()
		: mNetworkLogs(DEVICE_DT_GET(DT_NODELABEL(network_logs_retention))),
		  mEndUserLogs(DEVICE_DT_GET(DT_NODELABEL(end_user_logs_retention)))
	{
	}
#elif defined(CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS)
	DiagnosticLogProvider() : mNetworkLogs(DEVICE_DT_GET(DT_NODELABEL(network_logs_retention))) {}
#elif defined(CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS)
	DiagnosticLogProvider() : mEndUserLogs(DEVICE_DT_GET(DT_NODELABEL(end_user_logs_retention))) {}
#else
	DiagnosticLogProvider() = default;
#endif

	CHIP_ERROR SetIntentImplementation(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
					   IntentData &intentData);

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST
	size_t GetTestingLogsSize(chip::app::Clusters::DiagnosticLogs::IntentEnum intent);
	CHIP_ERROR GetTestingLogs(chip::app::Clusters::DiagnosticLogs::IntentEnum intent,
				  chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog);

	constexpr static size_t kTestingBufferLen = 4096;
	uint8_t mTestingUserLogsBuffer[kTestingBufferLen];
	uint8_t mTestingNetworkLogsBuffer[kTestingBufferLen];
	size_t mCurrentUserLogsSize = 0;
	size_t mCurrentNetworkLogsSize = 0;
	size_t mReadUserLogsOffset = 0;
	size_t mReadNetworkLogsOffset = 0;
#endif

	Nrf::FiniteMap<chip::app::Clusters::DiagnosticLogs::LogSessionHandle, IntentData, kMaxLogSessionHandle>
		mIntentMap;

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS
	DiagnosticLogsRetention mNetworkLogs;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS */

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS
	DiagnosticLogsRetention mEndUserLogs;
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS */
};

} /* namespace Nrf::Matter */
