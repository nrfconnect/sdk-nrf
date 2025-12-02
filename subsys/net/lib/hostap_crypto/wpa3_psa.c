/*
 * WPA3-SAE implementation using PSA APIs
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * This implementation provides a drop-in replacement for the original SAE
 * implementation using PSA PAKE APIs. It supports:
 * - Group 19 (NIST P-256) only
 * - H2E (Hash-to-Element) support
 * - HNP (Hash-and-Password) support
 * - No GDH (Group-Dependent Hash) support
 * - Exact same API as original SAE implementation
 */

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "common/wpa_common.h"
#include "utils/const_time.h"
#include "crypto/crypto.h"
#include "crypto/sha256.h"
#include "ieee802_11_defs.h"
#include "sae.h"

#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "mbedtls/md.h"
#include <string.h>
/* WPA3-SAE PSA implementation constants */
#define WPA3_PSA_MAX_COMMIT_LEN	 1024
#define WPA3_PSA_MAX_CONFIRM_LEN 1024
/* SAE commit: wire = [group(2)][scalar(32)][element(64)]; PSA/Oberon = [scalar][element] */
#define WPA3_PSA_SAE_COMMIT_WIRE_LEN 98
#define WPA3_PSA_SAE_COMMIT_PSA_LEN  96
#define WPA3_PSA_SAE_GROUP_19        19
#define WPA3_PSA_MAX_PT_PASSWORD_MAP 8

/* Password storage for PT-based operations */
struct wpa3_psa_pt_password {
	const struct sae_pt *pt;
	u8 password[256];
	size_t password_len;
	u8 ssid[32];
	size_t ssid_len;
};

static struct wpa3_psa_pt_password pt_password_map[WPA3_PSA_MAX_PT_PASSWORD_MAP];
static int pt_password_map_count;

/* Setup operation parameters */
struct wpa3_psa_setup_params {
	const u8 *addr1;
	const u8 *addr2;
	const u8 *password;
	size_t password_len;
	const u8 *ssid;
	size_t ssid_len;
	int group;
	bool h2e;
	bool gdh;
};

/* Internal PSA operation context */
struct wpa3_psa_operation {
	psa_pake_operation_t pake_op;
	psa_pake_cipher_suite_t cipher_suite;
	psa_key_id_t password_key;
	enum sae_state state;
	u8 own_addr[ETH_ALEN];
	u8 peer_addr[ETH_ALEN];
	u8 ssid[32];
	size_t ssid_len;
	u8 password[256];
	size_t password_len;
	u8 shared_key[256]; /* Increased for PSA PAKE commit/confirm data */
	size_t shared_key_len;
	u8 local_commit[256]; /* Store local commit for context calculation */
	size_t local_commit_len;
	u8 peer_commit[256]; /* Store peer commit for context calculation */
	size_t peer_commit_len;
	u8 pmk[SAE_PMK_LEN_MAX];
	size_t pmk_len;
	u8 kck[SAE_KCK_LEN];
	u8 pmkid[SAE_PMKID_LEN];
	int group;
	psa_algorithm_t hash_alg;
	bool h2e_enabled;
	bool gdh_enabled;
	u16 send_confirm;
	u16 rc;
};

/* Forward declarations */

static int wpa3_psa_setup_operation(struct wpa3_psa_operation *op,
				    const struct wpa3_psa_setup_params *params);
static int wpa3_psa_derive_commit(struct wpa3_psa_operation *op);
static int wpa3_psa_process_commit(struct wpa3_psa_operation *op, const u8 *commit_data,
				   size_t commit_len);
static int wpa3_psa_derive_confirm(struct wpa3_psa_operation *op);
static int wpa3_psa_process_confirm(struct wpa3_psa_operation *op, const u8 *confirm_data,
				    size_t confirm_len);
static int wpa3_psa_derive_keys(struct wpa3_psa_operation *op);
static void wpa3_psa_cleanup_operation(struct wpa3_psa_operation *op);
static struct sae_pt *wpa3_psa_derive_pt_group(int group, const u8 *ssid, size_t ssid_len,
					       const u8 *password, size_t password_len,
					       const char *identifier);
/* Map sae_data to internal PSA operation */
static struct wpa3_psa_operation *get_psa_op(struct sae_data *sae)
{
	if (!sae) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: get_psa_op failed - sae is NULL");
		return NULL;
	}

	if (!sae->tmp) {
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: get_psa_op - sae->tmp is NULL (expected during cleanup)");
		return NULL;
	}

	return (struct wpa3_psa_operation *)sae->tmp;
}

/* Initialize PSA operation in sae_data */
static int init_psa_op(struct sae_data *sae)
{
	struct wpa3_psa_operation *op;

	if (!sae) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: init_psa_op failed - sae is NULL");
		return -1;
	}

	/* Check if operation already exists */
	if (sae->tmp) {
		wpa3_psa_cleanup_operation((struct wpa3_psa_operation *)sae->tmp);
		os_free(sae->tmp);
		sae->tmp = NULL;
	}

	/* Allocate PSA operation context */
	op = os_zalloc(sizeof(*op));
	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: init_psa_op failed - memory allocation failed");
		return -1;
	}

	/* Initialize cipher suite first */
	op->cipher_suite = psa_pake_cipher_suite_init();

	/* Initialize PSA PAKE operation */
	op->pake_op = psa_pake_operation_init();

	/* Initialize operation fields */
	op->state = SAE_NOTHING;
	op->password_key = PSA_KEY_ID_NULL;
	op->send_confirm = 1; /* Initialize send_confirm counter to 1 (not 0) */
	op->shared_key_len = 0;
	op->pmk_len = 0;
	op->group = 19; /* Default to group 19 (NIST P-256) */
	/* H2E is determined by whether PT is used - if PT is used, H2E should be enabled */
	/* This will be set correctly when sae_prepare_commit_pt is called */
	op->h2e_enabled = false;
	op->gdh_enabled = false;

	/* Clear sensitive data */
	os_memset(op->password, 0, sizeof(op->password));
	os_memset(op->shared_key, 0, sizeof(op->shared_key));
	os_memset(op->pmk, 0, sizeof(op->pmk));
	os_memset(op->kck, 0, sizeof(op->kck));
	os_memset(op->pmkid, 0, sizeof(op->pmkid));

	/* Store in sae_data->tmp */
	sae->tmp = (struct sae_temporary_data *)op;

	return 0;
}

