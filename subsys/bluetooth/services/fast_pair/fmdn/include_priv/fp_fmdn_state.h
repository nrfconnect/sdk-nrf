/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_STATE_H_
#define _FP_FMDN_STATE_H_

#include <stdint.h>
#include <stddef.h>

#include <bluetooth/services/fast_pair/fmdn.h>

/**
 * @defgroup fp_fmdn_state Fast Pair FMDN state
 * @brief Internal API for Fast Pair FMDN state
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Length in bytes of the Ephemeral Identifier (EID). */
#define FP_FMDN_STATE_EID_LEN CONFIG_BT_FAST_PAIR_FMDN_ECC_LEN
/* Length in bytes of the Ephemeral Identity Key (EIK). */
#define FP_FMDN_STATE_EIK_LEN 32

/** Read the currently used Ephemeral Identifier.
 *
 *  Length of buffer used to store the Ephemeral Identifier must at least be equal
 *  to 20 bytes (for the SECP160R1 variant) or 32 bytes (for the SECP256R1 variant).
 *
 * @param[out] eid Ephemeral Identifier.

 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_state_eid_read(uint8_t *eid);

/** Read the currently provisioned Ephemeral Identity Key (EIK).
 *
 *  Length of buffer used to store the Ephemeral Identity Key (EIK) must at least
 *  be equal to 32 bytes.
 *
 * @param[out] eik Ephemeral Identity Key (EIK).
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_state_eik_read(uint8_t *eik);

/** Encode the Elliptic Curve type configuration.
 *  The configuration is encoded as required by the FMDN Accessory specification.
 *
 * @return Byte with an encoded information about the Elliptic Curve type.
 */
uint8_t fp_fmdn_state_ecc_type_encode(void);

/** Encode the TX power configuration in dBm.
 *  The TX power is encoded as required by the FMDN Accessory specification.
 *  The return value is a sum of TX power readout from the Bluetooth controller
 *  and the TX power correction value defined in Kconfig:
 *  CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL.
 *
 * @return Byte with information about the TX power, encoded as a signed integer.
 */
int8_t fp_fmdn_state_tx_power_encode(void);

/** Check if the beacon is provisioned with the Ephemeral Identity Key (EIK).
 *
 * @return True if the beacon is provisioned with the EIK, False Otherwise.
 */
bool fp_fmdn_state_is_provisioned(void);

/** Provision or unprovision the beacon with the Ephemeral Identity Key (EIK).
 *
 * @param[in] eik Ephemeral Identity Key (EIK).
 *                Non-NULL values are used to indicate provision operation
 *                and should point to the 32-byte buffer with the EIK.
 *                The NULL value is used to indicate unprovision operation.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_state_eik_provision(const uint8_t *eik);

/** Activate the Unwanted Tracking Protection (UTP) mode.
 *
 * @param[in] control_flags Control Flags.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_state_utp_mode_activate(uint8_t control_flags);

/** Deactivate the Unwanted Tracking Protection (UTP) mode.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_state_utp_mode_deactivate(void);

/** Check if the beacon should skip the authentication step for the ringing request.
 *
 * @return True if the ringing request shouldn't be authenticated, False Otherwise.
 */
bool fp_fmdn_state_utp_mode_ring_auth_skip(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_STATE_H_ */
