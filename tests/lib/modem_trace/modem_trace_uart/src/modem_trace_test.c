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

static const nrfx_uarte_t *p_uarte_inst_in_use;

static const char *start_trace_at_cmd_fmt = "AT%%XMODEMTRACE=1,%hu";
static unsigned int exp_trace_mode;
static int nrf_modem_at_printf_retval;
static bool exp_trace_stop;

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

void setUp(void)
{
	mock_nrfx_uarte_Init();
}

void tearDown(void)
{
	mock_nrfx_uarte_Verify();

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
		TEST_ASSERT_EQUAL_STRING("AT%%XMODEMTRACE=0", fmt);
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

/* Test that nrf_modem_lib_trace_start returns error when the modem trace module was not
 * initialized.
 */
void test_modem_trace_start_when_not_initialized(void)
{
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));
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
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_COREDUMP_ONLY));
}

void test_modem_trace_start_all(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));
}

void test_modem_trace_start_ip_only(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_IP_ONLY, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_IP_ONLY));
}

void test_modem_trace_start_lte_ip(void)
{
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_LTE_IP, 0);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_LTE_IP));
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

	modem_trace_init_with_uart_transport();

	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));

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
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));

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

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));
}

/* Test nrf_modem_lib_trace_stop when nrf_modem_at_printf returns fails. */
void test_modem_trace_stop_when_nrf_modem_at_printf_fails(void)
{
	/* Make nrf_modem_at_printf return failure. */
	exp_trace_stop = true;
	nrf_modem_at_printf_retval = -1;

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, nrf_modem_lib_trace_stop());
}

/* Test nrf_modem_lib_trace_stop when nrf_modem_at_printf returns success. */
void test_modem_trace_stop(void)
{
	/* Make nrf_modem_at_printf return failure. */
	exp_trace_stop = true;
	nrf_modem_at_printf_retval = 0;

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_stop());
}

void main(void)
{
	(void)unity_main();
}
