/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Steps 4 and 12: Key provisioning and revocation.
 *
 * Key provisioning on privileged cpuapp:
 *   Keys are imported into KMU slots via psa_import_key() with a key ID
 *   constructed by PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(scheme, slot).
 *   The CRACEN PSA driver executes the actual KMU write directly on the
 *   privileged cpuapp.
 *
 *   Key policy documentation: options for each attribute:
 *
 *   Usage scheme (passed as the first argument to PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT):
 *     CRACEN_KMU_KEY_USAGE_SCHEME_RAW       — not encrypted; pushed to CPU-accessible
 *                                             kmu_push_area for usage.
 *     CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED — pushed to CRACEN's protected RAM only.
 *                                             Only AES is supported in this mode.
 *     CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED — stored encrypted, decrypted to push area.
 *     CRACEN_KMU_KEY_USAGE_SCHEME_SEED      — IKG seed, pushed to CRACEN seed register.
 *
 *   Revocation policy (set via PSA key persistence):
 *     CRACEN_KEY_PERSISTENCE_REVOKABLE (0x02) — once deleted, the slot cannot be
 *                                               re-provisioned. Transition to 'Erased'.
 *     CRACEN_KEY_PERSISTENCE_READ_ONLY  (0x03) — cannot be erased except by ERASEALL.
 *     PSA_KEY_PERSISTENCE_DEFAULT             — key persists across resets; slot can be
 *                                               reused after deletion (ROTATING policy).
 *
 * Detecting a revoked (appears-empty) slot:
 *   A KMU slot that previously held a REVOKABLE key appears empty after revocation
 *   but psa_import_key() will fail without a clear error code. The application
 *   cannot distinguish this case from a hardware fault. This is a known hardware
 *   limitation. The log message "Possible reason — a key was provisioned and
 *   revoked from given KMU slot" is the best achievable diagnostic.
 */

#include "key_provisioning.h"
#include "mfg_key_blob.h"
#include "mfg_log.h"
#include "recovery.h"

#include <zephyr/kernel.h>
#include <psa/crypto.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>
#include <cracen/cracen_kmu.h>
#include <cracen/lib_kmu.h>

#include <string.h>

/* ---------------------------------------------------------------------------
 * Known verification message (must match the message used when the
 * key_verification_msgs/<name>_signed.msg files were signed).
 *
 * TODO: Decide on the canonical message content and update this constant
 *       together with the signature files. See open question in README.rst.
 * ---------------------------------------------------------------------------
 */
#define MFG_KEY_VERIFY_MESSAGE     "manufacturing_key_verification_v1"
#define MFG_KEY_VERIFY_MESSAGE_LEN (sizeof(MFG_KEY_VERIFY_MESSAGE) - 1U)

/* ---------------------------------------------------------------------------
 * Signature length for Ed25519 (64 bytes).
 * ---------------------------------------------------------------------------
 */
#define ED25519_SIGNATURE_LEN 64U

/* ---------------------------------------------------------------------------
 * Mirror of the private kmu_metadata bitfield struct from cracen_psa_kmu.c.
 *
 * This layout describes the 32-bit word stored in each KMU slot's METADATA
 * register.  It is stable because it is written to hardware OTP-equivalent
 * storage and must not change between NCS releases without a migration path.
 *
 * Source: nrf/subsys/nrf_security/src/drivers/cracen/cracenpsa/src/cracen_psa_kmu.c
 * ---------------------------------------------------------------------------
 */
typedef struct {
	uint32_t metadata_version: 4;
	uint32_t key_usage_scheme: 2;
	uint32_t reserved:         10;
	uint32_t algorithm:        4;
	uint32_t size:             3;
	uint32_t rpolicy:          2;
	uint32_t usage_flags:      7;
} kmu_slot_meta_t;

/* Sentinel value written to secondary slots of multi-slot keys
 * (e.g. slot N+1 for a 2-slot Ed25519 key occupying slots N and N+1).
 * Source: cracen_psa_kmu.c SECONDARY_SLOT_METADATA_VALUE */
#define KMU_SECONDARY_SLOT_METADATA UINT32_MAX

