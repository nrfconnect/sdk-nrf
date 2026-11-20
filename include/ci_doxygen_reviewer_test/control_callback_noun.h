/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CONTROL_CALLBACK_NOUN_H_
#define CI_DOXYGEN_REVIEWER_TEST_CONTROL_CALLBACK_NOUN_H_

#include <stdint.h>

/**
 * @file control_callback_noun.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Notification handler for reviewer control events.
 *
 * @param[in] ctx Handler context registered by the application.
 * @param[in] event_id Identifier of the event being delivered.
 *
 * @retval 0 Event handled successfully.
 * @retval -EINVAL @p ctx was NULL or @p event_id was invalid.
 */
typedef int (*ci_doxy_reviewer_control_notify_handler_t)(void *ctx, uint32_t event_id);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CONTROL_CALLBACK_NOUN_H_ */