/**
 * sae_set_group - Set SAE group (exact same API as original)
 * @sae: SAE data
 * @group: DH group
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_set_group(struct sae_data *sae, int group)
{
	struct wpa3_psa_operation *op;

	if (!sae) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_set_group failed - sae is NULL");
		return -1;
	}

	/* Initialize PSA operation if not already done */
	if (!sae->tmp) {
		if (init_psa_op(sae) != 0) {
			wpa_printf(MSG_ERROR,
				   "WPA3-PSA: sae_set_group failed - init_psa_op failed");
			return -1;
		}
	}

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_set_group failed - get_psa_op returned NULL");
		return -1;
	}

	/* Handle group selection - group 0 means auto-select, default to group 19 (NIST P-256) */
	if (group == 0) {
		group = 19;
	} else if (group != 19) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Only group 19 (NIST P-256) is supported, got %d",
			   group);
		return -1;
	}

	op->group = group;
	return 0;
}

/**
 * sae_clear_temp_data - Clear temporary SAE data (exact same API as original)
 * @sae: SAE data
 */
void sae_clear_temp_data(struct sae_data *sae)
{
	struct wpa3_psa_operation *op;

	if (!sae) {
		return;
	}

	op = get_psa_op(sae);
	if (op) {
		wpa3_psa_cleanup_operation(op);
		sae->tmp = NULL;
	}
}

/**
 * sae_clear_data - Clear all SAE data (exact same API as original)
 * @sae: SAE data
 */
void sae_clear_data(struct sae_data *sae)
{
	if (!sae) {
		return;
	}

	sae_clear_temp_data(sae);

	if (sae->tmp) {
		os_free(sae->tmp);
		sae->tmp = NULL;
	}
}

/**
 * sae_prepare_commit - Prepare SAE commit message (exact same API as original)
 * @addr1: First MAC address (typically own address)
 * @addr2: Second MAC address (typically peer address)
 * @password: Password for SAE
 * @password_len: Length of password
 * @sae: SAE data
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_prepare_commit(const u8 *addr1, const u8 *addr2, const u8 *password, size_t password_len,
		       struct sae_data *sae)
{
	struct wpa3_psa_operation *op;
	int res;

	if (!sae) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_prepare_commit failed - sae is NULL");
		return -1;
	}

	if (!addr1 || !addr2 || !password) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: sae_prepare_commit failed - invalid parameters (addr1=%p, "
			   "addr2=%p, password=%p)",
			   addr1, addr2, password);
		return -1;
	}

	/* Initialize PSA operation if not already done */
	if (init_psa_op(sae) < 0) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_prepare_commit failed - init_psa_op failed");
		return -1;
	}

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: sae_prepare_commit failed - get_psa_op returned NULL");
		return -1;
	}

	/* Store addresses and password for the full SAE flow */
	os_memcpy(op->own_addr, addr1, ETH_ALEN);
	os_memcpy(op->peer_addr, addr2, ETH_ALEN);
	os_memcpy(op->password, password, password_len);
	op->password_len = password_len;

	/* Set up PSA operation for the full SAE flow */
	struct wpa3_psa_setup_params params = {
		.addr1 = addr1,
		.addr2 = addr2,
		.password = password,
		.password_len = password_len,
		.ssid = op->ssid,
		.ssid_len = op->ssid_len,
		.group = op->group,
		.h2e = op->h2e_enabled,
		.gdh = op->gdh_enabled,
	};
	res = wpa3_psa_setup_operation(op, &params);
	if (res < 0) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_prepare_commit failed - setup failed");
		return -1;
	}

	/* Derive commit (M1 message) */
	res = wpa3_psa_derive_commit(op);
	if (res < 0) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_prepare_commit failed - derive failed");
		return -1;
	}

	/* Set state to committed - ready to send M1 */
	sae->state = SAE_COMMITTED;

	return 0;
}

/**
 * sae_write_commit - Write SAE commit message to buffer (exact same API as original)
 * @sae: SAE data
 * @buf: Buffer to write commit message
 * @token: Token
 * @identifier: Identifier
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_write_commit(struct sae_data *sae, struct wpabuf *buf, const struct wpabuf *token,
		     const char *identifier)
{
	struct wpa3_psa_operation *op;
	u8 *pos;

	op = get_psa_op(sae);
	if (!op || sae->state != SAE_COMMITTED) {
		return -1;
	}

	/* Get commit data from PSA operation */
	if (op->shared_key_len == 0) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: No commit data available");
		return -1;
	}

	/* PSA outputs 96 bytes (scalar+element); wire format is 98 (group+scalar+element) */
	size_t commit_wire_len;

	if (op->shared_key_len == WPA3_PSA_SAE_COMMIT_PSA_LEN) {
		commit_wire_len = WPA3_PSA_SAE_COMMIT_WIRE_LEN;
		pos = wpabuf_put(buf, commit_wire_len);
		/* Group ID in wire format is little-endian (matches hostap wpabuf_put_le16) */
		pos[0] = WPA3_PSA_SAE_GROUP_19 & 0xff;
		pos[1] = (WPA3_PSA_SAE_GROUP_19 >> 8) & 0xff;
		os_memcpy(pos + 2, op->shared_key, WPA3_PSA_SAE_COMMIT_PSA_LEN);
	} else if (op->shared_key_len == WPA3_PSA_SAE_COMMIT_WIRE_LEN) {
		commit_wire_len = op->shared_key_len;
		pos = wpabuf_put(buf, commit_wire_len);
		os_memcpy(pos, op->shared_key, op->shared_key_len);
	} else {
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: Commit data wrong size (%zu bytes, expected %d or %d)",
			   op->shared_key_len, WPA3_PSA_SAE_COMMIT_PSA_LEN,
			   WPA3_PSA_SAE_COMMIT_WIRE_LEN);
		return -1;
	}

	/* Caller (sme) may prepend 4 bytes (transaction seq + status); total frame = 4 + 98 */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Commit payload written (%zu bytes)", commit_wire_len);
	wpa_hexdump(MSG_DEBUG, "WPA3-PSA: Commit payload", pos, commit_wire_len);
	return 0;
}

/**
 * sae_parse_commit - Parse and process received commit message (exact same API as original)
 * @sae: SAE data
 * @data: Commit message data
 * @len: Length of commit message
 * @token: Token
 * @token_len: Token length
 * @allowed_groups: Allowed groups
 * @h2e: H2E flag
 * @ie_offset: IE offset
 *
 * Returns: 0 on success, SAE_SILENTLY_DISCARD on discard, -1 on failure
 */
