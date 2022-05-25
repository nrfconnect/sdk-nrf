/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <nrf_modem_lib_trace.h>
#include "mock_SEGGER_RTT.h"
#include "mock_nrf_modem.h"

extern int unity_main(void);

static int trace_rtt_channel;

static const char *start_trace_at_cmd_fmt = "AT%%XMODEMTRACE=1,%hu";
static unsigned int exp_trace_mode;
static int nrf_modem_at_printf_retval;

static K_HEAP_DEFINE(trace_heap, CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE);

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

void setUp(void)
{
	mock_SEGGER_RTT_Init();
	mock_nrf_modem_Init();
}

void tearDown(void)
{
	mock_SEGGER_RTT_Verify();
	mock_nrf_modem_Verify();
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
	va_list args;
	unsigned int trace_mode;

	TEST_ASSERT_EQUAL_STRING(start_trace_at_cmd_fmt, fmt);

	va_start(args, fmt);
	trace_mode = va_arg(args, unsigned int);
	va_end(args);

	TEST_ASSERT_EQUAL(exp_trace_mode, trace_mode);

	return nrf_modem_at_printf_retval;
}

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

static const uint8_t *trace_processed_callback_buf_expect;
static uint32_t trace_processed_callback_len_expect;
static int trace_processed_callback_retval;

static int trace_processed_callback_stub(const uint8_t *buf, uint32_t len, int cmock_calls)
{
	TEST_ASSERT_EQUAL(buf, trace_processed_callback_buf_expect);
	TEST_ASSERT_EQUAL(len, trace_processed_callback_len_expect);
	return trace_processed_callback_retval;
}

/* Custom mock written for nrf_modem_trace_processed_callback since the regular mock
 * function, __wrap_nrf_modem_trace_processed_callback_ExpectAndReturn, erroneously
 * calls the fault handler when compiler is optimizing for size.
 */
static void trace_processed_callback_ExpectAndReturn(const uint8_t *buf, uint32_t len, int retval)
{
	trace_processed_callback_buf_expect = buf;
	trace_processed_callback_len_expect = len;
	trace_processed_callback_retval = retval;
	__wrap_nrf_modem_trace_processed_callback_Stub(trace_processed_callback_stub);
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

static void trace_session_setup_with_rtt_transport(void)
{
	__wrap_SEGGER_RTT_AllocUpBuffer_AddCallback(&rtt_allocupbuffer_callback);
	__wrap_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init(&trace_heap));

	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));
}

void test_modem_trace_init_rtt_transport_medium(void)
{
	trace_rtt_channel = 1;

	__wrap_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(trace_rtt_channel);
	__wrap_SEGGER_RTT_AllocUpBuffer_AddCallback(&rtt_allocupbuffer_callback);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init(&trace_heap));
}

static void expect_rtt_write(const uint8_t *buffer, uint16_t size)
{
	__wrap_SEGGER_RTT_ASM_WriteSkipNoLock_ExpectAndReturn(trace_rtt_channel, buffer, size,
							      size);
}

/* Test that when RTT is configured as trace transport, the traces are forwarded to RTT API. */
void test_modem_trace_forwarding_to_rtt(void)
{
	const uint16_t sample_trace_buffer_size = 300;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	trace_rtt_channel = 1;

	trace_session_setup_with_rtt_transport();

	/* Simulate the reception of modem trace and expect the RTT API to be called.
	 * Since the trace buffer size is larger than NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE,
	 * the modem_trace module should fragment the buffer and call the RTT API twice.
	 */
	expect_rtt_write(sample_trace_data, CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE);
	expect_rtt_write(&sample_trace_data[CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE],
			 sizeof(sample_trace_data) -
				 CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE);

	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));

	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread pick up the trace from its fifo and send it
	 * over RTT.
	 */
	k_sleep(K_MSEC(1));
}

/* Test that the module drops traces if RTT channel allocation had failed. */
void test_modem_trace_when_rtt_channel_allocation_fails(void)
{
	/* Simulate failure by returning negative rtt channel. */
	__wrap_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(-1);

	TEST_ASSERT_EQUAL(-EBUSY, nrf_modem_lib_trace_init(&trace_heap));
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));

	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Verify that nrf_modem_trace_processed_callback is called even if the channel allocation
	 * had failed.
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	/* Simulate the reception of modem trace and expect no RTT API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_process(sample_trace_data,
							      sizeof(sample_trace_data)));
}

/* Test that the module drops traces and returns error when it was not initialized. */
void test_modem_trace_process_when_not_initialized(void)
{
	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Verify that nrf_modem_trace_processed_callback is called even if the trace module was
	 * not initialized.
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	/* Simulate the reception of modem trace and expect no RTT API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_process(sample_trace_data,
							      sizeof(sample_trace_data)));
}

void main(void)
{
	(void)unity_main();
}
