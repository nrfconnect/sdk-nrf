/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if CONFIG_EVENT_MANAGER_PROXY
int init_event_proxy(char *proxy_subscribe_event);
#else
static inline int init_event_proxy(char *proxy_subscribe_event)
{
	return 0;
}
#endif
