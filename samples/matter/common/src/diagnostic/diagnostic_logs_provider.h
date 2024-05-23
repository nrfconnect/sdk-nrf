/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "util/finite_map.h"

#include <app/clusters/diagnostic-logs-server/DiagnosticLogsProviderDelegate.h>
#include <lib/core/CHIPError.h>

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

	DiagnosticLogProvider() = default;

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
};

} /* namespace Nrf::Matter */
