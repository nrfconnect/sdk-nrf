/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <kernel.h>
#include <string.h>

#include "nrf_modem_lib_trace.h"
#include "mock_nrfx_uarte.h"

extern int unity_main(void);

static int64_t trace_stop_timestamp = INT64_MAX;
static const nrfx_uarte_t *p_uarte_inst_in_use;

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
	mock_nrfx_uarte_Init();
}

void tearDown(void)
{
	mock_nrfx_uarte_Verify();

	trace_stop_timestamp = INT64_MAX;
	p_uarte_inst_in_use = NULL;
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

static nrfx_err_t nrfx_uarte_init_callback(nrfx_uarte_t const *p_instance,
					   nrfx_uarte_config_t const *p_config,
					   nrfx_uarte_event_handler_t event_handler,
					   int no_of_calls)
{
	TEST_ASSERT_NOT_EQUAL(NULL, p_config);
	TEST_ASSERT_EQUAL(NRF_UARTE_PSEL_DISCONNECTED, p_config->pselcts);
	TEST_ASSERT_EQUAL(NRF_UARTE_PSEL_DISCONNECTED, p_config->pselrts);
	TEST_ASSERT_EQUAL(NRF_UARTE_HWFC_DISABLED, p_config->hal_cfg.hwfc);
	TEST_ASSERT_EQUAL(NRF_UARTE_PARITY_EXCLUDED, p_config->hal_cfg.parity);
	TEST_ASSERT_EQUAL(NRF_UARTE_BAUDRATE_1000000, p_config->baudrate);
	TEST_ASSERT_EQUAL(NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY, p_config->interrupt_priority);
	TEST_ASSERT_EQUAL(NULL, p_config->p_context);

	TEST_ASSERT_NOT_EQUAL(NULL, p_instance);
	TEST_ASSERT_EQUAL(NRFX_UARTE1_INST_IDX, p_instance->drv_inst_idx);
	p_uarte_inst_in_use = p_instance;

	TEST_ASSERT_EQUAL(NULL, event_handler);

	return NRFX_SUCCESS;
}

static void modem_trace_init_with_uart_transport(void)
{
	__wrap_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_SUCCESS);
	__wrap_nrfx_uarte_init_AddCallback(&nrfx_uarte_init_callback);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init());
}

static void trace_session_setup_with_uart_transport(uint16_t duration, uint32_t max_size)
{
	modem_trace_init_with_uart_transport();

	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);

	TEST_ASSERT_EQUAL(0,
			  nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, duration, max_size));
}

/* Utility function that sends a given number of bytes to the modem_trace module. */
static void traces_send(const uint32_t bytes_to_send)
{
	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	uint16_t num_full_trace_buffers = bytes_to_send / sample_trace_buffer_size;

	for (int i = 0; i < num_full_trace_buffers; i++) {
		__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
						     sizeof(sample_trace_data), NRFX_SUCCESS);

		nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));
	}

	uint16_t extra_bytes_needed = bytes_to_send % sample_trace_buffer_size;

	if (extra_bytes_needed != 0) {
		__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
						     extra_bytes_needed, NRFX_SUCCESS);

		nrf_modem_lib_trace_process(sample_trace_data, extra_bytes_needed);
	}
}

/* Test that nrf_modem_lib_trace_start returns error when the modem trace module was not
 * initialized.
 */
void test_modem_trace_start_when_not_initialized(void)
{
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, 0, 0));
}

void test_modem_trace_init_uart_transport_medium(void)
{
	__wrap_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_SUCCESS);
	__wrap_nrfx_uarte_init_AddCallback(&nrfx_uarte_init_callback);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init());
}

void test_modem_trace_start_coredump_only(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_COREDUMP_ONLY, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_COREDUMP_ONLY, 0, 10));
}

void test_modem_trace_start_all(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, 0, 10));
}

void test_modem_trace_start_ip_only(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_IP_ONLY, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_IP_ONLY, 0, 10));
}

void test_modem_trace_start_lte_ip(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_LTE_IP, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_LTE_IP, 0, 10));
}

/* Test that verifies that the modem_trace module runs a trace session only for a given duration.
 * The max size of the trace is set to zero (i.e no limit for max size of trace data).
 */
void test_modem_trace_start_with_duration_and_no_max_size(void)
{
	const uint16_t test_duration_in_seconds = 2;
	const uint16_t threshold_in_ms = 20;

	trace_session_setup_with_uart_transport(test_duration_in_seconds, NO_MAX_SIZE);

	int64_t trace_start_time_stamp = k_uptime_get();

	exp_trace_stop = true;

	k_sleep(K_MSEC((test_duration_in_seconds * 1000)));

	/* Verify that the trace session was stopped only after the required duration. */
	TEST_ASSERT_GREATER_OR_EQUAL((test_duration_in_seconds * 1000) - threshold_in_ms,
				     (trace_stop_timestamp - trace_start_time_stamp));
}

/* Test that verifies that the modem_trace module runs a trace session only until specified amount
 * of trace data is received when the duration parameter is set to zero (no timeout).
 * In this test case, the sizes of all individual trace sent are the same. The max size configured
 * is divisible the size of individual trace buffers.
 */
void test_modem_trace_stop_when_max_size_reached(void)
{
	const uint32_t test_max_data_size_bytes = 200;

	trace_session_setup_with_uart_transport(NO_TIMEOUT, test_max_data_size_bytes);

	const uint16_t last_sample_trace_data_size = 10;
	const uint8_t last_sample_trace_data[last_sample_trace_data_size];

	/* Simulate the reception of modem traces until just before max size is reached. */
	traces_send(test_max_data_size_bytes - sizeof(last_sample_trace_data));

	/* In the next call to nrf_modem_lib_trace_process(), we send just enough data to reach max
	 * size. So we expect the module to send that trace data to the transport medium and then
	 * disable the traces.
	 */
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, last_sample_trace_data,
					     sizeof(last_sample_trace_data), NRFX_SUCCESS);

	exp_trace_stop = true;

	nrf_modem_lib_trace_process(last_sample_trace_data, sizeof(last_sample_trace_data));
}

