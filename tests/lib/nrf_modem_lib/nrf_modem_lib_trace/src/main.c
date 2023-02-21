/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <unity.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/logging/log.h>
#include <syscalls/rand32.h>
#include <modem/nrf_modem_lib.h>
#include <modem/trace_backend.h>

#include "nrf_modem_lib_trace.h"

#include "cmock_trace_backend_mock.h"
#include "cmock_nrf_modem.h"
#include "cmock_nrf_modem_trace.h"
#include "cmock_nrf_modem_os.h"

LOG_MODULE_REGISTER(trace_test, CONFIG_NRF_MODEM_LIB_TRACE_TEST_LOG_LEVEL);

extern int unity_main(void);

extern struct nrf_modem_lib_trace_backend trace_backend;

static void clear_rw_frags(void);

static const char *trace_level_set_fmt = "AT%%XMODEMTRACE=1,%d";
static const char *at_printf_fmt_expected;
static enum nrf_modem_lib_trace_level at_printf_trace_level_expected;
static int at_printf_return;

#define MAX_N_STATIC_FRAGS 64
static struct nrf_modem_trace_data read_frags[MAX_N_STATIC_FRAGS];
static struct nrf_modem_trace_data write_frags[MAX_N_STATIC_FRAGS];

#define TRACE_THREAD_HANDLER_MULTI_RUNS 32

K_FIFO_DEFINE(get_fifo);
K_FIFO_DEFINE(write_fifo);

K_SEM_DEFINE(backend_deinit_sem, 0, 1);

static int nrf_modem_trace_get_error;
static int nrf_modem_trace_get_cmock_num_calls;
static int trace_backend_write_error;
static int trace_backend_write_cmock_num_calls;

static void nrf_modem_at_printf_ExpectTraceLevelAndReturn(
	const char *fmt, const enum nrf_modem_lib_trace_level trace_level, int retval);

static int callback_evt;

/* This is the override for the _weak callback. */
void nrf_modem_lib_trace_callback(enum nrf_modem_lib_trace_event evt)
{
	callback_evt = evt;
}

void setUp(void)
{
	nrf_modem_trace_get_error = 0;
	nrf_modem_trace_get_cmock_num_calls = 0;
	trace_backend_write_error = 0;
	trace_backend_write_cmock_num_calls = 9;

	nrf_modem_at_printf_ExpectTraceLevelAndReturn(
		"AT%%XMODEMTRACE=1,%d", NRF_MODEM_LIB_TRACE_LEVEL_FULL, 0);

	clear_rw_frags();
}

static void NRF_MODEM_LIB_ON_INIT_callback(void)
{
	STRUCT_SECTION_FOREACH(nrf_modem_lib_init_cb, e) {
		e->callback(0, e->context);
	}
}

static void generate_trace_frag(struct nrf_modem_trace_data *frag)
{
	/* Generate a random trace data pointer and a random length.
	 * Here we do not care about keeping track of crossover since
	 * we shall only test that the trace fragment provided by the modem
	 * library coincides with the data pointer and length the trace backend
	 * receives.
	 * In this project `CONFIG_TEST_RANDOM_GENERATOR` is set so that each
	 * subsequent call to `sys_rand32_get` is deterministic and reproducible.
	 */
	frag->data = (void *)sys_rand32_get();
	frag->len = sys_rand32_get() % 1024 + 1;
}

static void clear_rw_frags(void)
{
	memset(read_frags, 0, sizeof(read_frags));
	memset(write_frags, 0, sizeof(write_frags));
}

static void wait_trace_deinit(void)
{
	k_sem_take(&backend_deinit_sem, K_FOREVER);
}

static void nrf_modem_at_printf_ExpectTraceLevelAndReturn(
	const char *fmt, const enum nrf_modem_lib_trace_level trace_level, int retval)
{
	at_printf_fmt_expected = fmt;
	at_printf_trace_level_expected = trace_level;
	at_printf_return = retval;
}

/* Custom mock written for nrf_modem_at_printf since CMock can not be made to test parameters
 * of variadic functions.
 */
int nrf_modem_at_printf(const char *fmt, ...)
{
	va_list args;
	unsigned int trace_level;

	TEST_ASSERT_EQUAL_STRING(at_printf_fmt_expected, fmt);

	if (strcmp(at_printf_fmt_expected, trace_level_set_fmt) == 0) {
		va_start(args, fmt);
		trace_level = va_arg(args, unsigned int);
		va_end(args);

		TEST_ASSERT_EQUAL(at_printf_trace_level_expected, trace_level);
	}

	return at_printf_return;
}

int32_t nrf_modem_os_timedwait_stub(uint32_t context, int32_t *timeout, int cmock_num_calls)
{
	trace_backend_write_error = 0;

	return 0;
}

