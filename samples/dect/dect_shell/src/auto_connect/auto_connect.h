/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AUTO_CONNECT_H
#define AUTO_CONNECT_H

/* Use the default L4 event-driven trigger instead of a timed
 * schedule. Any other value overrides it.
 */
#define AUTO_CONNECT_DELAY_USE_DEFAULT_TRIGGER 0

int auto_connect_sett_enable_disable(bool enabled);
bool auto_connect_sett_is_enabled(void);
int auto_connect_sett_delay_set(int delay_secs);
int auto_connect_sett_delay_get(void);

#endif /* AUTO_CONNECT_H */
