/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Layout of the side-loaded manufacturing key blob, programmed into the
 * mfg_keys partition (see the boards overlay) by scripts/pack_mfg_keys.py.
 * The firmware ELF carries no key material; the blob is read from flash
 * at runtime via flash_area_*.
 *
 * Source of truth for layout: this file. The Python packer mirrors it.
 *
 * TODO: authenticate the blob (HMAC over the contents with a per-batch
 *       key; today only magic+version+size header is checked).
 * TODO: ensure the bootloader does not include the mfg_keys partition in
 *       any signed/hashed firmware range.
 */

#ifndef MFG_KEY_BLOB_H_
#define MFG_KEY_BLOB_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MFG_KEY_BLOB_MAGIC   0x4B47464DU   /* 'M','F','G','K' little-endian */
#define MFG_KEY_BLOB_VERSION 1U

#define MFG_ED25519_PUBKEY_LEN 32U
#define MFG_IKG_SEED_LEN       48U
#define MFG_KEYRAM_RAND_LEN    16U
#define MFG_ED25519_SIG_LEN    64U
#define MFG_DIGEST_LEN         32U

struct __packed mfg_key_blob {
	uint32_t magic;
	uint16_t version;
	uint16_t reserved;
	uint32_t total_size;

	uint8_t  urot_pubkey_gen0[MFG_ED25519_PUBKEY_LEN];
	uint8_t  urot_pubkey_gen1[MFG_ED25519_PUBKEY_LEN];
	uint8_t  mfg_app_pubkey  [MFG_ED25519_PUBKEY_LEN];

	uint8_t  ikg_seed       [MFG_IKG_SEED_LEN];
	uint8_t  keyram_random0 [MFG_KEYRAM_RAND_LEN];
	uint8_t  keyram_random1 [MFG_KEYRAM_RAND_LEN];

	uint8_t  urot_pubkey_gen0_sig[MFG_ED25519_SIG_LEN];
	uint8_t  urot_pubkey_gen1_sig[MFG_ED25519_SIG_LEN];

	uint8_t  bl1_digest          [MFG_DIGEST_LEN];
	uint8_t  bl2_slot_a_digest   [MFG_DIGEST_LEN];
	uint8_t  bl2_slot_b_digest   [MFG_DIGEST_LEN];
	uint8_t  app_candidate_digest[MFG_DIGEST_LEN];
};

/* Cached pointer to a validated blob, or NULL if the partition has no valid
 * blob. Validation is magic + version + total_size == sizeof(struct).
 */
const struct mfg_key_blob *mfg_key_blob_get(void);

/* True if a buffer of MFG_DIGEST_LEN bytes is all zeros (placeholder). */
static inline bool mfg_digest_is_zero(const uint8_t *digest)
{
	for (size_t i = 0; i < MFG_DIGEST_LEN; i++) {
		if (digest[i] != 0U) {
			return false;
		}
	}
	return true;
}

#endif /* MFG_KEY_BLOB_H_ */