/* Test that verifies that the modem_trace module stops a trace session when it receives a trace
 * that will not fit in the max allowed size configured.
 */
void test_modem_trace_stop_when_received_trace_wont_fit_in_max_size_allocated(void)
{
	const uint32_t test_max_data_size_bytes = 200;

	trace_session_setup_with_uart_transport(NO_TIMEOUT, test_max_data_size_bytes);

	/* Simulate the reception of modem traces until just before max size is reached. */
	traces_send(test_max_data_size_bytes - 1);

	/* In the next call to nrf_modem_lib_trace_process(), we send trace data that will not fit
	 * in the max size configured.
	 * Expect the module to disable traces. Do not expect the trace to be forwarded to the
	 * transport medium.
	 */
	exp_trace_stop = true;
	const uint8_t test_trace_data_that_wont_fit[10];

	nrf_modem_lib_trace_process(test_trace_data_that_wont_fit,
				    sizeof(test_trace_data_that_wont_fit));
}

/* Test that simulates a scenario when the application had provided both duration and max_size
 * parameters to the nrf_modem_lib_trace_start() API. And the max size had reached before duration.
 * The test verifies that verifies that the modem_trace module stops trace session as soon as
 * duration is reached.
 */
void test_modem_trace_stop_when_max_size_reached_before_duration(void)
{
	const uint32_t test_max_data_size_bytes = 2000;
	const uint32_t test_duration_in_seconds = 2;

	trace_session_setup_with_uart_transport(test_duration_in_seconds, test_max_data_size_bytes);

	exp_trace_stop = true;

	/* Simulate the reception of traces until max_size is reached. */
	traces_send(test_max_data_size_bytes);

	/* Sleep for the test duration to verify no other calls to nrf_modem_at_printf is made. */
	k_sleep(K_SECONDS(test_duration_in_seconds));
}

/* Test that simulates a scenario when the application had provided both duration and max_size
 * parameters to the nrf_modem_lib_trace_start() API. And the duration had reached before the
 * number of trace bytes specified by max_size parameter was received.
 * The test verifies that verifies that the modem_trace module stops trace session as soon as
 * the max_size limit is reached.
 */
void test_modem_trace_stop_when_duration_reached_before_max_size(void)
{
	const uint32_t test_max_data_size_bytes = 2000;
	const uint32_t test_duration_in_seconds = 2;

	trace_session_setup_with_uart_transport(test_duration_in_seconds, test_max_data_size_bytes);

	int64_t trace_start_time_stamp = k_uptime_get();

	/* Simulate the reception of some traces without reaching the max_size limit. */
	traces_send(test_max_data_size_bytes - 1);

	exp_trace_stop = true;

	k_sleep(K_SECONDS(test_duration_in_seconds));

	/* Verify that the trace session was stopped only after the required duration. */
	TEST_ASSERT_GREATER_OR_EQUAL(test_duration_in_seconds * 1000,
				     (trace_stop_timestamp - trace_start_time_stamp));
}

/* Test that when UART is configured as trace transport, the traces are forwarded to UART API. */
void test_modem_trace_forwarding_to_uart(void)
{
	const uint32_t max_uart_frag_size = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;
	/* Configure to send data higher than maximum number of bytes the modem_trace module will
	 * send over UART.
	 */
	const uint32_t sample_trace_buffer_size = max_uart_frag_size + 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	trace_session_setup_with_uart_transport(NO_TIMEOUT, NO_MAX_SIZE);

	/* Simulate the reception of modem trace and expect the UART API to be called.
	 * Since the trace buffer size is larger than maximum possible easydma transfer size,
	 * the modem_trace module should fragment the buffer and call the UART API twice.
	 */
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_SUCCESS);

	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use,
					     &sample_trace_data[max_uart_frag_size],
					     sizeof(sample_trace_data) - max_uart_frag_size,
					     NRFX_SUCCESS);

	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));
}

/* Test that the module drops traces if UART Init had failed. */
void test_modem_trace_when_transport_uart_init_fails(void)
{
	/* Simulate uart init failure. */
	__wrap_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_ERROR_BUSY);

	TEST_ASSERT_EQUAL(-EBUSY, nrf_modem_lib_trace_init());
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, 0, 0));

	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Simulate the reception of modem trace and expect no UART API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO,
			nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data)));
}

/* Test that the module drops traces and returns error when it was not initialized. */
void test_modem_trace_process_when_not_initialized(void)
{
	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Simulate the reception of modem trace and expect no UART API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO,
			nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data)));
}

/* Test that nrf_modem_lib_trace_process() forwards traces to the transport even if
 * nrf_modem_lib_trace_start() was not called, given that nrf_modem_lib_trace_init() was
 * successful.
 */
void test_modem_trace_process_when_modem_trace_start_was_not_called(void)
{
	test_modem_trace_init_uart_transport_medium();

	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     sample_trace_buffer_size, NRFX_SUCCESS);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_process(sample_trace_data,
						sizeof(sample_trace_data)));
}

/* Test nrf_modem_lib_trace_start when nrf_modem_at_printf returns fails. */
void test_modem_trace_start_when_nrf_modem_at_printf_fails(void)
{
	modem_trace_init_with_uart_transport();

	/* Make nrf_modem_at_printf return failure. */
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, -1);

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL, 0, 0));
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