static const char *kmu_alg_name(uint8_t alg)
{
	static const char *const names[] = {
		[0]  = "??",
		[1]  = "METADATA_ALG_CHACHA20",
		[2]  = "METADATA_ALG_CHACHA20_POLY1305",
		[3]  = "METADATA_ALG_AES_GCM",
		[4]  = "METADATA_ALG_AES_CCM",
		[5]  = "METADATA_ALG_AES_ECB",
		[6]  = "METADATA_ALG_AES_CTR",
		[7]  = "METADATA_ALG_AES_CBC",
		[8]  = "METADATA_ALG_SP800_108_COUNTER_CMAC",
		[9]  = "METADATA_ALG_CMAC",
		[10] = "METADATA_ALG_ED25519",
		[11] = "METADATA_ALG_ECDSA",
		[12] = "METADATA_ALG_ED25519PH",
		[13] = "METADATA_ALG_HMAC",
		[14] = "METADATA_ALG_ECDH",
		[15] = "??",
	};
	return (alg < ARRAY_SIZE(names)) ? names[alg] : "??";
}

static const char *kmu_size_name(uint8_t sz)
{
	static const char *const names[] = {
		[0] = "??",
		[1] = "METADATA_ALG_KEY_BITS_128",
		[2] = "METADATA_ALG_KEY_BITS_192",
		[3] = "METADATA_ALG_KEY_BITS_255",
		[4] = "METADATA_ALG_KEY_BITS_256",
		[5] = "METADATA_ALG_KEY_BITS_384_SEED",
		[6] = "METADATA_ALG_KEY_BITS_384",
		[7] = "??",
	};
	return (sz < ARRAY_SIZE(names)) ? names[sz] : "??";
}

static const char *kmu_scheme_name(uint8_t scheme)
{
	switch (scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED: return "CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED";
	case CRACEN_KMU_KEY_USAGE_SCHEME_SEED:      return "CRACEN_KMU_KEY_USAGE_SCHEME_SEED";
	case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED: return "CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED";
	case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:       return "CRACEN_KMU_KEY_USAGE_SCHEME_RAW";
	default:                                    return "??";
	}
}

static const char *kmu_rpolicy_name(uint8_t rpolicy)
{
	switch (rpolicy) {
	case LIB_KMU_REV_POLICY_RESERVED: return "LIB_KMU_REV_POLICY_RESERVED";
	case LIB_KMU_REV_POLICY_ROTATING: return "LIB_KMU_REV_POLICY_ROTATING";
	case LIB_KMU_REV_POLICY_LOCKED:   return "LIB_KMU_REV_POLICY_LOCKED";
	case LIB_KMU_REV_POLICY_REVOKED:  return "LIB_KMU_REV_POLICY_REVOKED";
	default:                          return "??";
	}
}

/* ---------------------------------------------------------------------------
 * UROT public keys.
 *   Each Ed25519 key occupies 2 consecutive KMU slots (255-bit key = 2 × 16-byte
 *   KMU slots), so gen0 occupies slots 120–121 and gen1 occupies 124–125.
 *   key_offset / sig_offset are offsets into struct mfg_key_blob.
 * ---------------------------------------------------------------------------
 */
struct urot_key_entry {
	const char *name;
	int         slot_id;
	size_t      key_offset;
	size_t      sig_offset;
};

static const struct urot_key_entry urot_key_entries[] = {
	{ "urot_pubkey_gen0", CONFIG_SAMPLE_MFG_UROT_KEY_GEN0_KMU_SLOT,
	  offsetof(struct mfg_key_blob, urot_pubkey_gen0),
	  offsetof(struct mfg_key_blob, urot_pubkey_gen0_sig) },
	{ "urot_pubkey_gen1", CONFIG_SAMPLE_MFG_UROT_KEY_GEN1_KMU_SLOT,
	  offsetof(struct mfg_key_blob, urot_pubkey_gen1),
	  offsetof(struct mfg_key_blob, urot_pubkey_gen1_sig) },
};

static const uint8_t *blob_field_ptr(const struct mfg_key_blob *blob, size_t off)
{
	return ((const uint8_t *)blob) + off;
}

static bool blob_bytes_all_zero(const uint8_t *bytes, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (bytes[i] != 0U) {
			return false;
		}
	}
	return true;
}