int nrf_modem_trace_get_stub(struct nrf_modem_trace_data **frags, size_t *n_frags, int timeout,
			     int cmock_num_calls)
{
	/* `cmock_num_calls` is reset to 0 at the beginning of each independent test execution. */
	int start_index = cmock_num_calls;
	int num_fragments = 0;
	struct nrf_modem_trace_data *frag;

	LOG_INF("trace_get_stub %d", cmock_num_calls);

	/* `cmock_num_calls` starts from 0 */
	nrf_modem_trace_get_cmock_num_calls = cmock_num_calls + 1;

	/* Block until we receive new data or `nrf_modem_trace_get` error. */
	while (k_fifo_is_empty(&get_fifo)) {
		if (nrf_modem_trace_get_error) {
			return nrf_modem_trace_get_error;
		}
	};

	/* Populate temporary static array to be used later on for returned data. */
	while (!k_fifo_is_empty(&get_fifo)) {
		frag = k_fifo_get(&get_fifo, K_FOREVER);
		memcpy(&read_frags[start_index + num_fragments],
		       frag,
		       sizeof(struct nrf_modem_trace_data));
		num_fragments++;
	}

	*frags = &read_frags[start_index];
	*n_frags = num_fragments;

	return 0;
}

int trace_backend_write_stub(const void *data, size_t len, int cmock_num_calls)
{
	/* `cmock_num_calls` starts from 0 */
	trace_backend_write_cmock_num_calls = cmock_num_calls + 1;

	if (trace_backend_write_error) {
		return trace_backend_write_error;
	}

	/* Associate local fragment pointer to a static fragment because later on
	 * we will call `k_fifo_alloc_put` which doesn't copy the item.
	 */
	struct nrf_modem_trace_data *frag = &write_frags[cmock_num_calls % MAX_N_STATIC_FRAGS];

	frag->data = data;
	frag->len = len;

	LOG_INF("data: %p, len: %d", data, len);

	k_fifo_alloc_put(&write_fifo, frag);

	return (int)len;
}

/* Function implementing a mechanism to synchronize main testing thread with trace thread via
 * a semaphore. This is the last function in the execution flow that can be mocked.
 */
int trace_backend_deinit_stub(int cmock_num_calls)
{
	k_sem_give(&backend_deinit_sem);

	return 0;
}

void test_trace_thread_handler_get_single(void)
{
	struct nrf_modem_trace_data header = { 0 };
	struct nrf_modem_trace_data data = { 0 };
	struct nrf_modem_trace_data *header_write;
	struct nrf_modem_trace_data *data_write;

	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	generate_trace_frag(&header);
	generate_trace_frag(&data);

	k_fifo_alloc_put(&get_fifo, &header);
	k_fifo_alloc_put(&get_fifo, &data);

	header_write = k_fifo_get(&write_fifo, K_FOREVER);
	data_write = k_fifo_get(&write_fifo, K_FOREVER);

	TEST_ASSERT_EQUAL_PTR(header.data, header_write->data);
	TEST_ASSERT_EQUAL_size_t(header.len, header_write->len);
	TEST_ASSERT_EQUAL_PTR(data.data, data_write->data);
	TEST_ASSERT_EQUAL_size_t(data.len, data_write->len);

	TEST_ASSERT_EQUAL(1, nrf_modem_trace_get_cmock_num_calls);
	TEST_ASSERT_EQUAL(2, trace_backend_write_cmock_num_calls);

	nrf_modem_trace_get_error = -ESHUTDOWN;

	wait_trace_deinit();
}

void test_trace_thread_handler_get_multi(void)
{
	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	for (int i = 0; i < TRACE_THREAD_HANDLER_MULTI_RUNS; i++) {
		struct nrf_modem_trace_data header = { 0 };
		struct nrf_modem_trace_data data = { 0 };
		struct nrf_modem_trace_data *header_write;
		struct nrf_modem_trace_data *data_write;

		generate_trace_frag(&header);
		generate_trace_frag(&data);

		k_fifo_alloc_put(&get_fifo, &header);
		k_fifo_alloc_put(&get_fifo, &data);

		header_write = k_fifo_get(&write_fifo, K_FOREVER);
		data_write = k_fifo_get(&write_fifo, K_FOREVER);

		TEST_ASSERT_EQUAL_PTR(header.data, header_write->data);
		TEST_ASSERT_EQUAL_size_t(header.len, header_write->len);
		TEST_ASSERT_EQUAL_PTR(data.data, data_write->data);
		TEST_ASSERT_EQUAL_size_t(data.len, data_write->len);
	}

	TEST_ASSERT_EQUAL(1 * TRACE_THREAD_HANDLER_MULTI_RUNS,
			  nrf_modem_trace_get_cmock_num_calls);
	TEST_ASSERT_EQUAL(2 * TRACE_THREAD_HANDLER_MULTI_RUNS,
			  trace_backend_write_cmock_num_calls);

	nrf_modem_trace_get_error = -ESHUTDOWN;

	wait_trace_deinit();
}

