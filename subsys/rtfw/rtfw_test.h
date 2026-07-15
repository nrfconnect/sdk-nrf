/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_TEST_H_
#define RTFW_TEST_H_

#include <stdint.h>

typedef void (*rtfw_test_hook_t)(void);

void rtfw_test_tokens_set(uint32_t published, uint32_t acknowledged);
void rtfw_test_mailbox_reset(void);
void rtfw_test_event_queue_reset(void);
void rtfw_test_delivery_signal(void);
void rtfw_test_delivery_drain(void);
void rtfw_test_status_copy_hook_set(rtfw_test_hook_t hook);
void rtfw_test_delivery_release_hook_set(rtfw_test_hook_t hook);
void rtfw_test_delivery_rearm_hook_set(rtfw_test_hook_t hook);
void rtfw_test_doorbell_reset(void);
uint32_t rtfw_test_doorbell_count_get(void);
uint32_t rtfw_test_delivery_runs_get(void);

#endif /* RTFW_TEST_H_ */