/* ---------------------------------------------------------------------------
 * Check whether a KMU slot is already occupied.
 * Returns true if a key is present, false if the slot is empty (or the key
 * cannot be queried).
 * ---------------------------------------------------------------------------
 */
static bool slot_is_occupied(int slot_id)
{
	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, slot_id);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = psa_get_key_attributes(key_id, &attr);

	psa_reset_key_attributes(&attr);
	return status == PSA_SUCCESS;
}

/* ---------------------------------------------------------------------------
 * Check expected attributes for a KMU-stored UROT Ed25519 public key.
 *
 * Reads both the raw KMU metadata word and PSA key attributes and cross-checks
 * them against the expected values for a UROT key (METADATA_ALG_ED25519,
 * CRACEN_KMU_KEY_USAGE_SCHEME_RAW, LIB_KMU_REV_POLICY_ROTATING,
 * PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE).
 *
 * Returns true if all attributes match, false (with error log) on mismatch.
 * ---------------------------------------------------------------------------
 */
static bool check_key_attributes(int slot_id, const char *key_name)
{
	/* Expected values for a UROT Ed25519 public key */
	const uint8_t exp_alg    = 10; /* METADATA_ALG_ED25519 */
	const uint8_t exp_scheme = CRACEN_KMU_KEY_USAGE_SCHEME_RAW;
	const uint8_t exp_rpol   = LIB_KMU_REV_POLICY_ROTATING;
	const psa_key_usage_t exp_usage =
		PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE;

	/* --- Read raw KMU metadata --- */
	uint32_t raw = 0;

	if (lib_kmu_read_metadata(slot_id, &raw) != LIB_KMU_SUCCESS) {
		MFG_LOG_ERR("%s already provisioned, but cannot read KMU metadata.\n",
			    key_name);
		return false;
	}

	kmu_slot_meta_t meta;

	memcpy(&meta, &raw, sizeof(meta));

	/* --- Read PSA attributes using the scheme stored in the metadata --- */
	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(meta.key_usage_scheme, slot_id);
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = psa_get_key_attributes(key_id, &attr);

	if (status != PSA_SUCCESS) {
		MFG_LOG_ERR("%s already provisioned, but cannot read PSA attributes "
			    "(status=%d).\n", key_name, status);
		psa_reset_key_attributes(&attr);
		return false;
	}

	psa_key_usage_t actual_usage = psa_get_key_usage_flags(&attr);

	psa_reset_key_attributes(&attr);

	bool attributes_match = (meta.algorithm == exp_alg)
		  && (meta.key_usage_scheme == exp_scheme)
		  && (meta.rpolicy == exp_rpol)
		  && (actual_usage == exp_usage);

	if (!attributes_match) {
		MFG_LOG_ERR("%s already provisioned, but its attributes are incorrect\n",
			    key_name);

		if (meta.algorithm != exp_alg) {
			MFG_LOG_ERR("  Expected: METADATA_ALG_ED25519,         as-is: %s\n",
				    kmu_alg_name(meta.algorithm));
		}
		if (meta.rpolicy != exp_rpol) {
			MFG_LOG_ERR("  Expected: LIB_KMU_REV_POLICY_ROTATING,  as-is: %s\n",
				    kmu_rpolicy_name(meta.rpolicy));
		}
		if (meta.key_usage_scheme != exp_scheme) {
			MFG_LOG_ERR("  Expected: CRACEN_KMU_KEY_USAGE_SCHEME_RAW, as-is: %s\n",
				    kmu_scheme_name(meta.key_usage_scheme));
		}
		if (actual_usage != exp_usage) {
			MFG_LOG_ERR("  Expected: PSA_KEY_USAGE_VERIFY_HASH | "
				    "PSA_KEY_USAGE_VERIFY_MESSAGE (0x%08x), "
				    "as-is: 0x%08x\n",
				    (unsigned)exp_usage, (unsigned)actual_usage);
		}
	}

	return attributes_match;
}

/* ---------------------------------------------------------------------------
 * Provision a single Ed25519 public key to a KMU slot.
 * ---------------------------------------------------------------------------
 */
