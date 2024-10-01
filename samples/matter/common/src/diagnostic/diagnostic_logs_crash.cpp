/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "diagnostic_logs_crash.h"

#include <cstdlib>
#include <zephyr/kernel.h>
#include <zephyr/retention/retention.h>

#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

using namespace Nrf;
using namespace chip;

namespace
{
const struct device *kCrashDataMem = DEVICE_DT_GET(DT_NODELABEL(crash_retention));
} // namespace

#define Verify(expr, value)                                                                                            \
	do {                                                                                                           \
		if (!(expr)) {                                                                                         \
			return (value);                                                                                \
		}                                                                                                      \
	} while (false)

size_t DiagnosticLogsCrash::CalculateSize()
{
	bool unusedOut;
	return ProcessConversionToLog(nullptr, 0, unusedOut);
}

size_t DiagnosticLogsCrash::ProcessConversionToLog(char *outBuffer, size_t bufferSize, bool &end)
{
	/* Allow to enter outBuffer as nullptr to calculate actual size.
	if buffer is available, its size cannot be 0 */
	if (outBuffer && bufferSize == 0) {
		return 0;
	}
	mOutBufferSize = bufferSize;
	mOutBuffer = outBuffer;
	mOffset = 0;
	mLine = 0;
	end = false;

	/* Process all stages one by one until reaching the buffer full or the end of the logs. */
	Verify(BasicDump(), mOffset);
	Verify(ReasonDump(), mOffset);
	end = true;

	return mOffset;
}

bool DiagnosticLogsCrash::BasicDump()
{
	Verify(Collect("Faulting instruction address (r15/pc): 0x%08x\n", mDescription.Esf.basic.pc), false);
	Verify(Collect("r0/a1:  0x%08x  r1/a2:  0x%08x  r2/a3:  0x%08x\n", mDescription.Esf.basic.a1,
		       mDescription.Esf.basic.a2, mDescription.Esf.basic.a3),
	       false);
	Verify(Collect("r3/a4:  0x%08x r12/ip:  0x%08x r14/lr:  0x%08x\n", mDescription.Esf.basic.a4,
		       mDescription.Esf.basic.ip, mDescription.Esf.basic.lr),
	       false);
	Verify(Collect(" xpsr:  0x%08x\n", mDescription.Esf.basic.xpsr), false);

	return true;
}

bool DiagnosticLogsCrash::ReasonDump()
{
	Verify(Collect("***** %s *****\n", FaultSourceToStr(mDescription.Source)), false);
	Verify(Collect("ZEPHYR FATAL ERROR %d on CPU %d\n", mDescription.Reason, _current_cpu->id), false);

	return true;
}

const char *DiagnosticLogsCrash::FaultSourceToStr(uint32_t source)
{
	switch (source) {
	case FaultSource::HardFault:
		return "HARD FAULT";
	case FaultSource::MemManageFault:
		return "MEMORY MANAGE FAULT";
	case FaultSource::BusFault:
		return "BUS FAULT";
	case FaultSource::UsageFault:
		return "USAGE FAULT";
	case FaultSource::SecureFault:
		return "SECURE FAULT";
	case FaultSource::DebugMonitor:
		return "DEBUG MONITOR";
	default:
		return "UNKNOWN";
	}
}

bool DiagnosticLogsCrash::Collect(const char *format, ...)
{
	/* Skip the line until reaching the last known one */
	if (mLastLine >= mLine && mLastLine != 0) {
		mLine++;
		return true;
	}

	memset(mBuffer, 0, sizeof(mBuffer));

	/* Collect new data */
	va_list args;
	va_start(args, format);
	int size = vsnprintk(mBuffer, sizeof(mBuffer), format, args);
	va_end(args);

	/* Skip the log line that does not fit the buffer size */
	if (size < 0) {
		return true;
	}

	/* The data can be written if the buffer is available,
	otherwise only mOffset will be used to calculate the actual size */
	if (mOutBuffer) {
		if (mOffset + size > mOutBufferSize) {
			/* The new data cannot be stored in the out buffer because there is no free space */
			return false;
		}
		memcpy(mOutBuffer + mOffset, mBuffer, size);
	}

	/* Save the processed line and seek to the next one */
	mLastLine = mLine++;
	/* Increase the offset */
	mOffset += size;

	return true;
}

void DiagnosticLogsCrash::ClearState()
{
	mLastLine = 0;
}

CHIP_ERROR DiagnosticLogsCrash::LoadCrashData()
{
	int ret = retention_is_valid(kCrashDataMem);
	VerifyOrReturnError(ret == 1, CHIP_ERROR_NOT_FOUND);
	ret = retention_read(kCrashDataMem, 0, reinterpret_cast<uint8_t *>(GetDescription()),
			     sizeof(*GetDescription()));
	VerifyOrReturnError(0 == ret, System::MapErrorZephyr(ret));

	return CHIP_NO_ERROR;
}

size_t DiagnosticLogsCrash::GetLogsSize()
{
	size_t outSize = 0;

	VerifyOrExit(CHIP_NO_ERROR == LoadCrashData(), );
	outSize = CalculateSize();

exit:
	/* Size calculation requires collecting all logs to get the summarized size. Clear state after that to start
	 * exact log collection from the beginning when requested. */
	ClearState();

	return outSize;
}

CHIP_ERROR DiagnosticLogsCrash::GetLogs(chip::MutableByteSpan &outBuffer, bool &outIsEndOfLog)
{
	size_t convertedSize = 0;
	CHIP_ERROR err = LoadCrashData();
	VerifyOrExit(CHIP_NO_ERROR == err, );

	convertedSize =
		ProcessConversionToLog(reinterpret_cast<char *>(outBuffer.data()), outBuffer.size(), outIsEndOfLog);
	outBuffer.reduce_size(convertedSize);

	ChipLogDetail(Zcl, "Getting log with size %zu. Is end of log: %s", convertedSize,
		      outIsEndOfLog ? "true" : "false");

exit:
	/* If it is the end of the log message we can clear the state, otherwise, we need to keep Esf data for
	 * the following bunch */
	if (outIsEndOfLog || convertedSize == 0 || err != CHIP_NO_ERROR) {
		ClearState();
	}

	return err;
}

CHIP_ERROR DiagnosticLogsCrash::FinishLogs()
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ
	return Clear();
#endif /* CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ */

	return CHIP_NO_ERROR;
}

CHIP_ERROR DiagnosticLogsCrash::Clear()
{
	return System::MapErrorZephyr(retention_clear(kCrashDataMem));
}

extern "C" {

/* The actual code of the z_fatal_error function assigned as __real_ for linker wrapping purpose.
It required -Wl,--wrap=z_fatal_error linker option added */
void __real_z_fatal_error(unsigned int reason, const struct arch_esf *esf);

/* Wrapped z_fatal_error function to implement saving crash data to the retention memory
and then call the _real_ function that contains the actual code of the z_fatal_error.
It required -Wl,--wrap=z_fatal_error linker option added */
void __wrap_z_fatal_error(unsigned int reason, const struct arch_esf *esf)
{
	/* Store the crash data into the retained RAM memory */
	if (esf) {
		Nrf::CrashDescription description = { *esf, reason, k_thread_name_get(_current), (uint32_t *)_current,
						      (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) };
		retention_write(DEVICE_DT_GET(DT_NODELABEL(crash_retention)), 0,
				reinterpret_cast<uint8_t *>(&description), sizeof(description));
	}

	/* Call the original z_fatal_error function */
	__real_z_fatal_error(reason, esf);
}
}
