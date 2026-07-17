/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_MULTI_USER_H_
#define _DULT_MULTI_USER_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>

#include <dult/dult.h>

/**
 * @defgroup dult_multi_user Detecting Unwanted Location Trackers - Multi-user coexistence
 * @brief DULT multi-user coexistence API.
 *
 * Lets several accessory-locating networks register with DULT at once during the
 * pre-association window.
 *
 * Used only with the v2 API (@kconfig{CONFIG_DULT_API_VARIANT_V2}).
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** DULT multi-user coexistence callback structure. */
struct dult_multi_user_cb {
	/** @brief Ownership arbitration notification.
	 *
	 *  Called for every registered user when a user calls @ref dult_enable
	 *  and wins the arbitration. The @p is_owner flag reports the outcome for
	 *  the user @p user refers to:
	 *  - true when @p user became the associated user (it won @ref dult_enable).
	 *  - false when @p user was evicted because another user won
	 *    @ref dult_enable.
	 *
	 *  On eviction the user remains registered until the application calls
	 *  @ref dult_user_unregister as part of its own teardown.
	 *
	 *  @p is_owner reflects the outcome of the @ref dult_enable this notification refers to.
	 *
	 *  @param[in] user     The user the notification refers to.
	 *  @param[in] is_owner true if @p user became the associated user, false if
	 *                      it was evicted.
	 */
	void (*ownership_claimed)(const struct dult_user *user, bool is_owner);

	/** @brief Ownership released notification.
	 *
	 *  Called for every registered user when the associated user releases the
	 *  association with @ref dult_reset while keeping the users registered.
	 *  DULT has no associated user until one of the registered users is enabled
	 *  again. Claiming the association at this point is the decision of the user
	 *  that owns the accessory state, not an automatic consequence of this
	 *  notification.
	 *
	 *  @p was_owner reflects @p user's role in the @ref dult_reset this notification refers to.
	 *
	 *  @param[in] user      The user the notification refers to.
	 *  @param[in] was_owner true if @p user was the associated user whose
	 *                       association just ended, false if @p user was a
	 *                       registered bystander that can now re-arbitrate.
	 */
	void (*ownership_released)(const struct dult_user *user, bool was_owner);
};

/** @brief Register the DULT multi-user coexistence callbacks for a user.
 *
 *  This function must be called after @ref dult_user_register. The callback structure must remain
 *  valid while the user is registered: it is preserved across @ref dult_reset and cleared by
 *  @ref dult_user_unregister.
 *
 *  The callbacks are invoked from the system workqueue, decoupled from the @ref dult_enable /
 *  @ref dult_reset call that caused the transition. Transitions are delivered in order, but when
 *  they are produced faster than the workqueue drains them (repeated @ref dult_enable /
 *  @ref dult_reset within a single execution context without yielding) cancelling transitions
 *  may be dropped, so intermediate states can be lost. Driving the DULT user lifecycle
 *  (registering, unregistering, enabling or resetting a user) from within the callback is
 *  allowed.
 *
 *  @param user User structure used to authenticate the user.
 *  @param cb   Multi-user callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_multi_user_cb_register(const struct dult_user *user,
				const struct dult_multi_user_cb *cb);

/** @brief Claim a Bluetooth connection for a DULT user.
 *
 *  Binds @p conn to @p user so that ANOS operations arriving on @p conn during
 *  the pre-association window are routed to @p user. The DULT user is expected
 *  to call this function directly from its Bluetooth connected callback for the
 *  connections it owns.
 *
 *  This is only relevant during the pre-association window; once @p user becomes the associated
 *  user, ANOS is already routed to it. A DULT user that does not rely on ANOS operations during
 *  the pre-association window does not need this API.
 *
 *  The claim is released automatically by DULT when @p conn is disconnected, so
 *  the DULT user does not need to release it explicitly.
 *
 *  @param user User structure used to authenticate the user.
 *  @param conn Bluetooth connection to claim.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_multi_user_conn_claim(const struct dult_user *user, struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_MULTI_USER_H_ */
