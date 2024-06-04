/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_METADATA_H__
#define SUIT_METADATA_H__

#include <stddef.h>
#include <stdint.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int suit_metadata_err_t;

/**< Content of compared suit_uuid_t structures differs */
#define METADATA_ERR_COMPARISON_FAILED 1

/* Define constant values for backward compatibility purposes. */
typedef enum {
	/** Digest status value uninitialized (invalid). */
	SUIT_DIGEST_UNKNOWN = 0,
	/** Digest value does not match. */
	SUIT_DIGEST_MISMATCH = 1,
	/** Digest value match, but was not checked for authenticity. */
	SUIT_DIGEST_UNAUTHENTICATED = 2,
	/** Digest value match, but signature verification failed. */
	SUIT_DIGEST_INCORRECT_SIGNATURE = 3,
	/** Digest value match and authenticated. */
	SUIT_DIGEST_AUTHENTICATED = 4,
} suit_digest_status_t;

/* Values aligned with suit-update-management encoding. */
typedef enum {
	/** Regular release. */
	SUIT_VERSION_RELEASE_NORMAL = 0,
	/** Release candidate. */
	SUIT_VERSION_RELEASE_RC = -1,
	/** Beta pre-release. */
	SUIT_VERSION_RELEASE_BETA = -2,
	/** Alpha pre-release. */
	SUIT_VERSION_RELEASE_ALPHA = -3,
} suit_version_release_type_t;

/* Semantic version of the manifest. */
typedef struct {
	/** Type of the release. */
	suit_version_release_type_t type;
	/* Major number. */
	int32_t major;
	/* Minor number. */
	int32_t minor;
	/* Patch number.*/
	int32_t patch;
	/* Pre-release number. Always set to zero for regular releases. */
	int32_t pre_release_number;
} suit_version_t;

/* Raw semantic version, as defined by the SUIT specification. */
typedef struct {
	int32_t raw[5];
	size_t len;
} suit_semver_raw_t;

/* Manifest role encoded as two nibbles: <domain ID> <index>. */
typedef enum {
	/** Manifest role uninitialized (invalid). */
	SUIT_MANIFEST_UNKNOWN = 0x00,

	/** Manifest describes the entry-point for all Nordic-controlled manifests. */
	SUIT_MANIFEST_SEC_TOP = 0x10,
	/** Manifest describes SDFW firmware and recovery updates. */
	SUIT_MANIFEST_SEC_SDFW = 0x11,
	/** Manifest describes SYSCTRL firmware update and boot procedures. */
	SUIT_MANIFEST_SEC_SYSCTRL = 0x12,

	/** Manifest describes the entry-point for all OEM-controlled manifests. */
	SUIT_MANIFEST_APP_ROOT = 0x20,
	/** Manifest describes OEM-specific recovery procedure. */
	SUIT_MANIFEST_APP_RECOVERY = 0x21,
	/** Manifest describes OEM-specific binaries, specific for application core. */
	SUIT_MANIFEST_APP_LOCAL_1 = 0x22,
	/** Manifest describes OEM-specific binaries, specific for application core. */
	SUIT_MANIFEST_APP_LOCAL_2 = 0x23,
	/** Manifest describes OEM-specific binaries, specific for application core. */
	SUIT_MANIFEST_APP_LOCAL_3 = 0x24,

	/** Manifest describes radio part of OEM-specific recovery procedure. */
	SUIT_MANIFEST_RAD_RECOVERY = 0x30,
	/** Manifest describes OEM-specific binaries, specific for radio core. */
	SUIT_MANIFEST_RAD_LOCAL_1 = 0x31,
	/** Manifest describes OEM-specific binaries, specific for radio core. */
	SUIT_MANIFEST_RAD_LOCAL_2 = 0x32,
} suit_manifest_role_t;

/** The 128-bit UUID, used for identifying vendors as well as classes. */
typedef struct {
	uint8_t raw[16];
} suit_uuid_t;

/** The 128-bit UUID, identifying the SUIT manifest class. */
typedef suit_uuid_t suit_manifest_class_id_t;

/** Downgrade prevention policy for the manifest */
typedef enum {
	/** No downgrade prevention. */
	SUIT_DOWNGRADE_PREVENTION_DISABLED = 1,
	/** Update forbidden if candidate version is lower than installed version */
	SUIT_DOWNGRADE_PREVENTION_ENABLED = 2,
	/** Unknown downgrade prevention policy */
	SUIT_DOWNGRADE_PREVENTION_UNKNOWN = 3,
} suit_downgrade_prevention_policy_t;

/** Policy determining if the manifest can be updated independently from its parent */
typedef enum {
	/** Independent update is forbidden. */
	SUIT_INDEPENDENT_UPDATE_DENIED = 1,
	/** Independent update is allowed. */
	SUIT_INDEPENDENT_UPDATE_ALLOWED = 2,
	/** Unknown independent updateability policy. */
	SUIT_INDEPENDENT_UPDATE_UNKNOWN = 3,
} suit_independent_updateability_policy_t;

/** Manifest signature verification policy */
typedef enum {
	/** Do not verify the manifest signature */
	SUIT_SIGNATURE_CHECK_DISABLED = 1,
	/** Verify the manifest signature only when performing update */
	SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE = 2,
	/** Verify the manifest signature only both when performing update and when booting */
	SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT = 3,
	/** Unknown signature verification policy */
	SUIT_SIGNATURE_CHECK_UNKNOWN = 4,
} suit_signature_verification_policy_t;

typedef struct {
	const suit_uuid_t *vendor_id;
	const suit_manifest_class_id_t *class_id;
	suit_manifest_role_t role;
} suit_manifest_class_info_t;

/* Component numbers for Secure Domain components */
typedef enum {
	/* Secure Domain FW */
	SUIT_SECDOM_COMPONENT_NUMBER_SDFW = 1,
	/* Secure Domain Recovery FW */
	SUIT_SECDOM_COMPONENT_NUMBER_SDFW_RECOVERY = 2,
} suit_secure_domain_component_number_t;

/**
 * @brief Checks if two suit_uuid_t structures hold the same uuid value
 *
 * @param[in]  uuid1  UUID to compare.
 * @param[in]  uuid2  UUID to compare with.
 *
 * @retval SUIT_PLAT_SUCCESS              on success
 * @retval SUIT_PLAT_ERR_INVAL            invalid parameter, i.e. null pointer
 * @retval METADATA_ERR_COMPARISON_FAILED content of UUIDs differs
 */
suit_metadata_err_t suit_metadata_uuid_compare(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2);

/** @brief Convert semantic version, encoded as array of integers into structure.
 *
 * @details The meaning of array elements is defined in suit-update-management extension.
 *
 * @param[out]  version    Pointer to the semantic version structure to be filled.
 * @param[in]   array      The input array with integers.
 * @param[in]   array_len  Length of the input array.
 *
 * @retval SUIT_PLAT_ERR_INVAL         If one of the input parameters is NULL.
 * @retval SUIT_PLAT_ERR_SIZE          If the input array is too short to hold semantic version.
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS If version is negative or the release type is not supported.
 * @retval SUIT_PLAT_SUCCESS           If conversion was successful.
 */
suit_metadata_err_t suit_metadata_version_from_array(suit_version_t *version, int32_t *array,
						     size_t array_len);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_METADATA_H__ */
