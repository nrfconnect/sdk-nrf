/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <sdfw/sdfw_services/suit_service.h>
#include <suit_manifests_state.h>

#define MAX_MANIFESTS 15

static void report_class_id(uint8_t *cid)
{

	static const uint8_t nRF54H20_sample_root_cid[] = {0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa,
							   0x58, 0xc5, 0xac, 0xce, 0xf9, 0xf5,
							   0x84, 0xc4, 0x11, 0x24};

	static const uint8_t nRF54H20_sample_app_recovery_cid[] = {
		0xA3, 0x7B, 0x77, 0xB0, 0x87, 0x0E, 0x57, 0x49,
		0xA8, 0x64, 0xF1, 0x44, 0x4A, 0xAF, 0xF5, 0x47};

	static const uint8_t nRF54H20_sample_app_cid[] = {0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8,
							  0x5f, 0xbc, 0x9e, 0x76, 0x7b, 0xc2,
							  0x9c, 0xe1, 0xb0, 0x4d};
	static const uint8_t nRF54H20_sample_app2_cid[] = {0x51, 0xde, 0x10, 0xb8, 0xee, 0x2e,
							   0x5b, 0x4b, 0x80, 0xee, 0x53, 0x4a,
							   0x4a, 0x3c, 0x04, 0xfc};

	static const uint8_t nRF54H20_sample_app3_cid[] = {0x2D, 0xCA, 0x15, 0xA5, 0xA3, 0x2E,
							   0x5A, 0x71, 0xBE, 0x54, 0xBA, 0x07,
							   0xBB, 0xAF, 0xAE, 0x27};

	static const uint8_t nRF54H20_sample_rad_recovery_cid[] = {
		0x58, 0x00, 0x98, 0xAE, 0xB2, 0x40, 0x50, 0x66,
		0xAF, 0x45, 0x57, 0x33, 0x25, 0xFC, 0x39, 0xF6};

	static const uint8_t nRF54H20_sample_rad_cid[] = {0x81, 0x6a, 0xa0, 0xa0, 0xaf, 0x11,
							  0x5e, 0xf2, 0x85, 0x8a, 0xfe, 0xb6,
							  0x68, 0xb2, 0xe9, 0xc9};
	static const uint8_t nRF54H20_sample_rad2_cid[] = {0x0a, 0xa4, 0xbe, 0x57, 0xb3, 0x1e,
							   0x57, 0x9c, 0xab, 0x81, 0x13, 0xbd,
							   0x90, 0xe1, 0x6e, 0xf7};
	static const uint8_t nRF54H20_nordic_top_cid[] = {0xf0, 0x3d, 0x38, 0x5e, 0xa7, 0x31,
							  0x56, 0x05, 0xb1, 0x5d, 0x03, 0x7f,
							  0x6d, 0xa6, 0x09, 0x7f};
	static const uint8_t nRF54H20_sys_cid[] = {0xc0, 0x8a, 0x25, 0xd7, 0x35, 0xe6, 0x59, 0x2c,
						   0xb7, 0xad, 0x43, 0xac, 0xc8, 0xd1, 0xd1, 0xc8};
	static const uint8_t nRF54H20_sec_cid[] = {0xd9, 0x6b, 0x40, 0xb7, 0x09, 0x2b, 0x5c, 0xd1,
						   0xa5, 0x9f, 0x9a, 0xf8, 0x0c, 0x33, 0x7e, 0xba};

	printk(" class ID:  %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	       cid[0], cid[1], cid[2], cid[3], cid[4], cid[5], cid[6], cid[7], cid[8], cid[9],
	       cid[10], cid[11], cid[12], cid[13], cid[14], cid[15]);

	if (memcmp(nRF54H20_sample_root_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_root) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_app_recovery_cid, cid, 16) == 0) {
		printk(" (nRF54H20_app_recovery) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_app_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_app) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_app2_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_app_local_2) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_app3_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_app_local_3) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_rad_recovery_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_rad_recovery) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_rad_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_rad) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_sample_rad2_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sample_rad2) - TO BE CHANGED for end product!!!\n");
	} else if (memcmp(nRF54H20_nordic_top_cid, cid, 16) == 0) {
		printk(" (nRF54H20_nordic_top)\n");
	} else if (memcmp(nRF54H20_sys_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sys)\n");
	} else if (memcmp(nRF54H20_sec_cid, cid, 16) == 0) {
		printk(" (nRF54H20_sec)\n");
	} else {
		printk("\n");
	}
}

