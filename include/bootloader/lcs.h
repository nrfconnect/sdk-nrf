/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LCS_H__
#define LCS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The PSA-compatible Life Cycle State (LCS) API.
 *
 * This file defines an API to read and update LCS.
 * It is independent of the underlying storage technology, yet it provides a
 * basic check of allowed state transitions.
 * The API is designed to be used by the bootloader, but it can also be used by
 * other components as well, as long as the underlying storage implementation is
 * available.
 *
 * The constants representing the LCS values are defined in such a way,
 * that a simple skip, uninitialized variable or a stack manipulation attack is
 * likely to result in an invalid LCS value, which can be detected by the calling API.
 */

enum lcs {
	/**
	 * This value can be used as an initial value of the LCS variable.
	 * It is invalid to call any of the APIs with this value as an argument,
	 * and it is not possible to transition into this state.
	 * In some cases, reading such value from underlying storage can be an
	 * indication of a tampering attempt.
	 */
	LCS_INVALID = 0x0,

	/**
	 * The value returned from the underlying storage cannot be mapped to a valid LCS.
	 * This can be an indication of a tampering attempt, or of a failure in
	 * the underlying storage.
	 * It is not possible to transition into this state.
	 * It may be possible to transition out of this state only through a
	 * factory reset procedure (if allowed).
	 */
	LCS_UNKNOWN = 0x60ebef49,

	/**
	 * The device is in the assembly and test phase.
	 * This is the default state of a fresh or erased device.
	 * In this state, the device is expected to receive the bootloader,
	 * manufacturing firmware and end-product update candidate.
	 * In this state the bootloader(s) are not able to perform a signature
	 * verification, but shall ensure the integrity of the next firmware in
	 * the chain.
	 * The manufacturing firmware is expected to"
	 *  - Lock down the first stage immutable bootloader.
	 *  - Provision the public keys used to verify the bootloader firmware.
	 *  - Provision the public keys used to verify the manufacturing firmware.
	 *  - (optionally) Provision the public keys used to verify the test and end-product
	 *    firmware.
	 *  - Disable the unauthenticated requests to erase the whole memory.
	 *  - Transition into the @p LCS_PSA_ROT_PROVISIONING state.
	 *
	 * At this stage, the manufacturing firmware should not assume that the device is soldered
	 * on the end-product.
	 *
	 * A device in this state is expected to be in a secured, trusted manufacturing environment.
	 * Transitioning into this state is possible only through an authenticated erase all
	 * procedure (if allowed).
	 */
	LCS_ASSEMBLY_AND_TEST = 0x5d44c48d,

	/**
	 * The device is in the PSA Root of Trust (RoT) provisioning phase.
	 * In this state, the device is expected to have its RoT keys provisioned.
	 * The bootloader(s) are expected to perform a signature verification of the firmware,
	 * but do not perform firmware updates.
	 *
	 * At this stage, the device is expected to be soldered on the end-product, but it is not
	 * expected to be in the hands of the end user.
	 * The device is expected to go through a securing phase, where both the
	 * hardware and software components are checked for integrity and completeness.
	 *
	 * The device may create a secure channel to provision additional keys,
	 * required to secure and run the main application.
	 * Since at this stage only a firmware, verified by the RoT keys is allowed to run,
	 * the device may be in an unsecured, untrusted environment.
	 *
	 * The manufacturing firmware must revoke the manufacturing keys and remove any other test
	 * assets before leaving this state.
	 * The device is expected to transition into the @p LCS_SECURED state before leaving the
	 * manufacturing environment.
	 * The manufacturing firmware is meant to be removed by applying the update candidate,
	 * containing the main application firmware.
	 * Transitioning into this state is possible only from the @p LCS_ASSEMBLY_AND_TEST.
	 * Transitioning out of this state may also be possible through an authenticated erase all
	 * procedure (if allowed).
	 */
	LCS_PSA_ROT_PROVISIONING = 0xafe19732,

	/**
	 * The device is secured, and in the deployed state.
	 * In this state, the device is expected to be in the hands of the end user,
	 * and to run the main application firmware, which is expected to be verified
	 * by the keys provisioned in the RoT provisioning phase.
	 * Transitioning into this state is possible only from the @p LCS_PSA_ROT_PROVISIONING.
	 * Transitioning out of this state is possible through a decommissioning procedure,
	 * as part of the product return process, or through an authenticated erase all
	 * procedure (if allowed).
	 * Leaving this state can be done only after removing all user data from the device,
	 * and it is expected to be irreversible.
	 */
	LCS_SECURED = 0x87bb60e7,

	/**
	 * The device is decommissioned, and in the discarded state.
	 * In this state, the device is expected to be discarded, and any-and-all access
	 * to the device is expected to be prohibited.
	 * In this state the device no longer performs regular operations.
	 * Transitioning into this state is possible from any valid state.
	 *
	 * This state can be used to secure a device after a tampering attempt.
	 * A manufacturer may allow to refurbish a device by transitioning it back to
	 * the @p LCS_ASSEMBLY_AND_TEST state through an authenticated erase all procedure.
	 */
	LCS_DECOMMISSIONED = 0x24152b9e,

	/**
	 * The maximum value of the enum, to ensure it is represented in 32 bits.
	 * This is not a valid state, and it is not possible to transition into this state.
	 */
	LCS_MAX = UINT32_MAX,
};

/**
 * @brief Query the current Life Cycle State value.
 *
 * @return The current Life Cycle State.
 */
enum lcs lcs_get(void);

/**
 * @brief Set the Life Cycle State value.
 *
 * @note This function intentionally does not return a status code,
 *       to prevent this from potentially being used to control the flow of
 *       subsequent code. Instead, explicitly query with @c lcs_get to ensure
 *       that the Life Cycle State was updated.
 *
 * @param[in] new_lcs  The new Life Cycle State.
 */
void lcs_set(enum lcs new_lcs);

#ifdef __cplusplus
}
#endif

#endif /* LCS_H__ */