u16 sae_parse_commit(struct sae_data *sae, const u8 *data, size_t len, const u8 **token,
		     size_t *token_len, int *allowed_groups, int h2e, int *ie_offset)
{
	struct wpa3_psa_operation *op;
	u16 res = WLAN_STATUS_SUCCESS;

	op = get_psa_op(sae);
	if (!op) {
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	/* Update H2E flag if peer indicates H2E support */
	/* Note: This is informational - commit is already sent, so we can't change it */
	if (h2e) {
		op->h2e_enabled = true;
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Peer indicates H2E support");
	}

	/* SAE commit on wire is 98 bytes: [group][scalar][element] */
	if (len != WPA3_PSA_SAE_COMMIT_WIRE_LEN) {
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	/* Store the full commit message for PSA processing */
	os_memcpy(op->shared_key, data, len);
	op->shared_key_len = len;

	/* Also store for context calculation */
	os_memcpy(op->peer_commit, data, len);
	op->peer_commit_len = len;

	/* Process the commit using PSA */
	res = wpa3_psa_process_commit(op, op->shared_key, op->shared_key_len);
	if (res != WLAN_STATUS_SUCCESS) {
		return res;
	}

	/* Parse optional fields */
	if (token) {
		*token = NULL;
		*token_len = 0;
	}

	if (allowed_groups) {
		*allowed_groups = 0; /* No groups specified */
	}

	if (ie_offset) {
		*ie_offset = len; /* No IE offset since we use the full message */
	}

	return WLAN_STATUS_SUCCESS;
}

/**
 * sae_write_confirm - Write SAE confirm message to buffer (exact same API as original)
 * @sae: SAE data
 * @buf: Buffer to write confirm message
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_write_confirm(struct sae_data *sae, struct wpabuf *buf)
{
	struct wpa3_psa_operation *op;
	u8 *pos;

	op = get_psa_op(sae);
	if (!op || op->shared_key_len == 0) {
		return -1;
	}

	/* Format SAE confirm message */
	/* SAE confirm format: [confirm] - PSA confirm output already includes send_confirm */

	/* Add confirm data - use the full PSA confirm output (includes send_confirm) */
	pos = wpabuf_put(buf, op->shared_key_len);
	os_memcpy(pos, op->shared_key, op->shared_key_len);

	return 0;
}

/**
 * sae_check_confirm - Check SAE confirm message (exact same API as original)
 * @sae: SAE data
 * @data: Confirm message data
 * @len: Length of confirm message
 * @ie_offset: IE offset
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_check_confirm(struct sae_data *sae, const u8 *data, size_t len, int *ie_offset)
{
	struct wpa3_psa_operation *op;
	const u8 *pos, *end;
	size_t confirm_len;
	int res;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Checking confirm message (M2)");

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: No PSA operation available");
		return -1;
	}

	pos = data;
	end = data + len;

	/* Parse SAE confirm message format */
	/* Expected format: [confirm] - PSA confirm output already includes send_confirm */

	/* Parse confirm data (includes send_confirm) */
	if (end - pos < 1) { /* Minimum confirm size */
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Confirm message too short for confirm data");
		return -1;
	}

	confirm_len = end - pos;
	if (confirm_len > sizeof(op->shared_key)) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Confirm data too large");
		return -1;
	}

	/* Process the confirm using PSA (includes send_confirm) */
	res = wpa3_psa_process_confirm(op, pos, confirm_len);
	if (res < 0) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Failed to process confirm");
		return -1;
	}

	/* Set state to accepted */
	sae->state = SAE_ACCEPTED;

	/* Populate the sae_data structure with the derived keys */
	/* This is what the supplicant expects to find in sae->pmk and sae->pmk_len */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Copying PMK to sae_data - op->pmk_len: %zu", op->pmk_len);
	os_memcpy(sae->pmk, op->pmk, op->pmk_len);
	sae->pmk_len = op->pmk_len;
	os_memcpy(sae->pmkid, op->pmkid, SAE_PMKID_LEN);
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Populated sae_data fields - pmk_len: %zu", sae->pmk_len);

	/* Set IE offset if requested */
	if (ie_offset) {
		*ie_offset = pos - data;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Confirm message checked successfully");
	return 0;
}

/**
 * sae_group_allowed - Check if SAE group is allowed (exact same API as original)
 * @sae: SAE data
 * @allowed_groups: Allowed groups
 * @group: Group to check
 *
 * Returns: 0 if allowed, -1 if not allowed
 */
u16 sae_group_allowed(struct sae_data *sae, int *allowed_groups, u16 group)
{
	/* Only allow group 19 */
	if (group == 19) {
		return 0;
	}
	return -1;
}

/**
 * sae_state_txt - Get SAE state text (exact same API as original)
 * @state: SAE state
 *
 * Returns: State text string
 */
const char *sae_state_txt(enum sae_state state)
{
	switch (state) {
	case SAE_NOTHING:
		return "NOTHING";
	case SAE_COMMITTED:
		return "COMMITTED";
	case SAE_CONFIRMED:
		return "CONFIRMED";
	case SAE_ACCEPTED:
		return "ACCEPTED";
	default:
		return "UNKNOWN";
	}
}

/**
 * sae_prepare_commit_pt - Prepare SAE commit message with PT (exact same API as original)
 * @sae: SAE data
 * @pt: SAE PT data
 * @addr1: First MAC address
 * @addr2: Second MAC address
 * @rejected_groups: Rejected groups
 * @pk: SAE PK data
 *
 * Returns: 0 on success, -1 on failure
 *
 * Note: For PSA implementation, the password must be set in op->password
 * before calling this function. This is typically done by calling
 * sae_prepare_commit first or by storing it when PT is derived.
 */
