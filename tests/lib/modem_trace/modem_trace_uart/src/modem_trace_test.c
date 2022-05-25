/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <string.h>

#include "nrf_modem_lib_trace.h"
#include "mock_nrf_modem.h"
#include "mock_nrfx_uarte.h"
#include "mock_pinctrl.h"

extern int unity_main(void);

static const nrfx_uarte_t *p_uarte_inst_in_use;
/* Variable to store the event_handler registered by the modem_trace module.*/
static nrfx_uarte_event_handler_t uarte_callback;

static const char *start_trace_at_cmd_fmt = "AT%%XMODEMTRACE=1,%hu";
static unsigned int exp_trace_mode;
static int nrf_modem_at_printf_retval;

static K_HEAP_DEFINE(trace_heap, CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE);

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

void nrfx_isr(const void *irq_handler)
{
	/* Declared only for pleasing linker. Never expected to be called. */
	TEST_ASSERT(false);
}

void setUp(void)
{
	mock_nrfx_uarte_Init();
	mock_nrf_modem_Init();
	mock_pinctrl_Init();
}

void tearDown(void)
{
	mock_nrfx_uarte_Verify();
	mock_nrf_modem_Verify();
	mock_pinctrl_Verify();

	p_uarte_inst_in_use = NULL;
	uarte_callback = NULL;
}

static void uart_tx_done_simulate(const uint8_t *const data, size_t len)
{
	nrfx_uarte_event_t uarte_event;

	uarte_event.type = NRFX_UARTE_EVT_TX_DONE;
	uarte_event.data.rxtx.bytes = len;
	uarte_event.data.rxtx.p_data = (uint8_t *)data;

	uarte_callback(&uarte_event, NULL);
}

static void uart_tx_error_simulate(void)
{
	nrfx_uarte_event_t uarte_event;

	uarte_event.type = NRFX_UARTE_EVT_ERROR;

	uarte_callback(&uarte_event, NULL);
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

static nrfx_err_t nrfx_uarte_init_callback(nrfx_uarte_t const *p_instance,
					   nrfx_uarte_config_t const *p_config,
					   nrfx_uarte_event_handler_t event_handler,
					   int no_of_calls)
{
	TEST_ASSERT_NOT_EQUAL(NULL, p_config);
	TEST_ASSERT_EQUAL(true, p_config->skip_gpio_cfg);
	TEST_ASSERT_EQUAL(true, p_config->skip_psel_cfg);
	TEST_ASSERT_EQUAL(NRF_UARTE_HWFC_DISABLED, p_config->hal_cfg.hwfc);
	TEST_ASSERT_EQUAL(NRF_UARTE_PARITY_EXCLUDED, p_config->hal_cfg.parity);
	TEST_ASSERT_EQUAL(NRF_UARTE_BAUDRATE_1000000, p_config->baudrate);
	TEST_ASSERT_EQUAL(1, p_config->interrupt_priority);
	TEST_ASSERT_EQUAL(NULL, p_config->p_context);

	TEST_ASSERT_NOT_EQUAL(NULL, p_instance);
	TEST_ASSERT_EQUAL(NRFX_UARTE1_INST_IDX, p_instance->drv_inst_idx);
	p_uarte_inst_in_use = p_instance;

	TEST_ASSERT_NOT_EQUAL(NULL, event_handler);

	uarte_callback = event_handler;

	return NRFX_SUCCESS;
}

static void modem_trace_init_with_uart_transport(void)
{
	__wrap_pinctrl_lookup_state_ExpectAnyArgsAndReturn(0);
	__wrap_pinctrl_configure_pins_ExpectAnyArgsAndReturn(0);
	__wrap_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_SUCCESS);
	__wrap_nrfx_uarte_init_AddCallback(&nrfx_uarte_init_callback);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init(&trace_heap));
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
	__wrap_pinctrl_lookup_state_ExpectAnyArgsAndReturn(0);
	__wrap_pinctrl_configure_pins_ExpectAnyArgsAndReturn(0);
	__wrap_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_SUCCESS);
	__wrap_nrfx_uarte_init_AddCallback(&nrfx_uarte_init_callback);
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_init(&trace_heap));
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

	/* Simulate the reception of modem trace.*/
	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread pick up the trace from its fifo and send it
	 * over UART.
	 * Since the trace buffer size is larger than maximum possible easydma transfer size,
	 * the modem_trace module should fragment the buffer and call the UART API twice. This
	 * time it is expected to call it with length set to max_uart_frag_size.
	 */
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_SUCCESS);

	k_sleep(K_MSEC(1));

	uart_tx_done_simulate(sample_trace_data, max_uart_frag_size);

	/* Make the test thread sleep and expect the second part of the trace sent over UART.*/
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use,
					     &sample_trace_data[max_uart_frag_size],
					     sizeof(sample_trace_data) - max_uart_frag_size,
					     NRFX_SUCCESS);

	k_sleep(K_MSEC(1));

	uart_tx_done_simulate(&sample_trace_data[max_uart_frag_size],
			      sizeof(sample_trace_data) - max_uart_frag_size);

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread call the
	 * nrf_modem_trace_processed_callback() since all fragments of trace data have been
	 * sent over UART.
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	k_sleep(K_MSEC(1));
}

/* Test that verifies the behavior of modem trace module when its thread fails to take the tx
 * sempahore.
 */