static bool provision_key(const char *key_name, int slot_id,
			  const uint8_t *key_data, size_t key_len)
{
	MFG_LOG_INF("Provisioning %s to KMU, slot %d...", key_name, slot_id);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, slot_id);

	psa_set_key_id(&attr, key_id);

	/*
	 * Lifetime: persistent in CRACEN KMU with REVOKABLE persistence so that
	 * once the slot is destroyed it cannot be re-provisioned with a different
	 * (potentially compromised) key.
	 *
	 * Other available policies:
	 *   PSA_KEY_PERSISTENCE_DEFAULT          — rotating (slot reusable after deletion)
	 *   CRACEN_KEY_PERSISTENCE_READ_ONLY     — erasable only by ERASEALL
	 */
	psa_set_key_lifetime(&attr,
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			CRACEN_KEY_PERSISTENCE_REVOKABLE,
			PSA_KEY_LOCATION_CRACEN_KMU));

	psa_set_key_type(&attr,
		PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_bits(&attr, 255);
	psa_set_key_usage_flags(&attr,
		PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_PURE_EDDSA);

	psa_key_id_t imported_key_id = PSA_KEY_ID_NULL;
	psa_status_t status = psa_import_key(&attr, key_data, key_len, &imported_key_id);

	psa_reset_key_attributes(&attr);

	if (status == PSA_SUCCESS) {
		MFG_LOG_INF(" OK\n");
		return true;
	}

	MFG_LOG_INF(" FAIL\n");
	MFG_LOG_ERR("The cause is unknown. Possible reason — a key was provisioned\n");
	MFG_LOG_ERR("and revoked from given KMU slot.\n");
	return false;
}

/* ---------------------------------------------------------------------------
 * Verify a KMU-stored key by performing a signature check.
 *
 * The verification message is the fixed string MFG_KEY_VERIFY_MESSAGE.
 * The expected signature is read from the mfg_urot_keys table (msg file).
 *
 * The signature file must be a raw 64-byte Ed25519 signature of the
 * verification message, computed with the PRIVATE key counterpart.
 * ---------------------------------------------------------------------------
 */
static bool verify_key_signature(const char *key_name, int slot_id,
				 const uint8_t *sig, size_t sig_len)
{
	MFG_LOG_INF("Performing signature check using %s (KMU stored)...", key_name);

	if (sig == NULL || sig_len != ED25519_SIGNATURE_LEN) {
		MFG_LOG_INF(" SKIP (no verification message file found for %s)\n", key_name);
		MFG_LOG_ERR("Place a signed message file at "
			    "keys/key_verification_msgs/%s_signed.msg\n", key_name);
		return false;
	}

	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, slot_id);

	psa_status_t status = psa_verify_message(
		key_id,
		PSA_ALG_PURE_EDDSA,
		(const uint8_t *)MFG_KEY_VERIFY_MESSAGE, MFG_KEY_VERIFY_MESSAGE_LEN,
		sig, sig_len);

	if (status == PSA_SUCCESS) {
		MFG_LOG_INF(" OK\n");
		return true;
	}

	MFG_LOG_INF(" FAIL\n");
	MFG_LOG_ERR("Perhaps a verification message file was signed by a private key\n");
	MFG_LOG_ERR("from other key pair.\n");
	MFG_LOG_ERR("Generate a new key pair and/or sign again message verification file\n");
	MFG_LOG_ERR("and build this app again.\n");
	return false;
}

/* ---------------------------------------------------------------------------
 * Provision the manufacturing application authentication key.
 *
 * TODO: Remove once BL1/NSIB provisions this key before the manufacturing app
 *       runs. In a full bootloader chain the key is already in the KMU slot
 *       when main() is entered; the manufacturing app should only revoke it
 *       in Step 12.
 * ---------------------------------------------------------------------------
 */
