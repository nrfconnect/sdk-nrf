/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <unity.h>

#include "trace_backend.h"

#include "cmock_nrfx_uarte.h"
#include "cmock_pinctrl.h"

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

extern struct nrf_modem_lib_trace_backend trace_backend;

static const nrfx_uarte_t *p_uarte_inst_in_use;
/* Variable to store the event_handler registered by the modem_trace module.*/
static nrfx_uarte_event_handler_t uarte_callback;

static int empty_callback(size_t len)
{
	return 0;
}

void nrfx_isr(const void *irq_handler)
{
	/* Declared only for pleasing linker. Never expected to be called. */
	TEST_ASSERT(false);
}

void tearDown(void)
{
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

static const uint8_t *trace_data;
static uint32_t trace_data_len;
static K_SEM_DEFINE(receive_traces_sem, 0, 1);
static bool is_waiting_on_traces;

static void send_traces_for_processing(const uint8_t *data, uint32_t len)
{
	trace_data = data;
	trace_data_len = len;
	k_sem_give(&receive_traces_sem);
}

#define TRACE_TEST_THREAD_STACK_SIZE 512
#define TRACE_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

static void trace_test_thread(void)
{
	while (1) {
		is_waiting_on_traces = true;
		k_sem_take(&receive_traces_sem, K_FOREVER);
		is_waiting_on_traces = false;

		trace_backend.write(trace_data, trace_data_len);
	}
}

K_THREAD_DEFINE(trace_test_thread_id, TRACE_TEST_THREAD_STACK_SIZE, trace_test_thread,
		NULL, NULL, NULL, TRACE_THREAD_PRIORITY, 0, 0);

/* Test that uart trace backend returns zero when NRFX UART Init succeeds. */
void test_trace_backend_init_uart(void)
{
	int ret;

	__cmock_pinctrl_lookup_state_ExpectAnyArgsAndReturn(0);
	__cmock_pinctrl_configure_pins_ExpectAnyArgsAndReturn(0);
	__cmock_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_SUCCESS);
	__cmock_nrfx_uarte_init_AddCallback(&nrfx_uarte_init_callback);

	ret = trace_backend.init(empty_callback);

	TEST_ASSERT_EQUAL(0, ret);
}

/* Test that uart trace backend return error when NRFX UART Init fails. */
void test_trace_backend_init_uart_ebusy(void)
{
	int ret;

	/* Simulate uart init failure. */
	__cmock_pinctrl_lookup_state_ExpectAnyArgsAndReturn(0);
	__cmock_pinctrl_configure_pins_ExpectAnyArgsAndReturn(0);
	__cmock_nrfx_uarte_init_ExpectAnyArgsAndReturn(NRFX_ERROR_BUSY);

	ret = trace_backend.init(empty_callback);

	TEST_ASSERT_EQUAL(-EBUSY, ret);
}

/* Test uart trace bckend deinit. */
void test_trace_backend_deinit_uart(void)
{
	int ret;

	test_trace_backend_init_uart();

	__cmock_nrfx_uarte_uninit_Expect(p_uarte_inst_in_use);
	__cmock_pinctrl_lookup_state_ExpectAnyArgsAndReturn(0);
	__cmock_pinctrl_configure_pins_ExpectAnyArgsAndReturn(0);

	ret = trace_backend.deinit();

	TEST_ASSERT_EQUAL(0, ret);
}

/* Test that uart trace backend divides large traces into several pieces
 * and forwards them one by one to NRFX UART.
 */
