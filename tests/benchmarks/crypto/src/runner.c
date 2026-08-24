/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Runs a suite's operations, one thread per operation.
 *
 * All threading, timing and reporting lives here, so suite code carries no
 * instrumentation and a thread's cycles and stack are its operation's alone.
 */

#include "crypto_benchmarks.h"

#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>

LOG_MODULE_DECLARE(crypto_benchmarks);

/* Above the logging thread, so a deferred flush cannot land in a timed run. */
#define OP_THREAD_PRIO 1

#define MAX_RECORDS 512

/* Fits "alg/keydesc/stage/op" without paying for CONFIG_THREAD_MAX_NAME_LEN. */
#define REPORT_NAME_LEN 64

/* One stack for all operations, which run one at a time. Reuse does not blur
 * the figures: k_thread_create() re-poisons it via CONFIG_INIT_STACKS.
 */
static K_THREAD_STACK_DEFINE(op_stack, CONFIG_CRYPTO_BENCHMARKS_OP_STACK_SIZE);
static struct k_thread op_thread;

static K_SEM_DEFINE(op_completed, 0, 1);
static K_SEM_DEFINE(op_released, 0, 1);

static struct {
	const struct op *op;
	const void *context;
	psa_status_t status;
	uint64_t elapsed_ns;
} current;

/* Parts kept apart so the JSON report can emit them as separate fields. All are
 * string literals from the suite tables, which outlive the run.
 */
static struct {
	const char *group;
	const char *alg;
	const char *keydesc;
	const char *stage;
	const char *op;
	psa_status_t status;
	uint64_t elapsed_ns;
	size_t stack_used;
	bool stack_usage_valid;
} records[MAX_RECORDS];

static size_t record_cnt;
static size_t operation_cnt;
static size_t dropped_record_cnt;
static size_t failure_cnt;
static size_t skipped_cnt;

static const void *op_context(void)
{
	return current.op->context != NULL ? current.op->context : current.context;
}

static void prepare_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	current.status = current.op->prepare(op_context());
}

/* Own thread, so prepare's time falls outside the timed region and its stack
 * peak cannot be mistaken for the operation's.
 */
static psa_status_t run_prepare(const char *op_name)
{
	char name[CONFIG_THREAD_MAX_NAME_LEN];
	k_tid_t tid;
	int name_length;

	if (current.op->prepare == NULL) {
		return PSA_SUCCESS;
	}

	current.status = PSA_ERROR_GENERIC_ERROR;

	/* Named apart, so a fault in setup is not read as one in the measured op. */
	name_length = snprintk(name, sizeof(name), "%s/prepare", op_name);
	if (name_length < 0 || name_length >= (int)sizeof(name)) {
		LOG_WRN("prepare name truncated: %s", name);
	}

	tid = k_thread_create(&op_thread, op_stack, K_THREAD_STACK_SIZEOF(op_stack), prepare_entry,
			      NULL, NULL, NULL, OP_THREAD_PRIO, 0, K_FOREVER);
	(void)k_thread_name_set(tid, name);
	k_thread_start(tid);

	if (k_thread_join(tid, K_MSEC(CONFIG_CRYPTO_BENCHMARKS_OP_TIMEOUT_MS)) != 0) {
		LOG_ERR("%s: did not finish within %d ms", name, CONFIG_CRYPTO_BENCHMARKS_OP_TIMEOUT_MS);
		k_thread_abort(tid);
		(void)k_thread_join(tid, K_FOREVER);

		return PSA_ERROR_COMMUNICATION_FAILURE;
	}

	return current.status;
}

static void op_entry(void *p1, void *p2, void *p3)
{
	timing_t start;
	timing_t end;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	timing_start();

	start = timing_counter_get();
	current.status = current.op->fn(op_context());
	end = timing_counter_get();

	current.elapsed_ns = timing_cycles_to_ns(timing_cycles_get(&start, &end));

	timing_stop();

	/* Park rather than return, so the runner can read the stack while the
	 * thread is alive. These frames sit above the operation's own peak.
	 */
	k_sem_give(&op_completed);
	k_sem_take(&op_released, K_FOREVER);
}

