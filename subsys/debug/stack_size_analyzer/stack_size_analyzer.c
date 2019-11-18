/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Stack analyzer implementation
 */

#include <debug/stack_size_analyzer.h>
#include <debug/stack.h>
#include <kernel.h>
#include <logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(stack_size_analyzer, CONFIG_STACK_SIZE_ANALYZER_LOG_LEVEL);

#if IS_ENABLED(CONFIG_STACK_SIZE_ANALYZER_USE_PRINTK)
#define STACK_SIZE_ANALYZER_PRINT(...) printk(__VA_ARGS__)
#define STACK_SIZE_ANALYZER_FMT(str)   str "\n"
#define STASK_SIZE_ANALYZER_VSTR(str)  (str)
#else
#define STACK_SIZE_ANALYZER_PRINT(...) LOG_INF(__VA_ARGS__)
#define STACK_SIZE_ANALYZER_FMT(str)   str
#define STASK_SIZE_ANALYZER_VSTR(str)  log_strdup(str)
#endif

/**
 * @brief Maximum length of the pointer when converted to string
 *
 * Pointer is converted to string in hexadecimal form.
 * It would use 2 hex digits for every single byte of the pointer
 * but some implementations adds 0x prefix when used with %p format option.
 */
#define PTR_STR_MAXLEN (sizeof(void *) * 2 + 2)


/**
 * @brief Internal callback for default print
 *
 * @param name The name of the thread
 * @param size The total size of the stack
 * @param used The used size of the stack
 */
static void stack_print_cb(const char *name,
				    size_t size, size_t used)
{
	unsigned int pcnt = (used * 100U) / size;

	STACK_SIZE_ANALYZER_PRINT(
		STACK_SIZE_ANALYZER_FMT(
			" %-20s: unused %u usage %u / %u (%u %%)"),
		STASK_SIZE_ANALYZER_VSTR(name),
		size - used, used, size, pcnt);
}

static void stack_thread_cb(const struct k_thread *thread, void *user_data)
{
	stack_size_analyzer_cb cb = user_data;
	const char *stack = (const char *)thread->stack_info.start;
	unsigned int size = thread->stack_info.size;
	unsigned int unused = stack_unused_space_get(stack, size);
	const char *name;
	char hexname[PTR_STR_MAXLEN + 1];

	name = k_thread_name_get((k_tid_t)thread);
	if (!name || name[0] == '\0') {
		name = hexname;
		sprintf(hexname, "%p", (void *)thread);
	}

	cb(name, size, size-unused);
}

void stack_size_analyzer_run(stack_size_analyzer_cb cb)
{
	if (IS_ENABLED(CONFIG_STACK_SIZE_ANALYZER_RUN_UNLOCKED)) {
		k_thread_foreach_unlocked(stack_thread_cb, cb);
	} else {
		k_thread_foreach(stack_thread_cb, cb);
	}
}

void stack_size_analyzer_print(void)
{
	STACK_SIZE_ANALYZER_PRINT(STACK_SIZE_ANALYZER_FMT("Stack analyze:"));
	stack_size_analyzer_run(stack_print_cb);
}

#if IS_ENABLED(CONFIG_STACK_SIZE_ANALYZER_AUTO)

void stack_size_analyzer_auto(void)
{
	for (;;) {
		stack_size_analyzer_print();
		k_sleep(CONFIG_STACK_SIZE_ANALYZER_AUTO_PERIOD);
	}
}

K_THREAD_DEFINE(stack_size_analyzer,
		CONFIG_STACK_SIZE_ANALYZER_AUTO_STACK_SIZE,
		stack_size_analyzer_auto,
		NULL, NULL, NULL,
		CONFIG_NUM_PREEMPT_PRIORITIES - 1,
		0,
		K_NO_WAIT);

#endif