int sae_prepare_commit_pt(struct sae_data *sae, const struct sae_pt *pt, const u8 *addr1,
			  const u8 *addr2, int *rejected_groups, const struct sae_pk *pk)
{
	struct wpa3_psa_operation *op;
	int res;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Prepare commit with PT (PSA mode)");

	if (!sae || !pt) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Invalid parameters for PT commit");
		return -1;
	}

	/* PSA PAKE handles PT internally, use standard SAE flow */

	/* Initialize PSA operation if not already done */
	if (init_psa_op(sae) < 0) {
		return -1;
	}

	op = get_psa_op(sae);
	if (!op) {
		return -1;
	}

	/* Store addresses for the SAE flow */
	os_memcpy(op->own_addr, addr1, ETH_ALEN);
	os_memcpy(op->peer_addr, addr2, ETH_ALEN);

	/* Retrieve password and SSID from PT password map if not already set */
	if (op->password_len == 0 || op->ssid_len == 0) {
		int i;
		bool found = false;

		/* Look up password and SSID stored when PT was derived */
		for (i = 0; i < pt_password_map_count; i++) {
			if (pt_password_map[i].pt == pt && pt_password_map[i].password_len > 0) {
				if (pt_password_map[i].password_len > sizeof(op->password)) {
					wpa_printf(MSG_ERROR,
						   "WPA3-PSA: Stored password too long (%zu > %zu)",
						   pt_password_map[i].password_len,
						   sizeof(op->password));
					return -1;
				}
				os_memcpy(op->password, pt_password_map[i].password,
					  pt_password_map[i].password_len);
				op->password_len = pt_password_map[i].password_len;

				/* Also retrieve SSID if available */
				if (pt_password_map[i].ssid_len > 0 &&
				    pt_password_map[i].ssid_len <= sizeof(op->ssid)) {
					os_memcpy(op->ssid, pt_password_map[i].ssid,
						  pt_password_map[i].ssid_len);
					op->ssid_len = pt_password_map[i].ssid_len;
				}

				found = true;
				wpa_printf(MSG_DEBUG,
					   "WPA3-PSA: Retrieved password and SSID from PT map "
					   "(pw_len=%zu, ssid_len=%zu)",
					   op->password_len, op->ssid_len);
				break;
			}
		}

		if (!found) {
			wpa_printf(MSG_ERROR,
				   "WPA3-PSA: sae_prepare_commit_pt failed - password not found "
				   "for PT. Password should have been stored when PT was derived.");
			return -1;
		}
	}

	/* Set up PSA operation for the full SAE flow with PT */
	/* When PT is used, H2E should be enabled (PT is derived using H2E KDF) */
	op->h2e_enabled = true;
	wpa_printf(MSG_DEBUG, "WPA3-PSA: PT mode - enabling H2E");

	struct wpa3_psa_setup_params params = {
		.addr1 = addr1,
		.addr2 = addr2,
		.password = op->password,
		.password_len = op->password_len,
		.ssid = op->ssid,
		.ssid_len = op->ssid_len,
		.group = op->group,
		.h2e = true, /* PT implies H2E */
		.gdh = op->gdh_enabled,
	};

	res = wpa3_psa_setup_operation(op, &params);
	if (res < 0) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Failed to setup operation for PT commit");
		return -1;
	}

	/* Derive commit (M1 message) */
	res = wpa3_psa_derive_commit(op);
	if (res < 0) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Failed to derive commit with PT");
		return -1;
	}

	/* Set state to committed - ready to send M1 */
	sae->state = SAE_COMMITTED;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: PT commit prepared successfully");
	return 0;
}

/**
 * sae_process_commit - Process SAE commit (exact same API as original)
 * @sae: SAE data
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_process_commit(struct sae_data *sae)
{
	struct wpa3_psa_operation *op;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: sae_process_commit called");

	if (!sae) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_process_commit failed - sae is NULL");
		return -1;
	}

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: sae_process_commit failed - get_psa_op returned NULL");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: sae_process_commit - operation: %p, state: %d", op,
		   op->state);

	if (op->state != SAE_COMMITTED) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: sae_process_commit failed - invalid state: %d (expected: %d)",
			   op->state, SAE_COMMITTED);
		return -1;
	}

	/* For PSA implementation, commit processing is done in sae_parse_commit */
	/* This function is called by the supplicant but we handle it in parse_commit */
	wpa_printf(MSG_DEBUG,
		   "WPA3-PSA: sae_process_commit - commit processing already done in parse_commit");
	return 0;
}

/**
 * sae_derive_pt - Derive SAE PT (exact same API as original)
 * @groups: Groups array
 * @ssid: SSID
 * @ssid_len: SSID length
 * @password: Password
 * @password_len: Password length
 * @identifier: Identifier
 *
 * Returns: SAE PT structure or NULL on failure
 */
struct sae_pt *sae_derive_pt(int *groups, const u8 *ssid, size_t ssid_len, const u8 *password,
			     size_t password_len, const char *identifier)
{
	struct sae_pt *pt = NULL, *last = NULL, *tmp;
	int default_groups[] = {19, 0};
	int i;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Deriving PT using PSA APIs only");

	if (!groups) {
		groups = default_groups;
	}
	for (i = 0; groups[i] > 0; i++) {
		tmp = wpa3_psa_derive_pt_group(groups[i], ssid, ssid_len, password, password_len,
					       identifier);
		if (!tmp) {
			continue;
		}

		if (last) {
			last->next = tmp;
		} else {
			pt = tmp;
		}
		last = tmp;
	}

	return pt;
}

/**
 * sae_deinit_pt - Deinitialize SAE PT (exact same API as original)
 * @pt: SAE PT structure
 */
void sae_deinit_pt(struct sae_pt *pt)
{
	int i;

	if (!pt) {
		return;
	}

	/* Clean up password and SSID from map */
	for (i = 0; i < pt_password_map_count; i++) {
		if (pt_password_map[i].pt == pt) {
			os_memset(pt_password_map[i].password, 0,
				  sizeof(pt_password_map[i].password));
			os_memset(pt_password_map[i].ssid, 0,
				  sizeof(pt_password_map[i].ssid));
			pt_password_map[i].password_len = 0;
			pt_password_map[i].ssid_len = 0;
			pt_password_map[i].pt = NULL;
		}
	}

	/* Free PT structure */
	os_free(pt);
}

/**
 * sae_derive_pwe_from_pt_ecc - Derive PWE from PT for ECC (exact same API as original)
 * @pt: SAE PT structure
 * @addr1: First MAC address
 * @addr2: Second MAC address
 *
 * Returns: ECC point or NULL on failure
 */
struct crypto_ec_point *sae_derive_pwe_from_pt_ecc(const struct sae_pt *pt, const u8 *addr1,
						   const u8 *addr2)
{
	/* For PSA implementation, we use PSA PAKE directly - return NULL */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: PWE derivation from PT not needed with PSA PAKE");
	return NULL;
}

/**
 * sae_derive_pwe_from_pt_ffc - Derive PWE from PT for FFC (exact same API as original)
 * @pt: SAE PT structure
 * @addr1: First MAC address
 * @addr2: Second MAC address
 *
 * Returns: FFC point or NULL on failure
 */
struct crypto_bignum *sae_derive_pwe_from_pt_ffc(const struct sae_pt *pt, const u8 *addr1,
						 const u8 *addr2)
{
	/* PSA implementation supports ECC groups only (group 19) */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: FFC groups not supported - ECC only");
	return NULL;
}

/**
 * sae_ecc_prime_len_2_hash_len - Convert ECC prime length to hash length (exact same API as
 * original)
 * @prime_len: Prime length
 *
 * Returns: Hash length
 */
size_t sae_ecc_prime_len_2_hash_len(size_t prime_len)
{
	/* For PSA implementation, only support group 19 (NIST P-256) */
	return 32;
}

/**
 * sae_ffc_prime_len_2_hash_len - Convert FFC prime length to hash length (exact same API as
 * original)
 * @prime_len: Prime length
 *
 * Returns: Hash length
 */