void test_trace_thread_handler_write_efault(void)
{
	struct nrf_modem_trace_data header = { 0 };
	struct nrf_modem_trace_data data = { 0 };

	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	generate_trace_frag(&header);
	generate_trace_frag(&data);

	trace_backend_write_error = -EFAULT;

	k_fifo_alloc_put(&get_fifo, &header);
	k_fifo_alloc_put(&get_fifo, &data);

	nrf_modem_trace_get_error = -ESHUTDOWN;

	wait_trace_deinit();

	TEST_ASSERT_EQUAL(1, trace_backend_write_cmock_num_calls);
}

void test_trace_thread_handler_get_einprogress(void)
{
	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	nrf_modem_trace_get_error = -EINPROGRESS;

	wait_trace_deinit();

	TEST_ASSERT_EQUAL(1, nrf_modem_trace_get_cmock_num_calls);
}

void test_trace_thread_handler_get_enodata(void)
{
	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	nrf_modem_trace_get_error = -ENODATA;

	wait_trace_deinit();

	TEST_ASSERT_EQUAL(1, nrf_modem_trace_get_cmock_num_calls);
}

void test_trace_thread_handler_get_eshutdown(void)
{
	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	nrf_modem_trace_get_error = -ESHUTDOWN;

	wait_trace_deinit();

	TEST_ASSERT_EQUAL(1, nrf_modem_trace_get_cmock_num_calls);
}

void test_nrf_modem_lib_trace_level_set(void)
{
	int ret;

	nrf_modem_at_printf_ExpectTraceLevelAndReturn(trace_level_set_fmt,
						      NRF_MODEM_LIB_TRACE_LEVEL_FULL,
						      0);

	ret = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_FULL);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_nrf_modem_lib_trace_level_set_efault(void)
{
	int ret;
	int wrong_level = 42;

	nrf_modem_at_printf_ExpectTraceLevelAndReturn(trace_level_set_fmt,
						      wrong_level,
						      -EFAULT);

	ret = nrf_modem_lib_trace_level_set(wrong_level);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_nrf_modem_lib_trace_enospc(void)
{
	int ret;
	struct nrf_modem_trace_data header = { 0 };
	struct nrf_modem_trace_data *header_write;

	__cmock_trace_backend_init_ExpectAndReturn(nrf_modem_trace_processed, 0);
	__cmock_nrf_modem_trace_get_Stub(nrf_modem_trace_get_stub);
	__cmock_trace_backend_write_Stub(trace_backend_write_stub);
	__cmock_trace_backend_deinit_Stub(trace_backend_deinit_stub);

	NRF_MODEM_LIB_ON_INIT_callback();

	trace_backend_write_error = -ENOSPC;

	generate_trace_frag(&header);

	k_fifo_alloc_put(&get_fifo, &header);

	ret = nrf_modem_lib_trace_processing_done_wait(K_FOREVER);
	TEST_ASSERT_EQUAL(-ENOSPC, ret);

	TEST_ASSERT_EQUAL(NRF_MODEM_LIB_TRACE_EVT_FULL, callback_evt);

	/* Clear space, make sure default handler is called */
	__cmock_trace_backend_clear_ExpectAndReturn(0);
	ret = nrf_modem_lib_trace_clear();
	TEST_ASSERT_EQUAL(0, ret);

	ret = nrf_modem_lib_trace_processing_done_wait(K_FOREVER);
	TEST_ASSERT_EQUAL(-ENOSPC, ret);

	TEST_ASSERT_EQUAL(NRF_MODEM_LIB_TRACE_EVT_FULL, callback_evt);

	/* Clear error and space, make sure we can continue to write. */
	trace_backend_write_error = 0;

	__cmock_trace_backend_clear_ExpectAndReturn(0);
	ret = nrf_modem_lib_trace_clear();
	TEST_ASSERT_EQUAL(0, ret);

	header_write = k_fifo_get(&write_fifo, K_FOREVER);
	TEST_ASSERT_EQUAL_PTR(header.data, header_write->data);
	TEST_ASSERT_EQUAL_size_t(header.len, header_write->len);
}

void test_nrf_modem_lib_trace_data_size(void)
{
	int ret;

	__cmock_trace_backend_data_size_ExpectAndReturn(1234);

	ret = nrf_modem_lib_trace_data_size();
	TEST_ASSERT_EQUAL(1234, ret);
}

void test_nrf_modem_lib_trace_enotsup(void)
{
	int ret;
	char buf[10];
	struct nrf_modem_lib_trace_backend trace_backend_orig;

	trace_backend_orig.read = trace_backend.read;
	trace_backend_orig.data_size = trace_backend.data_size;
	trace_backend_orig.clear = trace_backend.clear;

	trace_backend.read = NULL;
	trace_backend.data_size = NULL;
	trace_backend.clear = NULL;

	ret = nrf_modem_lib_trace_read(buf, 10);
	TEST_ASSERT_EQUAL(-ENOTSUP, ret);

	ret = nrf_modem_lib_trace_data_size();
	TEST_ASSERT_EQUAL(-ENOTSUP, ret);

	ret = nrf_modem_lib_trace_clear();
	TEST_ASSERT_EQUAL(-ENOTSUP, ret);

	trace_backend = trace_backend_orig;
}

void main(void)
{
	(void)unity_main();
}
