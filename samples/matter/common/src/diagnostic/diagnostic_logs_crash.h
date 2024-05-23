/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

#include "diagnostic_logs_provider.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>

using namespace chip;

namespace Nrf
{
struct CrashDescription {
	z_arch_esf_t Esf = {};
	unsigned int Reason = 0;
	const char *ThreadName = nullptr;
	uint32_t *ThreadInt = 0;
	unsigned int Source = 0;
};

#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS
/* Assuming 2 Bytes for prefix and 2 bytes for CRC */
static_assert(DT_REG_SIZE(DT_NODELABEL(crash_retention)) - 4 > sizeof(CrashDescription));
#endif

/**
 * @brief Class for creating log from the DiagnosticLogsCrash data.
 *
 * DiagnosticLogsCrash data can be obtained from the fatal error ISR and converted to human-readable
   format.
 *
 * This class takes care on the conversion from ESF data to Log and
 *
 * There is several steps of converting the esf data, and some of them can be disabled using kconfig
   to reduce the final log size:

	* BasicDump converts the basic registers of the device.
	  Cannot be disabled.
	* FpuDump converts the floating point unit registers of the device.
	  Can be disabled using CONFIG_FPU and CONFIG_FPU_SHARING
	* ExtraDump converts an additional extra exception info registers of the device.
	  Can be disabled using CONFIG_EXTRA_EXCEPTION_INFO.
	* ReasonDump converts the reason of the fault.
	  Cannot be disabled.
	* MultithreadDump converts the information about the the thread from which the error occurred.
	  Can be disabled using CONFIG_MULTITHREADING.
 */
class DiagnosticLogsCrash : public Nrf::Matter::DiagnosticLogsIntentIface {
public:
	constexpr static size_t MaxSingleDumpSize = 100;

	CHIP_ERROR GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog) override;
	CHIP_ERROR FinishLogs() override;
	size_t GetLogsSize() override;
	static CHIP_ERROR Clear();

private:
	/* The source is obtained based on values from ARM Cortex vector table set in the ICSR register. */
	enum FaultSource {
		HardFault = 3,
		MemManageFault = 4,
		BusFault = 5,
		UsageFault = 6,
		SecureFault = 7,
		DebugMonitor = 12
	};

	/**
	 * @brief Convert CrashDat to output string buffer that will contain a crash message.
	 *
	 * This method converts the crash data received from the ISR to human-readable
	   format that can be displayed as log.
	 *
	 * @param outBuffer output string buffer
	 * @param bufferSize size of the output string buffer.
	 * @param end Assign that this is end of the whole DiagnosticLogsCrash stored.
	 * @return size_t as the actual length of the converted data.
	 */
	size_t ProcessConversionToLog(char *outBuffer, size_t bufferSize, bool &end);

	/**
	 * @brief Get a pointer to the crash description structure.
	 *
	 * This method can be used to fill CrashDescription data.
	 *
	 * @return CrashDescription* a pointer to crash description object that contains ESF and reason data.
	 */
	CrashDescription *GetDescription() { return &mDescription; }

	/**
	 * @brief Calculate a size of the actual crash logs.
	 *
	 * The actual size depends on the crash type and collected crash data.
	 *
	 * @return size_t size of the actual crash logs.
	 */
	size_t CalculateSize();

	const char *FaultSourceToStr(uint32_t source);

	bool Collect(const char *format, ...);

	bool BasicDump();
	bool ReasonDump();

	CHIP_ERROR LoadCrashData();
	void ClearState();

	/* Actual crash data to be filled */
	CrashDescription mDescription = {};

	/* Output buffer descriptions */
	char *mOutBuffer = nullptr;
	size_t mOutBufferSize = 0;

	/* Local buffer to store single log line */
	char mBuffer[MaxSingleDumpSize] = {};
	size_t mOffset = 0;

	/* Current progress descriptions */
	uint16_t mLastLine = 0;
	uint16_t mLine = 0;
};

} /* namespace Nrf */
