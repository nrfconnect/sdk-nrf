/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <kernel.h>
#include <string.h>
#include <nrf_modem_lib_trace.h>
#include "mock_SEGGER_RTT.h"

extern int unity_main(void);

static int64_t trace_stop_timestamp = INT64_MAX;
static int trace_rtt_channel;

static const char *start_trace_at_cmd_fmt = "AT%%XMODEMTRACE=1,%hu";
static unsigned int exp_trace_mode;
static int nrf_modem_at_printf_retval;
static bool exp_trace_stop;

#define NO_TIMEOUT 0
#define NO_MAX_SIZE 0

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

void setUp(void)
{
	mock_SEGGER_RTT_Init();
}

void tearDown(void)
{
	mock_SEGGER_RTT_Verify();

	trace_stop_timestamp = INT64_MAX;
	exp_trace_stop = false;
}

static void nrf_modem_at_printf_ExpectTraceModeAndReturn(unsigned int trace_mode, int retval)
{
	exp_trace_mode = trace_mode;
	nrf_modem_at_printf_retval = retval;
}

/* Custom mock written for nrf_modem_at_printf since CMock can not be made to test parameters
 * of variadic functions.
 */
int nrf_modem_at_printf(const char *fmt, ...)
{
	if (exp_trace_stop) {
		if (strcmp(fmt, "AT%%XMODEMTRACE=0") == 0) {
			trace_stop_timestamp = k_uptime_get();
		} else {
			TEST_ASSERT_EQUAL_STRING("AT%%XMODEMTRACE=0", fmt);
		}
	} else {
		va_list args;
		unsigned int trace_mode;

		TEST_ASSERT_EQUAL_STRING(start_trace_at_cmd_fmt, fmt);

		va_start(args, fmt);
		trace_mode = va_arg(args, unsigned int);
		va_end(args);

		TEST_ASSERT_EQUAL(exp_trace_mode, trace_mode);
	}

	return nrf_modem_at_printf_retval;
}

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

static int rtt_allocupbuffer_callback(const char *sName, void *pBuffer, unsigned int BufferSize,
				      unsigned int Flags, int no_of_calls)
{
	char *exp_sName = "modem_trace";

	TEST_ASSERT_EQUAL_CHAR_ARRAY(exp_sName, sName, sizeof(exp_sName));
	TEST_ASSERT_NOT_EQUAL(NULL, pBuffer);
	TEST_ASSERT_EQUAL(CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE, BufferSize);
	TEST_ASSERT_EQUAL(SEGGER_RTT_MODE_NO_BLOCK_SKIP, Flags);

	return trace_rtt_channel;
}

static void modem_trace_init_with_rtt_transport(void)
{
	__wrap_SEGGER_RTT_AllocUpBuffer_AddCallback(&rtt_allocupbuffer_callback);
	__wrap_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init());
}

static void trace_session_setup_with_rtt_transport(uint16_t duration, uint32_t max_size)
{
	modem_trace_init_with_rtt_transport();

	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);

	TEST_ASSERT_EQUAL(0,
			  nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, duration, max_size));
}

void test_modem_trace_init_rtt_transport_medium(void)
{
	trace_rtt_channel = 1;

	__wrap_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(trace_rtt_channel);
	__wrap_SEGGER_RTT_AllocUpBuffer_AddCallback(&rtt_allocupbuffer_callback);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init());
}

static void expect_rtt_write(const uint8_t *buffer, uint16_t size)
{
#if CONFIG_BOARD_NATIVE_POSIX
	/* For native_posix platform, the macro RTT_USE_ASM does not get defined. Due to this,
	 * the SEGGER_RTT_WriteSkipNoLock() gets called instead of the assembly version of the API
	 * (SEGGER_RTT_ASM_Write_skipNoLock). See SEGGER_RTT.h for how the macro is used.
	 */
	__wrap_SEGGER_RTT_WriteSkipNoLock_ExpectAndReturn(trace_rtt_channel, buffer, size, size);
#else
	__wrap_SEGGER_RTT_ASM_WriteSkipNoLock_ExpectAndReturn(trace_rtt_channel, buffer, size,
							size);
#endif
}

/* Test that when RTT is configured as trace transport, the traces are forwarded to RTT API. */
void test_modem_trace_forwarding_to_rtt(void)
{
	const uint16_t sample_trace_buffer_size = 300;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	trace_rtt_channel = 1;

	trace_session_setup_with_rtt_transport(NO_TIMEOUT, NO_MAX_SIZE);

	/* Simulate the reception of modem trace and expect the RTT API to be called.
	 * Since the trace buffer size is larger than NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE,
	 * the modem_trace module should fragment the buffer and call the RTT API twice.
	 */
	expect_rtt_write(sample_trace_data, CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE);
	expect_rtt_write(&sample_trace_data[CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE],
		sizeof(sample_trace_data) - CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE);

	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));
}

/* Test that the module drops traces if RTT channel allocation had failed. */
void test_modem_trace_when_rtt_channel_allocation_fails(void)
{
	/* Simulate failure by returning negative rtt channel. */
	__wrap_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(-1);

	TEST_ASSERT_EQUAL(-EBUSY, nrf_modem_lib_trace_init());
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, 0, 0));

	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Simulate the reception of modem trace and expect no RTT API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO,
			nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data)));
}

/* Test that the module drops traces and returns error when it was not initialized. */
void test_modem_trace_process_when_not_initialized(void)
{
	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Simulate the reception of modem trace and expect no RTT API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO,
			nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data)));
}

/* Test nrf_modem_lib_trace_stop when nrf_modem_at_printf returns fails. */
void test_modem_trace_stop_when_nrf_modem_at_printf_fails(void)
{
	/* Make nrf_modem_at_printf return failure. */
	exp_trace_stop = true;
	nrf_modem_at_printf_retval = -1;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, nrf_modem_lib_trace_stop());
}

void main(void)
{
	(void)unity_main();
}
