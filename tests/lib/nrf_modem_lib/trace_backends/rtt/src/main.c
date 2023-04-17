/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <unity.h>
#include <zephyr/kernel.h>

#include "trace_backend.h"

extern struct nrf_modem_lib_trace_backend trace_backend;

#include "cmock_SEGGER_RTT.h"

#define BACKEND_RTT_BUF_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE

static int callback(size_t len)
{
	return 0;
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

static int trace_rtt_channel;

static int rtt_allocupbuffer_callback(const char *sName, void *pBuffer, unsigned int BufferSize,
				      unsigned int Flags, int no_of_calls)
{
	char *exp_sName = "modem_trace";

	TEST_ASSERT_EQUAL_CHAR_ARRAY(exp_sName, sName, sizeof(exp_sName));
	TEST_ASSERT_NOT_EQUAL(NULL, pBuffer);
	TEST_ASSERT_EQUAL(BACKEND_RTT_BUF_SIZE, BufferSize);
	TEST_ASSERT_EQUAL(SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL, Flags);

	return trace_rtt_channel;
}

void test_trace_backend_init_rtt(void)
{
	int ret;

	trace_rtt_channel = 1;

	__cmock_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(trace_rtt_channel);
	__cmock_SEGGER_RTT_AllocUpBuffer_AddCallback(&rtt_allocupbuffer_callback);

	ret = trace_backend.init(callback);

	TEST_ASSERT_EQUAL(0, ret);
}

void test_trace_backend_init_rtt_ebusy(void)
{
	int ret;

	/* Simulate failure by returning negative RTT channel. */
	__cmock_SEGGER_RTT_AllocUpBuffer_ExpectAnyArgsAndReturn(-1);

	ret = trace_backend.init(callback);
	TEST_ASSERT_EQUAL(-EBUSY, ret);
}

void test_trace_backend_init_rtt_efault(void)
{
	int ret;

	ret = trace_backend.init(NULL);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

/* Test that when RTT is configured as trace transport, the traces are forwarded to RTT API. */
void test_trace_backend_write_rtt(void)
{
	/* Bigger buffer so that we have to write twice */
	const uint8_t sample_trace_data[BACKEND_RTT_BUF_SIZE + 64];

	test_trace_backend_init_rtt();

	/* Since the trace buffer size is larger than NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE,
	 * the modem_trace module should fragment the buffer and call the RTT API twice.
	 */
	__cmock_SEGGER_RTT_WriteNoLock_ExpectAndReturn(
		trace_rtt_channel, sample_trace_data, BACKEND_RTT_BUF_SIZE, BACKEND_RTT_BUF_SIZE);

	uint32_t remaining = sizeof(sample_trace_data) - BACKEND_RTT_BUF_SIZE;

	__cmock_SEGGER_RTT_WriteNoLock_ExpectAndReturn(
		trace_rtt_channel, &sample_trace_data[BACKEND_RTT_BUF_SIZE], remaining, remaining);

	/* Simulate the reception of modem trace and expect the RTT API to be called. */
	trace_backend.write(sample_trace_data, sizeof(sample_trace_data));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