size_t sae_ffc_prime_len_2_hash_len(size_t prime_len)
{
	/* For PSA implementation, we don't use FFC - return default */
	return 32; /* Default to SHA-256 */
}

/**
 * sae_get_pmk - Get PMK from SAE data (exact same API as original)
 * @sae: SAE data
 * @pmk: Buffer for PMK
 * @pmk_len: Length of PMK buffer
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_get_pmk(struct sae_data *sae, u8 *pmk, size_t *pmk_len)
{
	struct wpa3_psa_operation *op;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: sae_get_pmk called");

	if (!sae || !pmk || !pmk_len) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_pmk failed - invalid parameters");
		return -1;
	}

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_pmk failed - no PSA operation");
		return -1;
	}

	/* Copy the PMK from our operation */
	if (*pmk_len < op->pmk_len) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_pmk failed - buffer too small (%zu < %zu)",
			   *pmk_len, op->pmk_len);
		return -1;
	}

	os_memcpy(pmk, op->pmk, op->pmk_len);
	*pmk_len = op->pmk_len;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: PMK provided, length: %zu", *pmk_len);
	return 0;
}

/**
 * sae_get_pmkid - Get PMKID from SAE data (exact same API as original)
 * @sae: SAE data
 * @pmkid: Buffer for PMKID
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_get_pmkid(struct sae_data *sae, u8 *pmkid)
{
	struct wpa3_psa_operation *op;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: sae_get_pmkid called");

	if (!sae || !pmkid) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_pmkid failed - invalid parameters");
		return -1;
	}

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_pmkid failed - no PSA operation");
		return -1;
	}

	/* Copy the PMKID from our operation */
	os_memcpy(pmkid, op->pmkid, SAE_PMKID_LEN);

	wpa_printf(MSG_DEBUG, "WPA3-PSA: PMKID provided");
	return 0;
}

/**
 * sae_get_kck - Get KCK from SAE data (exact same API as original)
 * @sae: SAE data
 * @kck: Buffer for KCK
 * @kck_len: Length of KCK buffer
 *
 * Returns: 0 on success, -1 on failure
 */
int sae_get_kck(struct sae_data *sae, u8 *kck, size_t *kck_len)
{
	struct wpa3_psa_operation *op;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: sae_get_kck called");

	if (!sae || !kck || !kck_len) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_kck failed - invalid parameters");
		return -1;
	}

	op = get_psa_op(sae);
	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_kck failed - no PSA operation");
		return -1;
	}

	if (*kck_len < SAE_KCK_LEN) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: sae_get_kck failed - buffer too small (%zu < %d)",
			   *kck_len, SAE_KCK_LEN);
		return -1;
	}

	/* PSA PAKE handles key confirmation internally - KCK is zero */
	/* Return zero KCK for API compatibility */
	os_memcpy(kck, op->kck, SAE_KCK_LEN);
	*kck_len = SAE_KCK_LEN;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: KCK provided, length: %zu", *kck_len);
	return 0;
}

/* Internal PSA implementation functions */

/**
 * wpa3_psa_derive_pt_group - Derive PT for a specific group using PSA only
 * @group: SAE group number
 * @ssid: SSID
 * @ssid_len: SSID length
 * @password: Password
 * @password_len: Password length
 * @identifier: Password identifier
 *
 * Returns: SAE PT structure or NULL on failure
 */
static struct sae_pt *wpa3_psa_derive_pt_group(int group, const u8 *ssid, size_t ssid_len,
					       const u8 *password, size_t password_len,
					       const char *identifier)
{
	struct sae_pt *pt;
	int i;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Derive PT - group %d (PSA only)", group);

	if (ssid_len > 32) {
		return NULL;
	}

	/* Handle group selection - group 0 means auto-select, default to group 19 (NIST P-256) */
	if (group == 0) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: PT derivation - Group 0 (auto-select) - using "
				      "group 19 (NIST P-256)");
		group = 19;
	} else if (group != 19) {
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: Unsupported group %d for PT derivation (only group 19 NIST "
			   "P-256 supported)",
			   group);
		return NULL;
	}

	pt = os_zalloc(sizeof(*pt));
	if (!pt) {
		return NULL;
	}

	pt->group = group;

	/* Store password and SSID for later use in sae_prepare_commit_pt */
	if (password && password_len > 0 && password_len <= 256) {
		/* Find empty slot or reuse existing for this PT */
		for (i = 0; i < WPA3_PSA_MAX_PT_PASSWORD_MAP; i++) {
			if (pt_password_map[i].pt == pt || pt_password_map[i].pt == NULL) {
				pt_password_map[i].pt = pt;
				os_memcpy(pt_password_map[i].password, password, password_len);
				pt_password_map[i].password_len = password_len;
				if (ssid && ssid_len > 0 && ssid_len <= 32) {
					os_memcpy(pt_password_map[i].ssid, ssid, ssid_len);
					pt_password_map[i].ssid_len = ssid_len;
				} else {
					pt_password_map[i].ssid_len = 0;
				}
				if (i >= pt_password_map_count) {
					pt_password_map_count = i + 1;
				}
				wpa_printf(MSG_DEBUG,
					   "WPA3-PSA: Stored password and SSID for PT (pw_len=%zu, "
					   "ssid_len=%zu)",
					   password_len, pt_password_map[i].ssid_len);
				break;
			}
		}
	}

	/* PSA PAKE handles PWE derivation internally, no need for EC/ECC structures */
	pt->ec = NULL;
	pt->ecc_pt = NULL;
	pt->dh = NULL;
	pt->ffc_pt = NULL;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: PT derived successfully for group %d (PSA mode)", group);
	return pt;
}

/**
 * wpa3_psa_setup_operation - Setup PSA operation
 * @op: PSA operation structure
 * @params: Setup parameters
 *
 * Returns: 0 on success, -1 on failure
 */
