/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EVENT_HANDLER_LIST_H__
#define EVENT_HANDLER_LIST_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Add the handler in the event handler list if not already present. */
int event_handler_list_handler_append(lte_lc_evt_handler_t handler);

/* Remove the handler from the event handler list if present. */
int event_handler_list_handler_remove(lte_lc_evt_handler_t handler);

/* Dispatch events for the registered event handlers. */
void event_handler_list_dispatch(const struct lte_lc_evt *const evt);

/* Test if the handler list is empty. */
bool event_handler_list_is_empty(void);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_HANDLER_LIST_H__ */