static void record(const struct suite_group *group, const struct suite *suite, const char *stage,
		   const struct op *op, const char *name, psa_status_t status, uint64_t elapsed_ns,
		   size_t stack_used, bool stack_usage_valid)
{
	operation_cnt++;

	/* An unimplemented algorithm is skipped, not failed; the row carries the
	 * status either way.
	 */
	if (status == PSA_ERROR_NOT_SUPPORTED) {
		skipped_cnt++;
	} else if (status != PSA_SUCCESS) {
		failure_cnt++;
	}

	if (record_cnt >= ARRAY_SIZE(records)) {
		dropped_record_cnt++;
		LOG_WRN("operation record dropped: %s", name);
		LOG_WRN("You need to increase the records array size!");
		return;
	}

	records[record_cnt].group = group->name;
	records[record_cnt].alg = suite->alg;
	records[record_cnt].keydesc = suite->keydesc;
	records[record_cnt].stage = stage;
	records[record_cnt].op = op->name;
	records[record_cnt].status = status;
	records[record_cnt].elapsed_ns = elapsed_ns;
	records[record_cnt].stack_used = stack_used;
	records[record_cnt].stack_usage_valid = stack_usage_valid;
	record_cnt++;
}

static psa_status_t run_op(const struct suite_group *group, const struct suite *suite,
			   const char *stage, const struct op *op)
{
	char name[CONFIG_THREAD_MAX_NAME_LEN];
	k_tid_t tid;
	size_t stack_used = 0;
	bool stack_usage_valid = false;
	int name_length;

	if (suite->keydesc != NULL) {
		name_length = snprintk(name, sizeof(name), "%s/%s/%s/%s", suite->alg,
				       suite->keydesc, stage, op->name);
	} else {
		name_length = snprintk(name, sizeof(name), "%s/%s/%s", suite->alg, stage,
				       op->name);
	}
	if (name_length < 0 || name_length >= (int)sizeof(name)) {
		LOG_WRN("operation name truncated: %s", name);
	}

	/* Thread-unsafe by design: one operation runs at a time. */
	current.op = op;
	current.context = suite->context;
	current.status = PSA_ERROR_GENERIC_ERROR;
	current.elapsed_ns = 0;

	/* A prepare failure is recorded against the operation it was setting up. */
	psa_status_t prepared = run_prepare(name);
	if (prepared != PSA_SUCCESS) {
		LOG_WRN("%s: prepare did not succeed (status %d)", name, prepared);
		record(group, suite, stage, op, name, prepared, 0, 0, false);

		return prepared;
	}

	/* Prepare left PSA_SUCCESS here; reset so the timed run must set its own. */
	current.status = PSA_ERROR_GENERIC_ERROR;

	k_sem_reset(&op_completed);
	k_sem_reset(&op_released);

	/* Suspended, so the name is in place before the first instruction runs. */
	tid = k_thread_create(&op_thread, op_stack, K_THREAD_STACK_SIZEOF(op_stack), op_entry,
			      NULL, NULL, NULL, OP_THREAD_PRIO, 0, K_FOREVER);
	(void)k_thread_name_set(tid, name);
	k_thread_start(tid);

	/* An operation thread cannot log, so a wedged one must be timed out. */
	if (k_sem_take(&op_completed, K_MSEC(CONFIG_CRYPTO_BENCHMARKS_OP_TIMEOUT_MS)) != 0) {
		LOG_ERR("%s: did not finish within %d ms", name,
			CONFIG_CRYPTO_BENCHMARKS_OP_TIMEOUT_MS);
		k_thread_abort(tid);
		(void)k_thread_join(tid, K_FOREVER);
		record(group, suite, stage, op, name, PSA_ERROR_COMMUNICATION_FAILURE, 0, 0,
		       false);

		return PSA_ERROR_COMMUNICATION_FAILURE;
	}

	/* Still parked, so the peak is readable. */
	size_t unused_stack;
	if (k_thread_stack_space_get(tid, &unused_stack) == 0) {
		stack_used = K_THREAD_STACK_SIZEOF(op_stack) - unused_stack;
		stack_usage_valid = true;
	} else {
		LOG_WRN("%s: stack usage unavailable", name);
	}

	k_sem_give(&op_released);
	(void)k_thread_join(tid, K_FOREVER);

	record(group, suite, stage, op, name, current.status, current.elapsed_ns, stack_used,
	       stack_usage_valid);

	return current.status;
}

static psa_status_t run_stage(const struct suite_group *group, const struct suite *suite,
			      const char *stage, const struct stage *st)
{
	const char *keydesc = suite->keydesc != NULL ? suite->keydesc : "nokey";

	if (st->op_cnt == 0) {
		return PSA_SUCCESS;
	}

	for (size_t i = 0; i < st->op_cnt; i++) {
		psa_status_t status = run_op(group, suite, stage, &st->ops[i]);

		if (status != PSA_SUCCESS) {
			LOG_WRN("%s/%s: skipping the rest of the %s stage", suite->alg, keydesc,
				stage);
			return status;
		}
	}

	return PSA_SUCCESS;
}