void test_trace_backend_write_uart(void)
{
	const uint32_t max_uart_frag_size = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;
	/* Configure to send data higher than maximum number of bytes the UART trace backend
	 * will send in 'one go' over UART.
	 */
	const uint32_t sample_trace_buffer_size = max_uart_frag_size + 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	test_trace_backend_init_uart();

	__cmock_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_SUCCESS);

	/* Simulate reception of a modem trace and let trace_test_thread run. */
	send_traces_for_processing(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));

	/* Simulate uart callback. Done sending 1st part of modem trace. */
	uart_tx_done_simulate(sample_trace_data, max_uart_frag_size);

	__cmock_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use,
					     &sample_trace_data[max_uart_frag_size],
					     sizeof(sample_trace_data) - max_uart_frag_size,
					     NRFX_SUCCESS);

	/* Let trace_test_thread run. */
	k_sleep(K_MSEC(1));

	/* Simulate uart callback. Done sending 2nd part of modem trace. */
	uart_tx_done_simulate(&sample_trace_data[max_uart_frag_size],
			      sizeof(sample_trace_data) - max_uart_frag_size);

	/* Let trace_test_thread run. */
	k_sleep(K_MSEC(1));

	TEST_ASSERT_EQUAL(0, k_sem_count_get(&receive_traces_sem));
	TEST_ASSERT_EQUAL(true, is_waiting_on_traces);
}

/* Test that verifies the behavior of uart trace backend when it times out, waiting
 * on the tx semaphore to send additional data of the received trace.
 */
void test_trace_backend_write_uart_nrfx_uarte_evt_tx_done(void)
{
	const uint32_t max_uart_frag_size = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;
	/* Configure to send data higher than maximum number of bytes the UART trace backend
	 * will send in 'one go' over UART.
	 */
	const uint32_t sample_trace_buffer_size = max_uart_frag_size + 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	test_trace_backend_init_uart();

	__cmock_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_SUCCESS);

	/* Simulate reception of a modem trace and let trace_test_thread run. */
	send_traces_for_processing(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));

	/* Simulate a condition when the UART TX is not completed for 100 ms (i.e
	 * UART_TX_WAIT_TIME_MS). The modem trace module is expected to not make any further
	 * calls to the UART API for this trace buffer.
	 */
	k_sleep(K_MSEC(100));

	/* Simulate a TX complete now and check if the modem trace module handles it gracefully. */
	uart_tx_done_simulate(sample_trace_data, max_uart_frag_size);

	/* Let trace_test_thread run. */
	k_sleep(K_MSEC(1));

	TEST_ASSERT_EQUAL(0, k_sem_count_get(&receive_traces_sem));
	TEST_ASSERT_EQUAL(true, is_waiting_on_traces);
}

/* Test that verifies the behavior of the uart trace backend when UART API returns error. */
void test_trace_backend_write_uart_nrfx_error_no_mem(void)
{
	const uint32_t max_uart_frag_size = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;
	/* Configure to send data higher than maximum number of bytes the UART trace backend
	 * will send in 'one go' over UART.
	 */
	const uint32_t sample_trace_buffer_size = max_uart_frag_size + 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	test_trace_backend_init_uart();

	/* Make the nrfx_uarte_tx return error. This should make the modem trace module
	 * abort sending the second part of the trace buffer.
	 */
	__cmock_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     max_uart_frag_size, NRFX_ERROR_NO_MEM);

	/* Simulate reception of a modem trace and let trace_test_thread run. */
	send_traces_for_processing(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));

	TEST_ASSERT_EQUAL(0, k_sem_count_get(&receive_traces_sem));
	TEST_ASSERT_EQUAL(true, is_waiting_on_traces);
}

/* Test that verifies that the uart trace backend handles an TX Error event from UART gracefully.
 */
void test_trace_backend_write_uart_nrfx_uarte_evt_error(void)
{
	const uint32_t sample_trace_buffer_size = 10;
	const uint8_t sample_trace_data[sample_trace_buffer_size];

	test_trace_backend_init_uart();

	__cmock_nrfx_uarte_tx_ExpectAndReturn(p_uarte_inst_in_use, sample_trace_data,
					     sizeof(sample_trace_data), NRFX_SUCCESS);

	/* Simulate reception of a modem trace and let trace_test_thread run. */
	send_traces_for_processing(sample_trace_data, sizeof(sample_trace_data));
	k_sleep(K_MSEC(1));

	uart_tx_error_simulate();

	/* Let trace_test_thread run. */
	k_sleep(K_MSEC(1));

	TEST_ASSERT_EQUAL(0, k_sem_count_get(&receive_traces_sem));
	TEST_ASSERT_EQUAL(true, is_waiting_on_traces);
}

int main(void)
{
	(void)unity_main();

	return 0;
}