static int wpa3_psa_setup_operation(struct wpa3_psa_operation *op,
				    const struct wpa3_psa_setup_params *params)
{
	psa_status_t status;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_setup_operation failed - op is NULL");
		return -1;
	}

	if (!params) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_setup_operation failed - params is NULL");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Setting up PSA operation");
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Group: %d, H2E: %s, GDH: %s", params->group,
		   params->h2e ? "yes" : "no", params->gdh ? "yes" : "no");

	/* Store operation parameters */
	os_memcpy(op->own_addr, params->addr1, ETH_ALEN);
	os_memcpy(op->peer_addr, params->addr2, ETH_ALEN);
	os_memcpy(op->ssid, params->ssid, params->ssid_len);
	op->ssid_len = params->ssid_len;
	os_memcpy(op->password, params->password, params->password_len);
	op->password_len = params->password_len;
	op->group = params->group;
	op->h2e_enabled = params->h2e;
	op->gdh_enabled = params->gdh;

	/* Set up hash algorithm */
	op->hash_alg = PSA_ALG_SHA_256;

	/* Determine WPA3-SAE PAKE algorithm based on parameters */
	/* Note: H2E is a PWE derivation method, not a PAKE algorithm variant */
	/* For PAKE, we use FIXED (HnP) or GDH regardless of H2E */
	psa_algorithm_t pake_alg;
	if (params->gdh) {
		pake_alg = PSA_ALG_WPA3_SAE_GDH(op->hash_alg);
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Using GDH algorithm");
	} else {
		pake_alg = PSA_ALG_WPA3_SAE_FIXED(op->hash_alg);
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Using HnP (fixed) algorithm");
	}

	/* Set up cipher suite */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Setting up cipher suite");
	op->cipher_suite = psa_pake_cipher_suite_init();

	/* Set PAKE algorithm */
	psa_pake_cs_set_algorithm(&op->cipher_suite, pake_alg);
	wpa_printf(MSG_DEBUG, "WPA3-PSA: PAKE algorithm set to 0x%08x", pake_alg);

	/* Set primitive */
	psa_pake_primitive_t primitive =
		PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC, PSA_ECC_FAMILY_SECP_R1, 256);
	psa_pake_cs_set_primitive(&op->cipher_suite, primitive);
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Primitive set to ECC SECP_R1 256-bit");

	/* Hash is already included in the algorithm definition */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Hash (SHA-256) included in algorithm");

	/* Initialize PAKE operation */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Initializing PAKE operation");
	op->pake_op = psa_pake_operation_init();

	/* Set up password key */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Setting up password key");
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);

	/* Validate password before processing */
	if (!params->password || params->password_len == 0) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_setup_operation failed - invalid password (len=%zu)",
			   params->password_len);
		return -1;
	}

	/* For H2E, derive PT from password and use PT as key */
	/* For HnP, use raw password as key */
	if (params->h2e) {
		psa_key_derivation_operation_t kdf = PSA_KEY_DERIVATION_OPERATION_INIT;
		psa_key_attributes_t pw_attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_attributes_t pt_attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_id_t pw_key = PSA_KEY_ID_NULL;

		wpa_printf(MSG_DEBUG, "WPA3-PSA: H2E mode - deriving PT from password");
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Using hash algorithm: 0x%08x (SHA-256)",
			   op->hash_alg);
		wpa_printf(MSG_DEBUG, "WPA3-PSA: H2E algorithm: 0x%08x",
			   PSA_ALG_WPA3_SAE_H2E(op->hash_alg));

		/* Set up password key for H2E KDF */
		psa_set_key_usage_flags(&pw_attr, PSA_KEY_USAGE_DERIVE);
		psa_set_key_lifetime(&pw_attr, PSA_KEY_LIFETIME_VOLATILE);
		psa_set_key_type(&pw_attr, PSA_KEY_TYPE_PASSWORD);
		psa_set_key_algorithm(&pw_attr, PSA_ALG_WPA3_SAE_H2E(op->hash_alg));

		status = psa_import_key(&pw_attr, params->password, params->password_len, &pw_key);
		if (status != PSA_SUCCESS) {
			wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to import password for H2E: %d",
				   status);
			return -1;
		}

		/* Derive PT using H2E KDF */
		status = psa_key_derivation_setup(&kdf, PSA_ALG_WPA3_SAE_H2E(op->hash_alg));
		if (status != PSA_SUCCESS) {
			wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to setup H2E KDF: %d", status);
			psa_destroy_key(pw_key);
			return -1;
		}

		/* Set SSID as salt - SSID is required for H2E KDF */
		if (!params->ssid || params->ssid_len == 0) {
			wpa_printf(MSG_ERROR,
				   "WPA3-PSA: H2E requires SSID but it's not set (len=%zu)",
				   params->ssid_len);
			psa_key_derivation_abort(&kdf);
			psa_destroy_key(pw_key);
			return -1;
		}

		status = psa_key_derivation_input_bytes(&kdf, PSA_KEY_DERIVATION_INPUT_SALT,
							params->ssid, params->ssid_len);
		if (status != PSA_SUCCESS) {
			wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to set SSID salt: %d", status);
			psa_key_derivation_abort(&kdf);
			psa_destroy_key(pw_key);
			return -1;
		}

		/* Input password */
		status = psa_key_derivation_input_key(&kdf, PSA_KEY_DERIVATION_INPUT_PASSWORD,
						      pw_key);
		if (status != PSA_SUCCESS) {
			wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to input password: %d", status);
			psa_key_derivation_abort(&kdf);
			psa_destroy_key(pw_key);
			return -1;
		}

		psa_destroy_key(pw_key);

		/* Output PT directly as key - use output_key instead of output_bytes */
		/* Set up PT key attributes - matching test code pattern */
		/* Test code uses same attributes variable, so usage flags are already set */
		/* But for PT keys, we only need to set type and bits */
		psa_set_key_type(&pt_attr, PSA_KEY_TYPE_WPA3_SAE_ECC_PT(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&pt_attr, 256);
		/* Set usage flags to DERIVE (required for key derivation output) */
		psa_set_key_usage_flags(&pt_attr, PSA_KEY_USAGE_DERIVE);
		/* Note: PT keys don't need algorithm set - it's determined by the key type */
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Using PT key type for H2E (bits=256)");
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: KDF algorithm: 0x%08x, expected hash: SHA-256 (0x%08x)",
			   PSA_ALG_WPA3_SAE_H2E(op->hash_alg), PSA_ALG_SHA_256);

		/* Verify KDF state before output */
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: About to call output_key - KDF alg: 0x%08x, "
			   "PT type: 0x%08x, bits: 256, hash_alg: 0x%08x",
			   PSA_ALG_WPA3_SAE_H2E(op->hash_alg),
			   PSA_KEY_TYPE_WPA3_SAE_ECC_PT(PSA_ECC_FAMILY_SECP_R1),
			   op->hash_alg);
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: Key type check - IS_WPA3_SAE_ECC_PT: %d, "
			   "IS_WPA3_SAE_H2E: %d, GET_HASH: 0x%08x",
			   PSA_KEY_TYPE_IS_WPA3_SAE_ECC_PT(PSA_KEY_TYPE_WPA3_SAE_ECC_PT(PSA_ECC_FAMILY_SECP_R1)),
			   PSA_ALG_IS_WPA3_SAE_H2E(PSA_ALG_WPA3_SAE_H2E(op->hash_alg)),
			   PSA_ALG_GET_HASH(PSA_ALG_WPA3_SAE_H2E(op->hash_alg)));

		status = psa_key_derivation_output_key(&pt_attr, &kdf, &op->password_key);
		if (status != PSA_SUCCESS) {
			wpa_printf(MSG_ERROR,
				   "WPA3-PSA: Failed to derive PT key: %d (KDF alg: 0x%08x, "
				   "PT type: 0x%08x, bits: 256, hash_alg: 0x%08x, "
				   "PSA_ALG_GET_HASH: 0x%08x)",
				   status, PSA_ALG_WPA3_SAE_H2E(op->hash_alg),
				   PSA_KEY_TYPE_WPA3_SAE_ECC_PT(PSA_ECC_FAMILY_SECP_R1),
				   op->hash_alg,
				   PSA_ALG_GET_HASH(PSA_ALG_WPA3_SAE_H2E(op->hash_alg)));
			psa_key_derivation_abort(&kdf);
			return -1;
		}

		psa_key_derivation_abort(&kdf);
		wpa_printf(MSG_DEBUG, "WPA3-PSA: PT key derived successfully for H2E");
	} else {
		/* For HnP, use raw password */
		psa_set_key_type(&attributes, PSA_KEY_TYPE_PASSWORD);
		psa_set_key_algorithm(&attributes, pake_alg);
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Using PSA_KEY_TYPE_PASSWORD for HnP");

		/* Note: key_bits is not set for password keys - PSA infers it from data length */

		status = psa_import_key(&attributes, params->password, params->password_len,
					&op->password_key);
		if (status != PSA_SUCCESS) {
			wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to import password key: %d",
				   status);
			return -1;
		}
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Password key imported successfully for HnP");
	}

	/* Set up PAKE operation */

	status = psa_pake_setup(&op->pake_op, op->password_key, &op->cipher_suite);
	if (status != PSA_SUCCESS) {
		/* PSA_ERROR_NOT_SUPPORTED is -134 per PSA Crypto API */
		const int not_supported = ((int)status == -134);

		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to setup PAKE: %d (%s)",
			   (int)status,
			   not_supported ? "PSA_ERROR_NOT_SUPPORTED" : "see PSA status");
		if (not_supported) {
			wpa_printf(MSG_ERROR,
				   "WPA3-PSA: Crypto backend does not support PAKE; ensure "
				   "CONFIG_PSA_NEED_OBERON_WPA3_SAE and WPA3-PSA are enabled.");
		}
		psa_destroy_key(op->password_key);
		return -1;
	}

	/* Set user and peer identifiers (MAC addresses) - required for WPA3-SAE */
	wpa_printf(MSG_DEBUG,
		   "WPA3-PSA: Setting user identifier (own MAC): %02x:%02x:%02x:%02x:%02x:%02x",
		   op->own_addr[0], op->own_addr[1], op->own_addr[2], op->own_addr[3],
		   op->own_addr[4], op->own_addr[5]);
	status = psa_pake_set_user(&op->pake_op, op->own_addr, ETH_ALEN);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to set user: %d", status);
		psa_destroy_key(op->password_key);
		return -1;
	}

	wpa_printf(MSG_DEBUG,
		   "WPA3-PSA: Setting peer identifier (peer MAC): %02x:%02x:%02x:%02x:%02x:%02x",
		   op->peer_addr[0], op->peer_addr[1], op->peer_addr[2], op->peer_addr[3],
		   op->peer_addr[4], op->peer_addr[5]);
	status = psa_pake_set_peer(&op->pake_op, op->peer_addr, ETH_ALEN);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to set peer: %d", status);
		psa_destroy_key(op->password_key);
		return -1;
	}

	/* H2E is handled internally by PSA PAKE - no additional salt setup needed */
	if (params->h2e) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: H2E enabled - handled by PSA PAKE");
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Operation setup completed successfully");
	return 0;
}