void run_suite(const struct suite_group *group, const struct suite *suite)
{
	psa_status_t status;

	/* Only key setup gates the rest; the two stages are independent. */
	status = run_stage(group, suite, "keysetup", &suite->keysetup);
	if (status != PSA_SUCCESS) {
		goto cleanup_with_error;
	}

	status = run_stage(group, suite, "single", &suite->singlepart);
	if (status != PSA_SUCCESS) {
		goto cleanup_with_error;
	}

	status = run_stage(group, suite, "multi", &suite->multipart);
	if (status != PSA_SUCCESS) {
		goto cleanup_with_error;
	}

	if (suite->check != NULL && suite->check() != APP_SUCCESS) {
		goto cleanup_with_error;
	}

	goto cleanup;

cleanup_with_error:
	/* Skips were already counted per operation; only real errors count here. */
	if (status != PSA_ERROR_NOT_SUPPORTED) {
		failure_cnt++;
	}
cleanup:
	if (suite->cleanup != NULL) {
		suite->cleanup();
	}
}

size_t failure_count(void)
{
	return failure_cnt;
}

/* Split so neither formatter needs a 64-bit conversion. */
static void split_elapsed(size_t i, uint32_t *us, uint32_t *ns)
{
	*us = (uint32_t)(records[i].elapsed_ns / 1000U);
	*ns = (uint32_t)(records[i].elapsed_ns % 1000U);
}

#ifdef CONFIG_CRYPTO_BENCHMARKS_OUTPUT_JSON

/*
 * printk() rather than LOG, whose prefix would leave the document unparsable,
 * and streamed a record at a time because it outgrows any buffer here. Name
 * parts are the sample's own [a-z0-9_] literals, so none needs escaping.
 */
void report_summary(void)
{

	/* The counts come first, so that a truncated capture still yields them. */
	printk("{\"summary\":{\"operations\":%zu,\"skipped\":%zu,\"failed\":%zu,"
	       "\"dropped_records\":%zu},\n",
	       operation_cnt, skipped_cnt, failure_cnt, dropped_record_cnt);
	printk("\"operations\":[\n");

	for (size_t i = 0; i < record_cnt; i++) {
		uint32_t us;
		uint32_t ns;

		split_elapsed(i, &us, &ns);

		printk("%s{\"group\":\"%s\",\"alg\":\"%s\",", i == 0 ? "" : ",\n",
		       records[i].group, records[i].alg);

		/* Null, not a placeholder, so it does not read as a measurement. */
		if (records[i].keydesc != NULL) {
			printk("\"keydesc\":\"%s\",", records[i].keydesc);
		} else {
			printk("\"keydesc\":null,");
		}

		printk("\"stage\":\"%s\",\"op\":\"%s\",\"status\":%d,\"elapsed_us\":%u.%03u,",
		       records[i].stage, records[i].op, (int)records[i].status, us, ns);

		if (records[i].stack_usage_valid) {
			printk("\"stack_used\":%zu}", records[i].stack_used);
		} else {
			printk("\"stack_used\":null}");
		}
	}

	printk("\n]}\n");
}

#else /* CONFIG_CRYPTO_BENCHMARKS_OUTPUT_TEXT */

void report_summary(void)
{

	for (size_t i = 0; i < record_cnt; i++) {
		char name[REPORT_NAME_LEN];
		uint32_t us;
		uint32_t ns;
		const char *stack = records[i].stack_usage_valid ? "" : "unavailable ";

		split_elapsed(i, &us, &ns);

		if (records[i].keydesc != NULL) {
			(void)snprintk(name, sizeof(name), "%s/%s/%s/%s", records[i].alg,
				       records[i].keydesc, records[i].stage, records[i].op);
		} else {
			(void)snprintk(name, sizeof(name), "%s/%s/%s", records[i].alg,
				       records[i].stage, records[i].op);
		}

		LOG_INF("%-50s %7u.%03u us  (status %d, stack %s%zu bytes)", name, us,
			ns, records[i].status, stack, records[i].stack_used);
	}

	if (dropped_record_cnt != 0) {
		LOG_WRN("%zu operation records were not retained", dropped_record_cnt);
	}

	LOG_INF("=== summary: %zu operations, %zu skipped, %zu failed ===", operation_cnt,
			skipped_cnt, failure_cnt);
}

#endif /* CONFIG_CRYPTO_BENCHMARKS_OUTPUT_JSON */