static void report_vendor_id(uint8_t *vid, suit_manifest_role_t role)
{

	static const uint8_t nordic_vid[] = {0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85,
					     0x8f, 0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4};

	printk(" vendor ID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	       vid[0], vid[1], vid[2], vid[3], vid[4], vid[5], vid[6], vid[7], vid[8], vid[9],
	       vid[10], vid[11], vid[12], vid[13], vid[14], vid[15]);

	if (memcmp(nordic_vid, vid, 16) == 0) {

		if (((role & 0xF0) == SUIT_MANIFEST_DOMAIN_APP) ||
		    ((role & 0xF0) == SUIT_MANIFEST_DOMAIN_RAD)) {
			printk(" (nordicsemi.com) - TO BE CHANGED for end product!!!\n");
		} else {
			printk(" (nordicsemi.com)\n");
		}
	} else {
		printk("\n");
	}
}

static void report_role(suit_manifest_role_t role)
{
	printk("   role: 0x%02x", role);

	if (role == SUIT_MANIFEST_SEC_TOP) {
		printk(" (Nordic Top Manifest)\n");
	} else if (role == SUIT_MANIFEST_SEC_SDFW) {
		printk(" (SDFW and SDFW recovery updates)\n");
	} else if (role == SUIT_MANIFEST_SEC_SYSCTRL) {
		printk(" (System Controller Manifest)\n");
	} else if (role == SUIT_MANIFEST_APP_ROOT) {
		printk(" (Root Manifest)\n");
	} else if (role == SUIT_MANIFEST_APP_RECOVERY) {
		printk(" (Application Recovery Manifest)\n");
	} else if (role >= SUIT_MANIFEST_APP_LOCAL_1 && role <= SUIT_MANIFEST_APP_LOCAL_3) {
		printk(" (Application Local Manifest)\n");
	} else if (role == SUIT_MANIFEST_RAD_RECOVERY) {
		printk(" (Radio Recovery Manifest)\n");
	} else if (role >= SUIT_MANIFEST_RAD_LOCAL_1 && role <= SUIT_MANIFEST_RAD_LOCAL_2) {
		printk(" (Radio Local Manifest)\n");
	} else {
		printk("\n");
	}
}

static void report_downgrade_prevention_policy(suit_downgrade_prevention_policy_t policy)
{
	printk("   downgrade prevention policy:");

	if (policy == SUIT_DOWNGRADE_PREVENTION_DISABLED) {
		printk(" downgrade allowed\n");
	} else if (policy == SUIT_DOWNGRADE_PREVENTION_ENABLED) {
		printk(" downgrade forbidden\n");
	} else {
		printk(" invalid!\n");
	}
}

static void report_independent_updateability_policy(suit_independent_updateability_policy_t policy)
{
	printk("   independent updateability policy:");

	if (policy == SUIT_INDEPENDENT_UPDATE_DENIED) {
		printk(" independent update forbidden\n");
	} else if (policy == SUIT_INDEPENDENT_UPDATE_ALLOWED) {
		printk(" independent update allowed\n");
	} else {
		printk(" invalid!\n");
	}
}

static void report_signature_verification_policy(suit_signature_verification_policy_t policy)
{
	printk("   signature verification policy:");

	if (policy == SUIT_SIGNATURE_CHECK_DISABLED) {
		printk(" signature verification disabled\n");
	} else if (policy == SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE) {
		printk(" signature verification on update\n");
	} else if (policy == SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT) {
		printk(" signature verification on update and boot\n");
	} else {
		printk(" invalid!\n");
	}
}

static void report_digest(uint8_t *digest, size_t size, int digest_alg_id)
{

	if (size >= 8) {
		printk("   digest: %02X%02X%02X%02X..%02X%02X%02X%02X", digest[0], digest[1],
		       digest[2], digest[3], digest[size - 4], digest[size - 3], digest[size - 2],
		       digest[size - 1]);
	}

	if (digest_alg_id == -16) {
		printk(" (sha-256)\n");
	} else if (digest_alg_id == -18) {
		printk(" (shake128)\n");
	} else if (digest_alg_id == -43) {
		printk(" (sha-384)\n");
	} else if (digest_alg_id == -44) {
		printk(" (sha-512)\n");
	} else if (digest_alg_id == -45) {
		printk(" (shake256)\n");
	} else {
		printk("\n");
	}
}

