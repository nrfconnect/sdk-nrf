/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_RING_H_
#define _FP_FMDN_RING_H_

#include <stdint.h>
#include <stddef.h>

#include <bluetooth/services/fast_pair/fmdn.h>

/**
 * @defgroup fp_fmdn_ring Fast Pair FMDN Ring module
 * @brief Internal API for Fast Pair FMDN Ring module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Encode the number of ringing components that are defined via Kconfig at the build time.
 *  The number is encoded as required by the Beacon Parameters Read request.
 *
 * @return Encoded number of ringing components (0-3).
 */
uint8_t fp_fmdn_ring_comp_num_encode(void);

/** Encode the ringing capabilities that are defined via Kconfig at the build time.
 *  The capabilities are encoded as required by the Beacon Parameters Read request.
 *
 * @return Encoded ringing capabilities.
 */
uint8_t fp_fmdn_ring_cap_encode(void);

/** Get the bitmask with the active ringing components as specified by the application.
 *  The bitmask is automatically cleared on the disable operation of the extension.
 *
 * @return Bitmask with the active ringing components (see @ref bt_fast_pair_fmdn_ring_comp).
 */
uint8_t fp_fmdn_ring_active_comp_bm_get(void);

/** Get the remaining timeout value for ongoing ringing action.
 *
 * @return Remaining timeout in deciseconds.
 */
uint16_t fp_fmdn_ring_timeout_get(void);

/** Check if the number of available ringing components matches the requested bitmask.
 *
 *  @param active_comp_bm Bitmask with the active ringing components.
 *
 * @return True  If number of available ringing components is greater than or equal to
 *               to the number of requested components in the target ringing bitmask.
 *         False Otherwise.
 */
bool fp_fmdn_ring_is_active_comp_bm_supported(uint8_t active_comp_bm);

/** Set the ringing state.
 *
 *  @param src   Source of the ringing activity that triggered state update.
 *  @param param Parameters of the ringing state.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_ring_state_param_set(
	enum bt_fast_pair_fmdn_ring_src src,
	const struct bt_fast_pair_fmdn_ring_state_param *param);

/** Handle the parameters of the ringing request.
 *
 *  @param src Source of the ringing activity.
 *  @param param Parameters of the ringing request.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_ring_req_handle(
	enum bt_fast_pair_fmdn_ring_src src,
	const struct bt_fast_pair_fmdn_ring_req_param *param);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_RING_H_ */
