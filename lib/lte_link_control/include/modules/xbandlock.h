/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef XBANDLOCK_H__
#define XBANDLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Apply the runtime band lock, if set. The runtime band lock is lost when the modem is
 * powered off, so it must be applied again before the modem is activated.
 */
void xbandlock_runtime_lock_apply(void);

/* Clear the stored runtime band lock. */
void xbandlock_runtime_lock_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* XBANDLOCK_H__ */
