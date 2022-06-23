/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEST_EVENT_ALLOCATOR_H_
#define _TEST_EVENT_ALLOCATOR_H_

/**
 * @file
 * @defgroup em_test_event_allocator Event Manager test event allocator
 * @{
 * @brief Event Manager test event allocator.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Inform allocator if out of memory (OOM) error is expected.
 *
 * By default, the allocator asserts to ensure that allocation was successful.
 * When OOM error is expected, the assertion is skipped.
 *
 * @param[in] expected	Boolean informing if out of memory error is expected.
 */
void test_event_allocator_oom_expect(bool expected);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _TEST_EVENT_ALLOCATOR_H_ */
