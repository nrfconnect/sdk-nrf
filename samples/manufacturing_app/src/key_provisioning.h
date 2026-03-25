/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file key_provisioning.h
 *
 * Steps 4 and 12 — Provision and verify secure-boot / ADAC public keys,
 * and revoke the manufacturing application authentication key.
 */

#ifndef KEY_PROVISIONING_H_
#define KEY_PROVISIONING_H_

/**
 * @brief Provision the manufacturing application authentication key.
 *
 * Writes the mfg_app_pubkey (Ed25519) to KMU slot CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT
 * with a REVOKABLE persistence policy so it can be destroyed in Step 12 but
 * never reloaded after that.
 *
 * TODO: Remove this call once the bootloader chain (BL1/NSIB) is updated to
 * provision the manufacturing app key before handing over to this application.
 * In production the key must already be present in the KMU slot when main()
 * is entered; the manufacturing app should only revoke it, not provision it.
 *
 * Suspends execution on any error.
 */
void key_provision_mfg_app_key(void);

/**
 * @brief Step 4 — Provision and verify all secure-boot and ADAC keys.
 *
 * For each key defined in the keys/ directory:
 *   1. Validates PEM format and detects private keys.
 *   2. Warns if the key is the known placeholder from this sample.
 *   3. Checks whether the key is already in the KMU slot.
 *   4. If not provisioned, writes the key to KMU.
 *   5. Verifies the key by performing a signature check using a pre-built
 *      verification message from keys/key_verification_msgs/.
 *
 * Suspends execution on any error that cannot be recovered from.
 */
void step4_key_provision_all(void);

/**
 * @brief Step 12 — Revoke the manufacturing application authentication key.
 *
 * Destroys the key held in KMU slot CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT.
 * After this call the manufacturing application's signing key is revoked and
 * the device will reject any future manufacturing image signed with that key
 * (provided the slot used LIB_KMU_REV_POLICY_ROTATING or REVOKABLE policy).
 */
void step12_key_revoke_mfg_key(void);

#endif /* KEY_PROVISIONING_H_ */