/**
 * wpa3_psa_derive_commit - Derive SAE commit message (M1) using PSA
 * @op: PSA operation structure
 *
 * Returns: 0 on success, -1 on failure
 */
static int wpa3_psa_derive_commit(struct wpa3_psa_operation *op)
{
	psa_status_t status;
	u8 commit_data[1024]; /* Buffer for commit message */
	size_t commit_len = 0;

	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_derive_commit failed - op is NULL");
		return -1;
	}

	/* Generate our commit message using PSA PAKE */
	status = psa_pake_output(&op->pake_op, PSA_PAKE_STEP_COMMIT, commit_data,
				 sizeof(commit_data), &commit_len);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_derive_commit failed - psa_pake_output returned %d",
			   status);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Commit data generated: %zu bytes", commit_len);

	/* Store local commit data for later context calculation */
	if (commit_len > sizeof(op->local_commit)) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_derive_commit failed - commit data too large (%zu > "
			   "%zu)",
			   commit_len, sizeof(op->local_commit));
		return -1;
	}

	os_memcpy(op->local_commit, commit_data, commit_len);
	op->local_commit_len = commit_len;

	/* Also store in shared_key for backward compatibility */
	os_memcpy(op->shared_key, commit_data, commit_len);
	op->shared_key_len = commit_len;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Commit message derived successfully (%zu bytes)",
		   commit_len);
	return 0;
}

/**
 * wpa3_psa_process_commit - Process incoming commit message using PSA
 * @op: PSA operation structure
 * @commit_data: Incoming commit data
 * @commit_len: Length of commit data
 *
 * Returns: 0 on success, -1 on failure
 */
static int wpa3_psa_process_commit(struct wpa3_psa_operation *op, const u8 *commit_data,
				   size_t commit_len)
{
	psa_status_t status;
	uint8_t send_confirm_counter[2] = {0x01, 0x00};

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Processing commit message");

	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_process_commit failed - op is NULL");
		return -1;
	}

	if (!commit_data) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_process_commit failed - commit_data is NULL");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Processing commit data of %zu bytes", commit_len);

	/* Wire format is 98 bytes [group][scalar][element]; PSA expects 96 [scalar][element] */
	if (commit_len == WPA3_PSA_SAE_COMMIT_WIRE_LEN) {
		commit_data += 2;
		commit_len = WPA3_PSA_SAE_COMMIT_PSA_LEN;
	} else if (commit_len != WPA3_PSA_SAE_COMMIT_PSA_LEN) {
		wpa_printf(MSG_DEBUG,
			   "WPA3-PSA: Commit data wrong size (%zu bytes, expected %d or %d)",
			   commit_len, WPA3_PSA_SAE_COMMIT_PSA_LEN, WPA3_PSA_SAE_COMMIT_WIRE_LEN);
		return -1;
	}
	wpa_hexdump(MSG_DEBUG, "WPA3-PSA: Commit payload for PSA", commit_data, commit_len);

	if (commit_len > sizeof(op->peer_commit)) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_process_commit failed - commit data too large (%zu "
			   "> %zu)",
			   commit_len, sizeof(op->peer_commit));
		return -1;
	}

	os_memcpy(op->peer_commit, commit_data, commit_len);
	op->peer_commit_len = commit_len;

	/* Input the peer's commit message to PSA PAKE (96 bytes: scalar + element) */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Calling psa_pake_input for commit step");
	status = psa_pake_input(&op->pake_op, PSA_PAKE_STEP_COMMIT, commit_data, commit_len);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_process_commit failed - psa_pake_input returned %d",
			   status);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Commit message processed successfully");

	/* Update state to SAE_COMMITTED */
	op->state = SAE_COMMITTED;
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Operation state updated to SAE_COMMITTED");

	/* Set send-confirm counter (required before generating confirms) - as per working test */
	status = psa_pake_input(&op->pake_op, PSA_PAKE_STEP_CONFIRM_COUNT, send_confirm_counter, 2);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to set send_confirm counter: %d", status);
		return -1;
	}

	/* Derive confirm message (M2) */
	status = wpa3_psa_derive_confirm(op);
	if (status != 0) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_process_commit failed - "
				      "wpa3_psa_derive_confirm failed");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Confirm message derived successfully");
	return 0;
}