static void report_digest_status(int status)
{
	if (status == 2) {
		printk("   signature check not performed\n");
	} else if (status == 3) {
		printk("   signature check failed!\n");
	} else if (status == 4) {
		printk("   signature check passed\n");
	}
}

static void report_sequence_number(int sequence_number)
{
	printk("   sequence number: %d\n", sequence_number);
}

static void report_semver(int32_t *raw, size_t len)
{

	int32_t major = 0;
	int32_t minor = 0;
	int32_t patch = 0;
	int32_t release_type = 0;
	int32_t release_type_idx = 0;
	int32_t pre_release_number = 0;

	if (len == 0 || len > 5) {
		return;
	}

	if (raw[0] < 0) {
		return;
	}

	for (size_t idx = 0; idx < len; idx++) {
		int32_t element = raw[idx];

		if (element >= 0 && release_type == 0) {
			switch (idx) {
			case 0:
				major = element;
				break;
			case 1:
				minor = element;
				break;
			case 2:
				patch = element;
				break;
			default:
				return;
			}

		} else if (element < 0 && element >= -3 && release_type == 0) {
			release_type = element;
			release_type_idx = idx;
		} else if (element >= 0 && release_type < 0 && idx - 1 == release_type_idx) {
			pre_release_number = element;
		} else {
			return;
		}
	}

	printk("   semantic version: %d.%d.%d", major, minor, patch);

	if (release_type == -1) {
		printk("-rc.%d\n", pre_release_number);
	} else if (release_type == -2) {
		printk("-beta.%d\n", pre_release_number);
	} else if (release_type == -3) {
		printk("-alpha.%d\n", pre_release_number);
	} else {
		printk("\n");
	}
}

void suit_mainfests_state_report(void)
{

	suit_plat_err_t rc;
	suit_manifest_role_t roles[MAX_MANIFESTS] = {0};
	size_t class_info_count = MAX_MANIFESTS;

	rc = suit_get_supported_manifest_roles(roles, &class_info_count);
	if (rc != SUIT_PLAT_SUCCESS) {
		return;
	}

	printk("-------------------------------------------------------------\n");

	for (size_t i = 0; i < class_info_count; i++) {

		suit_manifest_role_t role = roles[i];
		suit_ssf_manifest_class_info_t class_info = {0};

		rc = suit_get_supported_manifest_info(role, &class_info);
		if (rc != SUIT_PLAT_SUCCESS) {
			continue;
		}

		report_class_id(class_info.class_id.raw);
		report_vendor_id(class_info.vendor_id.raw, class_info.role);
		report_role(class_info.role);
		report_downgrade_prevention_policy(class_info.downgrade_prevention_policy);
		report_independent_updateability_policy(
			class_info.independent_updateability_policy);
		report_signature_verification_policy(class_info.signature_verification_policy);

		unsigned int seq_num = 0;
		suit_semver_raw_t semver_raw = {0};
		suit_digest_status_t digest_status = SUIT_DIGEST_UNKNOWN;
		int digest_alg_id = 0;
		uint8_t digest_buf[64] = {0};
		suit_plat_mreg_t digest;

		digest.mem = digest_buf;
		digest.size = sizeof(digest_buf);

		rc = suit_get_installed_manifest_info(&class_info.class_id, &seq_num, &semver_raw,
						      &digest_status, &digest_alg_id, &digest);

		if (rc == SUIT_PLAT_ERR_NOT_FOUND || digest.size == 0) {
			printk("   Manifest not installed or damaged!\n\n");
			continue;
		} else if (rc != SUIT_PLAT_SUCCESS) {
			printk("\n");
			continue;
		}

		report_digest(digest_buf, digest.size, digest_alg_id);
		report_digest_status(digest_status);
		report_sequence_number(seq_num);
		report_semver(semver_raw.raw, semver_raw.len);

		printk("\n");
	}
}