void test_modem_trace_uart_tx_sem_take_failure(void)
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

	/* Simulate the reception of modem trace.*/
	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread pick up the trace from its fifo and send it
	 * over UART.
	 */
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_SUCCESS);

	k_sleep(K_MSEC(1));

	/* Simulate a condition when the UART TX is not completed for 100 ms (i.e
	 * UART_TX_WAIT_TIME_MS). The modem trace module is expected to not make any further
	 * calls to the UART API for this trace buffer. Instead it is expected to call
	 * nrf_modem_trace_processed_callback().
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	k_sleep(K_MSEC(100));

	/* Simulate a TX complete now and check if the modem trace module handles it gracefully. */
	uart_tx_done_simulate(sample_trace_data, max_uart_frag_size);

	k_sleep(K_MSEC(1));
}

/* Test that verifies the behavior of the modem trace module when UART API returns error. */
void test_modem_trace_uart_tx_returns_error(void)
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

	/* Simulate the reception of modem trace.*/
	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread pick up the trace from its fifo and send it
	 * over UART. Make the nrfx_uarte_tx return error. This should make the modem trace module
	 * abort sending the second part of the trace buffer and just call the
	 * nrf_modem_trace_processed_callback().
	 */
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_ERROR_NO_MEM);

	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	k_sleep(K_MSEC(1));
}

/* Test that verifies that the modem trace module handles an TX Error event from UART gracefully.
 */
void test_modem_trace_uart_tx_error(void)
{
	const uint32_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	modem_trace_init_with_uart_transport();

	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));

	/* Simulate the reception of modem trace.*/
	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread pick up the trace from its fifo and send it
	 * over UART.
	 */
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     sizeof(sample_trace_data), NRFX_SUCCESS);

	k_sleep(K_MSEC(1));

	uart_tx_error_simulate();

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread call the
	 * nrf_modem_trace_processed_callback().
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);
	k_sleep(K_MSEC(1));
}

/* Test that the module drops traces if UART Init had failed. */
void test_modem_trace_when_transport_uart_init_fails(void)
{
	/* Simulate uart init failure. */
	__wrap_pinctrl_lookup_state_ExpectAnyArgsAndReturn(0);
	__wrap_pinctrl_configure_pins_ExpectAnyArgsAndReturn(0);
	__wrap_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_ERROR_BUSY);

	TEST_ASSERT_EQUAL(-EBUSY, nrf_modem_lib_trace_init(&trace_heap));
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));

	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Verify that nrf_modem_trace_processed_callback is called even if uart init had failed. */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	/* Simulate the reception of modem trace and expect no UART API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_process(sample_trace_data,
							      sizeof(sample_trace_data)));
}

/* Test that the module drops traces and returns error when it was not initialized. */
void test_modem_trace_process_when_not_initialized(void)
{
	const uint16_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	/* Verify that nrf_modem_trace_processed_callback is called even if the trace module was not
	 * initialized.
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	/* Simulate the reception of modem trace and expect no UART API to be called. */
	TEST_ASSERT_EQUAL(-ENXIO, nrf_modem_lib_trace_process(sample_trace_data,
							      sizeof(sample_trace_data)));
}

/* Test that nrf_modem_lib_trace_process() forwards traces to the transport even if
 * nrf_modem_lib_trace_start() was not called, given that nrf_modem_lib_trace_init(...) was
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

	/* Make the test thread sleep so that the trace handler thread is allowed to run.
	 * This should make the trace handler thread pick up the trace from its fifo and send it
	 * over UART.
	 */
	k_sleep(K_MSEC(1));

	trace_processed_callback_ExpectAndReturn(sample_trace_data, sizeof(sample_trace_data), 0);

	uart_tx_done_simulate(sample_trace_data, sizeof(sample_trace_data));

	k_sleep(K_MSEC(1));
}

/* Test nrf_modem_lib_trace_start when nrf_modem_at_printf returns fails. */
void test_modem_trace_start_when_nrf_modem_at_printf_fails(void)
{
	modem_trace_init_with_uart_transport();

	/* Make nrf_modem_at_printf return failure. */
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, -1);

	TEST_ASSERT_EQUAL(-EOPNOTSUPP, nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL));
}

/* Test nrf_modem_lib_trace_stop */
void test_modem_trace_stop(void)
{
	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_stop());
}

/* Test no more traces sent to UART after trace_stop */
void test_modem_trace_no_traces_after_trace_stop(void)
{
	const uint8_t sample_trace_data[10];

	modem_trace_init_with_uart_transport();

	TEST_ASSERT_EQUAL(0, nrf_modem_lib_trace_stop());

	/* The test will fail if traces are forwarded because __wrap_nrfx_uarte_tx
	 * will be called more times than expected (0).
	 */
	trace_processed_callback_ExpectAndReturn(sample_trace_data,
		sizeof(sample_trace_data), 0);
	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));
}

/* Test more traces sent to UART after trace_stop then trace_start */
void test_modem_trace_more_traces_after_trace_stop_start(void)
{
	const uint8_t sample_trace_data[10];

	modem_trace_init_with_uart_transport();

	TEST_ASSERT_EQUAL_MESSAGE(0, nrf_modem_lib_trace_stop(), "stop failed");

	// Verify AT%%XMODEMTRACE command is sent
	nrf_modem_at_printf_ExpectTraceModeAndReturn(NRF_MODEM_LIB_TRACE_ALL, 0);
	TEST_ASSERT_EQUAL_MESSAGE(0,
		nrf_modem_lib_trace_start(NRF_MODEM_LIB_TRACE_ALL), "start failed");

	// Expect UART TX data
	__wrap_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     sizeof(sample_trace_data), NRFX_SUCCESS);
	nrf_modem_lib_trace_process(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));

	// Expect TX DONE triggers callback
	trace_processed_callback_ExpectAndReturn(sample_trace_data,
		sizeof(sample_trace_data), 0);
	uart_tx_done_simulate(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));
}

void main(void)
{
	(void)unity_main();
}