/**
 * wpa3_psa_derive_confirm - Derive SAE confirm message (M2) using PSA
 * @op: PSA operation structure
 *
 * Returns: 0 on success, -1 on failure
 */
static int wpa3_psa_derive_confirm(struct wpa3_psa_operation *op)
{
	psa_status_t status;
	u8 confirm_data[256]; /* Buffer for confirm message */
	size_t confirm_len = 0;

	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_derive_confirm failed - op is NULL");
		return -1;
	}

	/* Note: send_confirm is now set in wpa3_psa_process_commit before calling this function */

	/* Generate confirm message using PSA PAKE */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Calling psa_pake_output for confirm step");
	status = psa_pake_output(&op->pake_op, PSA_PAKE_STEP_CONFIRM, confirm_data,
				 sizeof(confirm_data), &confirm_len);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_derive_confirm failed - psa_pake_output returned %d",
			   status);
		return -1;
	}

	/* Store confirm data for later use */
	if (confirm_len > sizeof(op->shared_key)) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_derive_confirm failed - confirm data too large (%zu "
			   "> %zu)",
			   confirm_len, sizeof(op->shared_key));
		return -1;
	}

	os_memcpy(op->shared_key, confirm_data, confirm_len);
	op->shared_key_len = confirm_len;

	return 0;
}

/**
 * wpa3_psa_process_confirm - Process incoming confirm message using PSA
 * @op: PSA operation structure
 * @confirm_data: Incoming confirm data
 * @confirm_len: Length of confirm data
 *
 * Returns: 0 on success, -1 on failure
 */
static int wpa3_psa_process_confirm(struct wpa3_psa_operation *op, const u8 *confirm_data,
				    size_t confirm_len)
{
	psa_status_t status;

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Processing confirm message");

	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_process_confirm failed - op is NULL");
		return -1;
	}

	if (!confirm_data) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_process_confirm failed - confirm_data is NULL");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Processing confirm data of %zu bytes", confirm_len);

	/* Input the peer's confirm message to PSA PAKE */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Calling psa_pake_input for confirm step");
	status = psa_pake_input(&op->pake_op, PSA_PAKE_STEP_CONFIRM, confirm_data, confirm_len);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR,
			   "WPA3-PSA: wpa3_psa_process_confirm failed - psa_pake_input returned %d",
			   status);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Confirm message processed successfully");

	/* Derive final keys */
	wpa_printf(MSG_DEBUG, "WPA3-PSA: Deriving final keys");
	status = wpa3_psa_derive_keys(op);
	if (status != 0) {
		wpa_printf(
			MSG_ERROR,
			"WPA3-PSA: wpa3_psa_process_confirm failed - wpa3_psa_derive_keys failed");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPA3-PSA: Final keys derived successfully");
	return 0;
}

/**
 * wpa3_psa_derive_keys - Derive final keys using PSA
 * @op: PSA operation structure
 *
 * Returns: 0 on success, -1 on failure
 */
static int wpa3_psa_derive_keys(struct wpa3_psa_operation *op)
{
	psa_status_t status;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t shared_key_id = PSA_KEY_ID_NULL;
	size_t pmk_len;
	uint8_t *pmk_buf = NULL;
	int ret = -1;

	if (!op) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: wpa3_psa_derive_keys failed - op is NULL");
		return -1;
	}

	/* PSA PAKE returns PMK directly - no keyseed derivation needed */
	/* Key derivation is handled internally by PSA PAKE */

	/* Get the shared key from PSA PAKE */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_DERIVE);

	status = psa_pake_get_shared_key(&op->pake_op, &attributes, &shared_key_id);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to get shared key: %d", status);
		return -1;
	}

	/* Export the shared key - PSA PAKE returns PMK directly, not the intermediate k */
	pmk_buf = (uint8_t *)os_malloc(32);
	if (!pmk_buf) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to allocate memory for PMK");
		goto cleanup;
	}

	status = psa_export_key(shared_key_id, pmk_buf, 32, &pmk_len);
	if (status != PSA_SUCCESS) {
		wpa_printf(MSG_ERROR, "WPA3-PSA: Failed to export shared key (PMK): %d", status);
		goto cleanup;
	}

	/* PSA PAKE shared key is already the PMK, use it directly */
	os_memcpy(op->pmk, pmk_buf, pmk_len);
	op->pmk_len = pmk_len;

	ret = 0;

cleanup:
	if (shared_key_id != PSA_KEY_ID_NULL) {
		psa_destroy_key(shared_key_id);
	}
	if (pmk_buf) {
		os_free(pmk_buf);
	}

	if (ret == 0) {
		wpa_printf(MSG_DEBUG, "WPA3-PSA: Keys derived successfully");
	}

	return ret;
}

/**
 * wpa3_psa_cleanup_operation - Clean up PSA operation resources
 */
static void wpa3_psa_cleanup_operation(struct wpa3_psa_operation *op)
{
	if (!op) {
		return;
	}

	/* Abort PAKE operation */
	psa_pake_abort(&op->pake_op);

	/* Destroy password key if it exists */
	if (op->password_key != PSA_KEY_ID_NULL) {
		psa_destroy_key(op->password_key);
		op->password_key = PSA_KEY_ID_NULL;
	}

	/* Clear sensitive data */
	os_memset(op->password, 0, sizeof(op->password));
	os_memset(op->shared_key, 0, sizeof(op->shared_key));
	os_memset(op->pmk, 0, sizeof(op->pmk));
	os_memset(op->kck, 0, sizeof(op->kck));
}
