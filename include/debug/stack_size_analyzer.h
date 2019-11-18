/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef __STACK_SIZE_ANALYZER_H
#define __STACK_SIZE_ANALYZER_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup stack_size_analyzer Stack analyzer
 * @brief Module for analyzing stack usage in samples
 *
 * This module implements functions and the configuration that simplifies
 * stack size analysis.
 * @{
 */

/**
 * @brief Stack size analyzer callback function
 *
 * Callback function with stack size statistics.
 *
 * @param name  The name of the thread or stringified address
 *              of the thread handle if name is not set.
 * @param size  The total size of the stack
 * @param used  Stack size in use
 */
typedef void (*stack_size_analyzer_cb)(const char *name,
				  size_t size, size_t used);

/**
 * @brief Run the stack size analyzer and return the results to the callback
 *
 * This function analyzes stacks for all threads and calls
 * a given callback on every thread found.
 *
 * @param cb The callback function handler
 */
void stack_size_analyzer_run(stack_size_analyzer_cb cb);

/**
 * @brief Run the stack size analyzer and print the result
 *
 * This function runs the stack size analyzer and prints the output in standard
 * form.
 */
void stack_size_analyzer_print(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __STACK_SIZE_ANALYZER_H */
