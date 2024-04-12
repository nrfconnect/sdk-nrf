/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_CALLBACKS_H_
#define _FP_FMDN_CALLBACKS_H_

#include <stdint.h>
#include <stddef.h>

#include <bluetooth/services/fast_pair/fmdn.h>

/**
 * @defgroup fp_fmdn_callbacks Fast Pair FMDN Callbacks
 * @brief Internal API for Fast Pair FMDN Callbacks
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Notify the callback layer that the authenticated peer was notified
 *  about the accessory clock value.
 */
void fp_fmdn_callbacks_clock_synced_notify(void);

/** Notify the callback layer about the provisioning state changes.
 *
 *  @param provisioned true if the accessory has been successfully provisioned.
 *                     false if the accessory has been successfully unprovisioned.
 */
void fp_fmdn_callbacks_provisioning_state_changed_notify(bool provisioned);

/** Register the information callbacks internally. The internally registered
 *  callbacks have higher priority than the callbacks registered with the public
 *  Fast Pair API. You can call this function only in the disabled state of the
 *  of the FMDN module (see @ref bt_fast_pair_is_ready function). This API for
 *  callback registration is optional and does not have to be used. You can register
 *  multiple instances of information callbacks.
 *
 *  @param cb Information callback structure.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_callbacks_info_cb_register(struct bt_fast_pair_fmdn_info_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_CALLBACKS_H_ */