static bool provision_mfg_app_key_impl(void)
{
	const struct mfg_key_blob *blob = mfg_key_blob_get();

	if (blob == NULL) {
		MFG_LOG_ERR("mfg_keys blob unavailable — cannot provision mfg_app_pubkey.\n");
		return false;
	}

	if (blob_bytes_all_zero(blob->mfg_app_pubkey, sizeof(blob->mfg_app_pubkey))) {
		MFG_LOG_ERR("mfg_app_pubkey is all-zeros in mfg_keys blob — cannot provision.\n");
		return false;
	}

	int slot = CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT;

	if (slot_is_occupied(slot)) {
		MFG_LOG_INF("mfg_app_pubkey already provisioned (KMU slot %d).\n", slot);
		return true;
	}

	MFG_LOG_INF("Provisioning mfg_app_pubkey to KMU slot %d...", slot);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, slot);

	psa_set_key_id(&attr, key_id);
	/* REVOKABLE persistence: Step 12 destroys this key and the slot must
	 * not be re-provisionable afterwards.
	 */
	psa_set_key_lifetime(&attr,
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			CRACEN_KEY_PERSISTENCE_REVOKABLE,
			PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_type(&attr,
		PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_bits(&attr, 255);
	psa_set_key_usage_flags(&attr,
		PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_PURE_EDDSA);

	psa_key_id_t imported_id = PSA_KEY_ID_NULL;
	psa_status_t status = psa_import_key(&attr,
					     blob->mfg_app_pubkey,
					     sizeof(blob->mfg_app_pubkey),
					     &imported_id);

	psa_reset_key_attributes(&attr);

	if (status == PSA_SUCCESS) {
		MFG_LOG_INF(" OK\n");
		return true;
	}

	MFG_LOG_INF(" FAIL (psa_import_key returned %d)\n", status);
	return false;
}

void key_provision_mfg_app_key(void)
{
	if (!provision_mfg_app_key_impl()) {
		recovery_suspend(false);
	}
}

/* ---------------------------------------------------------------------------
 * Print a one-line usage flag list for a PSA key_usage_t value.
 * Outputs each set flag as a PSA_KEY_USAGE_* name followed by a comma.
 * ---------------------------------------------------------------------------
 */
static void log_usage_flags(psa_key_usage_t usage)
{
	if (usage & PSA_KEY_USAGE_ENCRYPT) {
		MFG_LOG_INF(" PSA_KEY_USAGE_ENCRYPT,");
	}
	if (usage & PSA_KEY_USAGE_DECRYPT) {
		MFG_LOG_INF(" PSA_KEY_USAGE_DECRYPT,");
	}
	if (usage & PSA_KEY_USAGE_SIGN_MESSAGE) {
		MFG_LOG_INF(" PSA_KEY_USAGE_SIGN_MESSAGE,");
	}
	if (usage & PSA_KEY_USAGE_VERIFY_MESSAGE) {
		MFG_LOG_INF(" PSA_KEY_USAGE_VERIFY_MESSAGE,");
	}
	if (usage & PSA_KEY_USAGE_SIGN_HASH) {
		MFG_LOG_INF(" PSA_KEY_USAGE_SIGN_HASH,");
	}
	if (usage & PSA_KEY_USAGE_VERIFY_HASH) {
		MFG_LOG_INF(" PSA_KEY_USAGE_VERIFY_HASH,");
	}
	if (usage & PSA_KEY_USAGE_DERIVE) {
		MFG_LOG_INF(" PSA_KEY_USAGE_DERIVE,");
	}
	if (usage & PSA_KEY_USAGE_EXPORT) {
		MFG_LOG_INF(" PSA_KEY_USAGE_EXPORT,");
	}
	if (usage == 0) {
		MFG_LOG_INF(" (none)");
	}
}

/* ---------------------------------------------------------------------------
 * Print the KMU metadata for a single slot.
 * Counts consecutive secondary slots to report num_slots.
 * ---------------------------------------------------------------------------
 */
static void kmu_print_slot(int slot)
{
	uint32_t raw = 0;

	if (lib_kmu_read_metadata(slot, &raw) != LIB_KMU_SUCCESS) {
		MFG_LOG_INF("  Slot %d: cannot read metadata\n", slot);
		return;
	}

	if (raw == KMU_SECONDARY_SLOT_METADATA) {
		MFG_LOG_INF("  Slot %d: secondary slot (part of previous key)\n", slot);
		return;
	}

	kmu_slot_meta_t meta;

	memcpy(&meta, &raw, sizeof(meta));

	/*
	 * Derive the number of KMU slots occupied by this key from the size field.
	 * Each KMU slot holds 128 bits; keys wider than that spill into consecutive
	 * slots.  The mapping mirrors the CRACEN driver (cracen_psa_kmu.c):
	 *   128  bits → 1 slot
	 *   192 / 255 / 256 bits → 2 slots*
	 *   384-seed → 3 slots
	 *   384 bits → 2 slots
	 *
	 * * Simplification: P-256 public key uses 4, but that key type is not
	 *   provisioned by this sample.
	 */
	static const uint8_t size_to_slots[] = {
		[0] = 1, /* unknown — assume 1 */
		[METADATA_ALG_KEY_BITS_128]      = 1,
		[METADATA_ALG_KEY_BITS_192]      = 2,
		[METADATA_ALG_KEY_BITS_255]      = 2,
		[METADATA_ALG_KEY_BITS_256]      = 2,
		[METADATA_ALG_KEY_BITS_384_SEED] = 3,
		[METADATA_ALG_KEY_BITS_384]      = 2,
		[7]                              = 1, /* reserved */
	};
	int num_slots = (meta.size < ARRAY_SIZE(size_to_slots)) ? size_to_slots[meta.size] : 1;

	/*
	 * The PSA key handle encodes both the usage scheme and the slot index:
	 *   PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(scheme, slot)
	 *     = 0x7FFF0000 | (scheme << 12) | slot
	 * We read the scheme from the hardware metadata so the printed key_id
	 * is the one that PSA operations must use for this slot.
	 */
	psa_key_id_t key_id =
		PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(meta.key_usage_scheme, slot);

	MFG_LOG_INF("\nSlot idx: %d (%d), PSA key_id: 0x%08X, raw metadata: %08X\n",
		    slot, num_slots, (unsigned)key_id, (unsigned)raw);

	MFG_LOG_INF("  Size:  %s\n", kmu_size_name(meta.size));

	/* Usage scheme, including hardware-level description */
	MFG_LOG_INF("  Key usage scheme: %s\n", kmu_scheme_name(meta.key_usage_scheme));
	switch (meta.key_usage_scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
		MFG_LOG_INF("    These keys can only be pushed to CRACEN's protected RAM.\n");
		MFG_LOG_INF("    The keys are not encrypted. Only AES supported.\n");
		break;
	case CRACEN_KMU_KEY_USAGE_SCHEME_SEED:
		MFG_LOG_INF("    Pushed to the IKG seed register.\n");
		MFG_LOG_INF("    Uses three consecutive KMU slots (384 bits total).\n");
		break;
	case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED:
		MFG_LOG_INF("    Keys are stored in encrypted form.\n");
		MFG_LOG_INF("    They will be decrypted to the KMU push area for usage.\n");
		break;
	case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:
		MFG_LOG_INF("    These keys are not encrypted.\n");
		MFG_LOG_INF(
			"    They will be pushed to MCU-accessible kmu_push_area for usage.\n");
		break;
	}

	MFG_LOG_INF("  Algorithm:   %s\n", kmu_alg_name(meta.algorithm));

	/* PSA usage flags: obtained from the driver rather than decoding the
	 * 7-bit usage_flags field directly, since the PSA API is the inteded
	 * interface for key operations. */
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	if (psa_get_key_attributes(key_id, &attr) == PSA_SUCCESS) {
		MFG_LOG_INF("  Usage:");
		log_usage_flags(psa_get_key_usage_flags(&attr));
		MFG_LOG_INF(" \n");
		psa_reset_key_attributes(&attr);
	} else {
		MFG_LOG_INF("  Usage: (unavailable via PSA for this slot)\n");
	}

	/* Revocation policy including hardware-level description */
	MFG_LOG_INF("  Revocation policy: %s\n", kmu_rpolicy_name(meta.rpolicy));
	switch (meta.rpolicy) {
	case LIB_KMU_REV_POLICY_ROTATING:
		MFG_LOG_INF("    Key Slot can be reused. Revocation transits it to 'Erased'\n");
		break;
	case LIB_KMU_REV_POLICY_LOCKED:
		MFG_LOG_INF("    Key is locked. Cannot be erased except by ERASEALL.\n");
		break;
	case LIB_KMU_REV_POLICY_REVOKED:
		MFG_LOG_INF("    Key has been revoked.\n");
		break;
	default:
		break;
	}
}

/* ---------------------------------------------------------------------------
 * KMU slot summary — called at the end of Step 4.
 * ---------------------------------------------------------------------------
 */
static void kmu_print_slot_summary(void)
{
	static const struct {
		const char *label;
		int         slot_id;
	} provisioned_slots[] = {
		{ "urot_pubkey_gen0", CONFIG_SAMPLE_MFG_UROT_KEY_GEN0_KMU_SLOT },
		{ "urot_pubkey_gen1", CONFIG_SAMPLE_MFG_UROT_KEY_GEN1_KMU_SLOT },
		{ "mfg_app_pubkey",   CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT       },
	};

	for (size_t i = 0; i < ARRAY_SIZE(provisioned_slots); i++) {
		int slot = provisioned_slots[i].slot_id;

		MFG_LOG_INF("%s (slot %d):", provisioned_slots[i].label, slot);

		if (lib_kmu_is_slot_empty(slot)) {
			MFG_LOG_INF(" empty\n");
			continue;
		}

		kmu_print_slot(slot);
	}
}

/* ---------------------------------------------------------------------------
 * Step 4 — Validate and provision UROT public keys from the mfg_keys blob.
 * ---------------------------------------------------------------------------
 */
void step4_key_provision_all(void)
{
	MFG_LOG_STEP("Validating secure boot and ADAC keys");

	const struct mfg_key_blob *blob = mfg_key_blob_get();

	if (blob == NULL) {
		MFG_LOG_ERR("mfg_keys blob unavailable — cannot provision UROT keys.\n");
		recovery_suspend(false);
	}

	for (size_t i = 0; i < ARRAY_SIZE(urot_key_entries); i++) {
		const struct urot_key_entry *e = &urot_key_entries[i];
		const uint8_t *key_data = blob_field_ptr(blob, e->key_offset);
		const uint8_t *sig      = blob_field_ptr(blob, e->sig_offset);

		MFG_LOG_INF("Validating %s...", e->name);

		if (blob_bytes_all_zero(key_data, MFG_ED25519_PUBKEY_LEN)) {
			MFG_LOG_INF(" SKIP (no key data in blob)\n");
			continue;
		}
		MFG_LOG_INF(" OK\n");

		if (slot_is_occupied(e->slot_id)) {
			MFG_LOG_INF("%s already provisioned.\n", e->name);
			if (!check_key_attributes(e->slot_id, e->name)) {
				recovery_suspend(false);
			}
		} else {
			if (!provision_key(e->name, e->slot_id,
					   key_data, MFG_ED25519_PUBKEY_LEN)) {
				recovery_suspend(false);
			}
		}

		if (blob_bytes_all_zero(sig, MFG_ED25519_SIG_LEN)) {
			MFG_LOG_INF("Skipping signature check for %s (no signature in blob).\n",
				    e->name);
			continue;
		}

		if (!verify_key_signature(e->name, e->slot_id, sig, MFG_ED25519_SIG_LEN)) {
			recovery_suspend(false);
		}
	}

	MFG_LOG_INF("\n");
	kmu_print_slot_summary();
}

/* ---------------------------------------------------------------------------
 * Step 12 — Revoke manufacturing application authentication key
 * ---------------------------------------------------------------------------
 */
void step12_key_revoke_mfg_key(void)
{
	MFG_LOG_INF("Revoking the MANUFACTURING_APP_KEY...");

	/*
	 * The manufacturing app key is stored in KMU slot
	 * CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT and was provisioned with
	 * CRACEN_KEY_PERSISTENCE_REVOKABLE. psa_destroy_key() therefore moves
	 * the slot to the 'Erased' state and the key cannot be re-provisioned.
	 *
	 * In a full bootloader chain the slot is provisioned by BL1/NSIB; in
	 * that case verify the bootloader configuration uses REVOKABLE so this
	 * revocation is meaningful.
	 */
	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT);

	psa_status_t status = psa_destroy_key(key_id);

	if (status == PSA_SUCCESS) {
		MFG_LOG_INF(" OK\n");
	} else {
		MFG_LOG_INF(" FAIL (psa_destroy_key returned %d)\n", status);
		MFG_LOG_ERR("MANUFACTURING_APP_KEY could not be revoked.\n");
		MFG_LOG_ERR("Manufacturing app access to the device is not revoked. "
			    "This is a security defect — halting.\n");
		recovery_suspend(false);
	}

	MFG_LOG_INF("MANUFACTURING_APP_KEY revoked. Manufacturing app can no longer "
		    "authenticate to this device.\n");
}
