/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_state, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>

#include "fp_activation.h"
#include "fp_fmdn_battery.h"
#include "fp_fmdn_callbacks.h"
#include "fp_fmdn_clock.h"
#include "fp_fmdn_state.h"
#include "fp_crypto.h"
#include "fp_storage_eik.h"

#include "dult.h"
#include "fp_fmdn_dult_integration.h"

/* Byte length and offset of fields used to generate the FMDN frame. */
#define FMDN_FRAME_UUID_LEN            2
#define FMDN_FRAME_TYPE_OFFSET         (0 + FMDN_FRAME_UUID_LEN)
#define FMDN_FRAME_TYPE_LEN            1
#define FMDN_FRAME_EID_OFFSET          (FMDN_FRAME_TYPE_OFFSET + FMDN_FRAME_TYPE_LEN)
#define FMDN_FRAME_EID_LEN             FP_FMDN_STATE_EID_LEN
#define FMDN_FRAME_HASHED_FLAGS_OFFSET (FMDN_FRAME_EID_OFFSET + FMDN_FRAME_EID_LEN)
#define FMDN_FRAME_HASHED_FLAGS_LEN    1
#define FMDN_FRAME_PAYLOAD_LEN \
	(FMDN_FRAME_HASHED_FLAGS_OFFSET + FMDN_FRAME_HASHED_FLAGS_LEN)

/* Hashed flags bitmask */
#define FMDN_FRAME_HASHED_FLAGS_UTP_BIT_OFFSET     0
#define FMDN_FRAME_HASHED_FLAGS_UTP_BIT_LEN        1
#define FMDN_FRAME_HASHED_FLAGS_BATTERY_BIT_OFFSET 1
#define FMDN_FRAME_HASHED_FLAGS_BATTERY_BIT_LEN    2

/* Constants used in the FMDN advertising frame. */
#define FMDN_FRAME_UUID 0xFEAA

/* FMDN Frame type with Unwanted Tracking Protection Mode indication. */
#define FMDN_FRAME_TYPE_UTP_MODE_OFF 0x40
#define FMDN_FRAME_TYPE_UTP_MODE_ON  0x41

/* Byte length and offset of fields used to generate a seed for Ephemeral Identifier. */
#define FMDN_EID_SEED_PADDING_LEN        11
#define FMDN_EID_SEED_ROT_PERIOD_EXP_LEN 1
#define FMDN_EID_SEED_FMDN_CLOCK_LEN     sizeof(uint32_t)
#define FMDN_EID_SEED_LEN                    \
	((FMDN_EID_SEED_PADDING_LEN +        \
	  FMDN_EID_SEED_ROT_PERIOD_EXP_LEN + \
	  FMDN_EID_SEED_FMDN_CLOCK_LEN) * 2)

/* Constants used to generate a seed for Ephemeral Identifier. */
#define FMDN_EID_SEED_ROT_PERIOD_EXP   10
#define FMDN_EID_SEED_PADDING_TYPE_ONE 0xFF
#define FMDN_EID_SEED_PADDING_TYPE_TWO 0x00

/* Limits in seconds to the EID rotation period randomness as recommended by the specification. */
#define FMDN_EID_ROT_PERIOD_RAND_LOWER_LIMIT 1
#define FMDN_EID_ROT_PERIOD_RAND_UPPER_LIMIT 204
#define FMDN_EID_ROT_PERIOD_RAND_LIMIT_DIFF     \
	(FMDN_EID_ROT_PERIOD_RAND_UPPER_LIMIT - \
	FMDN_EID_ROT_PERIOD_RAND_LOWER_LIMIT)

/* Maximum number of FMDN connection slots. */
#define FMDN_MAX_CONN (CONFIG_BT_FAST_PAIR_FMDN_MAX_CONN)

/* FMDN TX power settings. */
#define FMDN_TX_POWER                (CONFIG_BT_FAST_PAIR_FMDN_TX_POWER)
#define FMDN_TX_POWER_CORRECTION_VAL (CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL)
#define FMDN_TX_POWER_CALIBRATED_MIN (-100)
#define FMDN_TX_POWER_CALIBRATED_MAX (20)

/* Constants used in Elliptic Curve calculation. */
#define SECP_MOD_RES_LEN FP_FMDN_STATE_EID_LEN

/* Constants used for Unwanted Tracking Protection mode. */
#define UTP_EID_ROTATIONS_PER_RPA_ROTATION 85 /* 85 * 1024s = 87040s ~ 1451m ~ 24h11m */

/* Bit numbers of Control Flags in the Unwanted Tracking Protection mode. */
enum utp_control_flags_bit_num {
	UTP_CONTROL_FLAGS_BIT_NUM_RING_AUTH_SKIP = 0,
};

/* Verify if the length of the EIK is consistent with the storage module. */
BUILD_ASSERT(FP_FMDN_STATE_EIK_LEN == FP_STORAGE_EIK_LEN);

/* Reserve at least two connection slots for FMDN connections and advertising. */
BUILD_ASSERT(CONFIG_BT_MAX_CONN > FMDN_MAX_CONN);

/* Validate the Elliptic Curve configuration. */
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP256R1) ||
	     IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP160R1));
BUILD_ASSERT((SECP_MOD_RES_LEN == FP_CRYPTO_ECC_SECP160R1_MOD_LEN) ||
	     (SECP_MOD_RES_LEN == FP_CRYPTO_ECC_SECP256R1_MOD_LEN));

static uint8_t fmdn_frame_payload[FMDN_FRAME_PAYLOAD_LEN] = {
	BT_UUID_16_ENCODE(FMDN_FRAME_UUID), FMDN_FRAME_TYPE_UTP_MODE_OFF,
};

static const struct bt_data fmdn_frame_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_SVC_DATA16, fmdn_frame_payload, sizeof(fmdn_frame_payload)),
};

static struct bt_le_ext_adv *fmdn_adv_set;
static struct bt_le_adv_param fmdn_adv_param = {
	.id = BT_ID_DEFAULT,
	.options =
		/* Enable Extended Advertising for the SECP256R1 ECC variant. */
		(IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP256R1) * BT_LE_ADV_OPT_EXT_ADV) |
		BT_LE_ADV_OPT_CONNECTABLE,
};
static int8_t fmdn_adv_set_tx_power;

static uint8_t * const fmdn_frame_type = (fmdn_frame_payload + FMDN_FRAME_TYPE_OFFSET);
static uint8_t * const fmdn_frame_hashed_flags =
	(fmdn_frame_payload + FMDN_FRAME_HASHED_FLAGS_OFFSET);
static uint8_t fmdn_frame_hashed_flags_xor_operand;

static uint8_t * const fmdn_eid = (fmdn_frame_payload + FMDN_FRAME_EID_OFFSET);
static uint32_t fmdn_eid_clock_checkpoint;

static bool utp_mode;
static bool utp_mode_rpa_change_request;
static uint8_t utp_mode_control_flags;
static uint32_t utp_rotations; /* Number of EID rotations in each RPA rotation cycle */

static bool is_enabled;

static uint32_t fmdn_conn_cnt;
static bool fmdn_conns[CONFIG_BT_MAX_CONN];

static void fmdn_disconnected_work_handle(struct k_work *work);
static void fmdn_post_init_work_handle(struct k_work *work);

static K_WORK_DEFINE(fmdn_disconnected_work, fmdn_disconnected_work_handle);
static K_WORK_DEFINE(fmdn_post_init_work, fmdn_post_init_work_handle);

static int fmdn_adv_start(void);

static void eid_seed_half_encode(struct net_buf_simple *buf,
				 uint8_t padding_pattern,
				 uint32_t fmdn_clock)
{
	uint8_t padding[FMDN_EID_SEED_PADDING_LEN];

	memset(padding, padding_pattern, sizeof(padding));

	net_buf_simple_add_mem(buf, padding, sizeof(padding));
	net_buf_simple_add_u8(buf, FMDN_EID_SEED_ROT_PERIOD_EXP);
	net_buf_simple_add_be32(buf, fmdn_clock);
}

static int eid_encode(void)
{
	int err;
	uint32_t fmdn_clock;
	uint8_t eik[FP_STORAGE_EIK_LEN];
	uint8_t encrypted_eid_seed[FP_CRYPTO_AES256_BLOCK_LEN];
	const uint8_t uninitialized_eid[FP_FMDN_STATE_EID_LEN] = {};
	uint8_t secp_mod_res[SECP_MOD_RES_LEN];
	uint8_t mod_res_hash[FP_CRYPTO_SHA256_HASH_LEN];

	NET_BUF_SIMPLE_DEFINE(eid_seed_buf, FMDN_EID_SEED_LEN);

	/* Prepare the FMDN Clock value. */
	fmdn_clock = fp_fmdn_clock_read();

	/* Clear the K lowest bits in the clock value. */
	fmdn_clock &= ~BIT_MASK(FMDN_EID_SEED_ROT_PERIOD_EXP);

	/* Check if the EID seed or EIK has changed since the last call. */
	if (memcmp(fmdn_eid, uninitialized_eid, sizeof(uninitialized_eid)) != 0) {
		if (fmdn_clock == fmdn_eid_clock_checkpoint) {
			LOG_DBG("FMDN State: EID does not require recalculation");

			return 0;
		}
	}
	fmdn_eid_clock_checkpoint = fmdn_clock;

	/* Prepare the EID seed data. */
	eid_seed_half_encode(&eid_seed_buf,
			     FMDN_EID_SEED_PADDING_TYPE_ONE,
			     fmdn_clock);
	eid_seed_half_encode(&eid_seed_buf,
			     FMDN_EID_SEED_PADDING_TYPE_TWO,
			     fmdn_clock);

	/* Load the EIK. */
	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("FMDN State: fp_storage_eik_get failed: %d", err);

		return err;
	}

	LOG_HEXDUMP_DBG(eid_seed_buf.data, eid_seed_buf.len, "EID seed data:");
	LOG_HEXDUMP_DBG(eik, sizeof(eik), "EIK:");

	/* Encrypt the EID seed data with the Ephemeral Identity Key
	 * using the AES-ECB-256 scheme.
	 */
	err = fp_crypto_aes256_ecb_encrypt(encrypted_eid_seed, eid_seed_buf.data, eik);
	if (err) {
		LOG_ERR("FMDN State: EID seed data encryption failed: %d", err);

		return err;
	}

	LOG_HEXDUMP_DBG(encrypted_eid_seed,
			sizeof(encrypted_eid_seed),
			"Encrypted EID seed data:");

	/* Calculate the EID as the x coordinate of a point on the elliptic curve. */
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP160R1)) {
		err = fp_crypto_ecc_secp160r1_calculate(fmdn_eid,
							secp_mod_res,
							encrypted_eid_seed,
							sizeof(encrypted_eid_seed));
		if (err) {
			LOG_ERR("FMDN State: EID calculation using secp160r1 failed: %d",
				err);

			return err;
		}
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP256R1)) {
		err = fp_crypto_ecc_secp256r1_calculate(fmdn_eid,
							secp_mod_res,
							encrypted_eid_seed,
							sizeof(encrypted_eid_seed));
		if (err) {
			LOG_ERR("FMDN State: EID calculation using secp256r1 failed: %d",
				err);

			return err;
		}
	} else {
		__ASSERT(0, "ECC selection not supported");
	}

	LOG_HEXDUMP_DBG(fmdn_eid, FP_FMDN_STATE_EID_LEN, "EID:");

	/* Calculate the XOR operand for the Hashed Flags bitmask. */
	err = fp_crypto_sha256(mod_res_hash, secp_mod_res, sizeof(secp_mod_res));
	if (err) {
		LOG_ERR("FMDN State: secp modulo result hashing failed: %d", err);

		return err;
	}

	fmdn_frame_hashed_flags_xor_operand = mod_res_hash[sizeof(mod_res_hash) - 1];

	return 0;
}

static uint8_t frame_type_encode(void)
{
	uint8_t frame_type_byte;

	frame_type_byte = utp_mode ?
		FMDN_FRAME_TYPE_UTP_MODE_ON :
		FMDN_FRAME_TYPE_UTP_MODE_OFF;

	LOG_DBG("FMDN State: FMDN Frame type: 0x%02X", frame_type_byte);

	return frame_type_byte;
}

static uint8_t hashed_flags_encode(void)
{
	uint8_t hashed_flags_seed;
	uint8_t hashed_flags_byte;
	enum fp_fmdn_battery_level battery_level;

	hashed_flags_seed = 0;
	battery_level = fp_fmdn_battery_level_get();
	WRITE_BIT(hashed_flags_seed, FMDN_FRAME_HASHED_FLAGS_UTP_BIT_OFFSET, utp_mode);
	hashed_flags_seed |= (battery_level << FMDN_FRAME_HASHED_FLAGS_BATTERY_BIT_OFFSET);

	hashed_flags_byte = (hashed_flags_seed ^ fmdn_frame_hashed_flags_xor_operand);

	LOG_DBG("FMDN State: Hashed Flags byte:");
	LOG_DBG("\tBitmask:\t0x%02X", hashed_flags_seed);
	LOG_DBG("\tHash:\t\t0x%02X", fmdn_frame_hashed_flags_xor_operand);
	LOG_DBG("\tFinal value:\t0x%02X", hashed_flags_byte);

	return hashed_flags_byte;
}

static int fmdn_adv_data_async_update(void)
{
	int err;

	if (!fmdn_adv_set) {
		/* FMDN advertising set is not active. */
		return 0;
	}

	/* Verify that the frame type does not change during async update. */
	__ASSERT(*fmdn_frame_type == frame_type_encode(),
		"FMDN State: frame type should not change during async update");

	/* Prepare the Hashed Flags byte in the new advertising payload. */
	*fmdn_frame_hashed_flags = hashed_flags_encode();

	/* Set the new advertising payload in the Bluetooth stack. */
	err = bt_le_ext_adv_set_data(fmdn_adv_set,
				     fmdn_frame_data,
				     ARRAY_SIZE(fmdn_frame_data),
				     NULL,
				     0);
	if (err) {
		LOG_ERR("FMDN State: bt_le_ext_adv_set_data returned error: %d", err);
		return err;
	}

	LOG_DBG("FMDN State: updating the Hashed Flags byte in advertising payload");

	return 0;
}

static int fmdn_adv_payload_generate(void)
{
	int err;

	err = eid_encode();
	if (err) {
		LOG_ERR("FMDN State: eid_encode returned error: %d", err);
		return err;
	}

	/* Prepare the Hashed Flags byte in the new advertising payload. */
	*fmdn_frame_hashed_flags = hashed_flags_encode();

	/* Set the FMDN frame type with the updated UTP Mode indication. */
	*fmdn_frame_type = frame_type_encode();

	return 0;
}

static uint16_t fmdn_adv_rpa_timeout_calculate(void)
{
	int err;
	uint8_t rand_rotation_time_seed;
	uint32_t rand_rotation_time;
	uint16_t non_rand_rotation_time;
	uint32_t fmdn_clock;

	/* Calculate non-random part as the next anticipated rotation time. */
	fmdn_clock = fp_fmdn_clock_read();
	non_rand_rotation_time = BIT(FMDN_EID_SEED_ROT_PERIOD_EXP);
	non_rand_rotation_time -= fmdn_clock % BIT(FMDN_EID_SEED_ROT_PERIOD_EXP);

	/* Calculate the positive randomized time factor. */
	err = sys_csrand_get(&rand_rotation_time_seed, sizeof(rand_rotation_time_seed));
	if (err) {
		LOG_WRN("FMDN State: sys_csrand_get failed: %d", err);

		sys_rand_get(&rand_rotation_time_seed, sizeof(rand_rotation_time_seed));
	}

	BUILD_ASSERT(FMDN_EID_ROT_PERIOD_RAND_LIMIT_DIFF < UINT8_MAX);

	/* Convert the random part range from <0; UINT8_MAX> to
	 * <FMDN_EID_ROT_PERIOD_RAND_LOWER_LIMIT;
	 *  FMDN_EID_ROT_PERIOD_RAND_UPPER_LIMIT>
	 */
	rand_rotation_time = FMDN_EID_ROT_PERIOD_RAND_LIMIT_DIFF;
	rand_rotation_time *= rand_rotation_time_seed;
	rand_rotation_time /= UINT8_MAX;
	rand_rotation_time += FMDN_EID_ROT_PERIOD_RAND_LOWER_LIMIT;

	__ASSERT((non_rand_rotation_time + rand_rotation_time) < UINT16_MAX,
		"FMDN State: Invalid RPA timeout calculation");

	LOG_DBG("FMDN State: calculating new rotation time: %d s + %d s",
		non_rand_rotation_time, rand_rotation_time);

	return (non_rand_rotation_time + rand_rotation_time);
}

static bool fmdn_adv_rpa_expired_state_transition(void)
{
	bool rotate_rpa;

	if (fmdn_conn_cnt == FMDN_MAX_CONN) {
		LOG_DBG("FMDN State: RPA locked on disabled advertising");

		/* Return false to the Bluetooth rpa_expired callback API
		 * to maintain valid RPA status in the Bluetooth stack that
		 * ensures that this callback will be triggerred periodically
		 * on each RPA expiry timeout. This approach guarantees that
		 * this module will drive rotation of remaining advertising
		 * sets in the FMDN provisioned state even if the FMDN
		 * advertising is inactive (e.g. when maintaining a maximum
		 * number of connections).
		 */
		return false;
	}

	if (utp_mode_rpa_change_request) {
		utp_mode_rpa_change_request = false;

		return true;
	}

	rotate_rpa = true;

	if (utp_mode) {
		utp_rotations++;

		if (utp_rotations == UTP_EID_ROTATIONS_PER_RPA_ROTATION) {
			utp_rotations = 0;

			LOG_DBG("FMDN State: UTP mode: executing daily RPA rotation");
		} else {
			rotate_rpa = false;

			LOG_DBG("FMDN State: UTP mode: RPA locked for this EID rotation");
		}
	}

	return rotate_rpa;
}

static bool fmdn_adv_rpa_expired(struct bt_le_ext_adv *adv)
{
	int err;
	bool rotate_rpa;
	uint16_t next_rpa_timeout;

	BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_RPA_SHARING),
		     "RPA sharing is not supported when combined with the FMDN extension");

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!fp_fmdn_state_is_provisioned()) {
		/* Keep the RPA in the valid state to ensure that the RPA expired callback will be
		 * received on a forced RPA rotation during the FMDN provisioning operation (and
		 * just before the start of the FMDN advertising). The RPA expired callback will
		 * not be received if RPA rotation was previously allowed here in the unprovisioned
		 * state (no expired callback for RPAs already marked as invalid).
		 */
		return false;
	}

	LOG_DBG("FMDN State: RPA expired");

	/* Handle state changes related to the RPA rotation. */
	rotate_rpa = fmdn_adv_rpa_expired_state_transition();

	/* Randomize the next rotation period on the RPA timeout. */
	next_rpa_timeout = fmdn_adv_rpa_timeout_calculate();
	err = bt_le_set_rpa_timeout(next_rpa_timeout);
	if (err) {
		LOG_ERR("FMDN State: bt_set_rpa_timeout failed: %d", err);
	}

	/* Prepare the advertising payload for the FMDN frame. */
	err = fmdn_adv_payload_generate();
	if (err) {
		LOG_ERR("FMDN State: generation of advertising payload failed: %d", err);
	}

	/* Update the advertising payload with the new EID. */
	err = bt_le_ext_adv_set_data(fmdn_adv_set,
				     fmdn_frame_data,
				     ARRAY_SIZE(fmdn_frame_data),
				     NULL,
				     0);
	if (err) {
		LOG_ERR("FMDN State: bt_le_ext_adv_set_data returned error: %d", err);
	}

	LOG_DBG("FMDN State: rotating advertising payload");
	LOG_DBG("FMDN State: next rotation in %d [s]", next_rpa_timeout);

	return rotate_rpa;
}

static void fmdn_adv_connected(struct bt_le_ext_adv *adv,
			       struct bt_le_ext_adv_connected_info *info)
{
	int err;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	LOG_DBG("FMDN State: peer connected");

	fmdn_conns[bt_conn_index(info->conn)] = true;
	fmdn_conn_cnt++;

	if (!fp_fmdn_state_is_provisioned()) {
		return;
	}

	if (fmdn_conn_cnt < FMDN_MAX_CONN) {
		err = fmdn_adv_start();
		if (err) {
			LOG_ERR("FMDN State: failed to start advertising: %d", err);
		}
	}
}

static int fmdn_adv_stop(void)
{
	int err;

	__ASSERT(fmdn_adv_set != NULL,
		"FMDN State: invalid state of the advertising set");

	err = bt_le_ext_adv_stop(fmdn_adv_set);
	if (err) {
		LOG_ERR("bt_le_ext_adv_stop returned error: %d", err);
		return err;
	}

	LOG_DBG("FMDN State: advertising stopped");

	return 0;
}

static int fmdn_adv_start(void)
{
	int err;
	struct bt_le_ext_adv_start_param ext_adv_start_param = {0};

	__ASSERT(fmdn_adv_set != NULL,
		"FMDN State: invalid state of the advertising set");

	/* Check if connection limit is not exceeded. */
	if (fmdn_conn_cnt >= FMDN_MAX_CONN) {
		LOG_DBG("FMDN State: unable to resume advertising: no free connection slots");
		return -ENOMEM;
	}

	err = bt_le_ext_adv_start(fmdn_adv_set, &ext_adv_start_param);
	if (err && (err != -EALREADY)) {
		LOG_ERR("bt_le_ext_adv_start returned error: %d", err);
		return err;
	}

	LOG_DBG("FMDN State: advertising %s",
		(err == -EALREADY) ? "is still active" : "started");

	return 0;
}

static int fmdn_tx_power_set(uint16_t handle, int8_t *tx_power)
{
	int err;
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct bt_hci_rp_vs_write_tx_power_level *rp;
	struct net_buf *buf;
	struct net_buf *rsp = NULL;

	if (!tx_power) {
		LOG_ERR("FMDN State: no TX power param");
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, sizeof(*cp));
	if (!buf) {
		LOG_ERR("FMDN State: cannot allocate buffer to set TX power");
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = BT_HCI_VS_LL_HANDLE_TYPE_ADV;
	cp->tx_power_level = *tx_power;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, buf, &rsp);
	if (err) {
		LOG_ERR("FMDN State: cannot set TX power (err: %d)", err);
		return err;
	}

	rp = (struct bt_hci_rp_vs_write_tx_power_level *) rsp->data;
	*tx_power = rp->selected_tx_power;

	net_buf_unref(rsp);

	return err;
}

static int fmdn_adv_set_rotate(void)
{
	int err;
	struct bt_le_oob oob;

	__ASSERT(fmdn_adv_set != NULL,
		"FMDN State: invalid state of the advertising set");

	/* Force the RPA rotation to synchronize the Fast Pair advertising
	 * with FMDN advertising using rpa_expired callback.
	 */
	err = bt_le_oob_get_local(fmdn_adv_param.id, &oob);
	if (err) {
		LOG_ERR("FMDN State: bt_le_oob_get_local failed: %d", err);
		return err;
	}

	LOG_DBG("FMDN State: forcing advertising payload rotation");

	return 0;
}

static int fmdn_adv_set_teardown(void)
{
	int err;

	__ASSERT(fmdn_adv_set != NULL,
		"FMDN State: invalid state of the advertising set");

	err = bt_le_ext_adv_delete(fmdn_adv_set);
	if (err) {
		LOG_ERR("FMDN State: bt_le_ext_adv_delete returned error: %d", err);
		return err;
	}

	fmdn_adv_set = NULL;

	/* Ensure that advertising would not be restarted when no longer initialized. */
	(void) k_work_cancel(&fmdn_disconnected_work);

	return 0;
}

static int fmdn_adv_set_setup(void)
{
	int err;
	uint8_t adv_handle;
	int8_t tx_power;
	static struct bt_le_ext_adv_cb fmdn_adv_set_cb = {
		.connected = fmdn_adv_connected,
		.rpa_expired = fmdn_adv_rpa_expired,
	};

	__ASSERT(fmdn_adv_set == NULL,
		"FMDN State: invalid state of the advertising set");

	/* Create the FMDN advertising set. */
	err = bt_le_ext_adv_create(&fmdn_adv_param,
				   &fmdn_adv_set_cb,
				   &fmdn_adv_set);
	if (err) {
		LOG_ERR("FMDN State: bt_le_ext_adv_create returned error: %d", err);

		/* Additional context for specific error codes. */
		if (err == -ENOMEM) {
			LOG_ERR("FMDN State: bt_le_ext_adv_create returning -ENOMEM (%d) is "
				"typically caused by incorrect advertising set memory allocation "
				"or incorrect CONFIG_BT_EXT_ADV_MAX_ADV_SET configuration", err);
		} else if (err == -EINVAL) {
			LOG_ERR("FMDN State: bt_le_ext_adv_create returning -EINVAL (%d) is "
				"typically caused by incorrect advertising parameters "
				"configuration that is set in the bt_fast_pair_fmdn_adv_param_set "
				"API function", err);
		}

		return err;
	}

	/* Set the TX power for FMDN advertising and connections. */
	err = bt_hci_get_adv_handle(fmdn_adv_set, &adv_handle);
	if (err) {
		LOG_ERR("FMDN State: bt_hci_get_adv_handle returned error: %d", err);
		(void) fmdn_adv_set_teardown();
		return err;
	}

	tx_power = FMDN_TX_POWER;
	err = fmdn_tx_power_set(adv_handle, &tx_power);
	if (err) {
		LOG_ERR("FMDN State: fmdn_tx_power_set returned error: %d", err);
		(void) fmdn_adv_set_teardown();
		return err;
	}

	if (tx_power != FMDN_TX_POWER) {
		LOG_WRN("FMDN State: the desired TX power configuration of %d dBm"
			" is different from the actual TX power of %d dBm"
			" due to the \"%s\" board limitations",
			FMDN_TX_POWER, tx_power, CONFIG_BOARD);
	} else {
		LOG_DBG("FMDN State: TX power set to %d [dBm]", tx_power);
	}

	fmdn_adv_set_tx_power = tx_power;

	return 0;
}

static void fmdn_disconnected_work_handle(struct k_work *work)
{
	int err;

	err = fmdn_adv_set_rotate();
	if (err) {
		LOG_ERR("FMDN State: fmdn_adv_set_rotate failed: %d", err);
		return;
	}

	err = fmdn_adv_start();
	if (err) {
		LOG_ERR("FMDN State: failed to start advertising: %d", err);
	}
}

static void fmdn_disconnected(struct bt_conn *conn, uint8_t reason)
{
	bool max_conn = false;

	if (!bt_fast_pair_is_ready()) {
		return;
	}

	if (!fmdn_conns[bt_conn_index(conn)]) {
		return;
	}

	if (fmdn_conn_cnt == FMDN_MAX_CONN) {
		max_conn = true;
	}

	fmdn_conns[bt_conn_index(conn)] = false;
	fmdn_conn_cnt--;

	if (!fp_fmdn_state_is_provisioned()) {
		/* FMDN is unprovisioned. */
		return;
	}

	if (max_conn) {
		k_work_submit(&fmdn_disconnected_work);
	}
}

BT_CONN_CB_DEFINE(fmdn_conn_callbacks) = {
	.disconnected = fmdn_disconnected,
};

static enum dult_near_owner_state_mode fmdn_utp_to_dult_near_owner_map(bool utp_active)
{
	return utp_active ? DULT_NEAR_OWNER_STATE_MODE_SEPARATED :
			    DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;
}

static void fmdn_utp_mode_state_reset(void)
{
	utp_mode = false;
	utp_mode_rpa_change_request = false;
	utp_rotations = 0;
	utp_mode_control_flags = 0;

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT)) {
		int err;

		err = dult_near_owner_state_set(fp_fmdn_dult_integration_user_get(),
						fmdn_utp_to_dult_near_owner_map(utp_mode));
		__ASSERT_NO_MSG(!err);
	}
}

static int fmdn_utp_mode_state_set(bool activated, uint8_t *control_flags)
{
	int err;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!fp_fmdn_state_is_provisioned()) {
		return -EINVAL;
	}

	if (!activated) {
		__ASSERT(!control_flags, "Control Flags param should be NULL");

		utp_rotations = 0;
		utp_mode_control_flags = 0;
	} else {
		__ASSERT(control_flags, "Control Flags param should not be NULL");

		/* Validate Control Flags field. */
		const uint8_t control_flags_mask =
			BIT(UTP_CONTROL_FLAGS_BIT_NUM_RING_AUTH_SKIP);

		if ((*control_flags & ~control_flags_mask) != 0) {
			return -EINVAL;
		}

		utp_mode_control_flags = *control_flags;
	}

	if (utp_mode == activated) {
		return 0;
	}

	utp_mode = activated;
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT)) {
		int err;

		err = dult_near_owner_state_set(fp_fmdn_dult_integration_user_get(),
						fmdn_utp_to_dult_near_owner_map(utp_mode));
		__ASSERT_NO_MSG(!err);
	}
	utp_mode_rpa_change_request = true;

	/* Detecting Unwanted Location Trackers (DULT) specification:
	 * 3.5.1 Rotation Policy:
	 * An accessory SHALL rotate its address on any transition from near-owner
	 * state to separated state as well as any transition from separated state
	 * to near-owner state.
	 *
	 * Guidelines specific to FMDN to be compliant with DULT spec:
	 * "Unwanted tracking protection mode" defined in this document maps to the
	 * "separated state" defined by the DULT spec.
	 */
	err = fmdn_adv_set_rotate();
	if (err) {
		LOG_ERR("FMDN State: fmdn_adv_set_rotate failed: %d", err);
		return err;
	}

	return 0;
}

int fp_fmdn_state_utp_mode_activate(uint8_t control_flags)
{
	return fmdn_utp_mode_state_set(true, &control_flags);
}

int fp_fmdn_state_utp_mode_deactivate(void)
{
	return fmdn_utp_mode_state_set(false, NULL);
}

bool fp_fmdn_state_utp_mode_ring_auth_skip(void)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!fp_fmdn_state_is_provisioned()) {
		return false;
	}

	if (!utp_mode) {
		return false;
	}

	return (utp_mode_control_flags & BIT(UTP_CONTROL_FLAGS_BIT_NUM_RING_AUTH_SKIP));
}

int fp_fmdn_state_eid_read(uint8_t *eid)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!fp_fmdn_state_is_provisioned()) {
		return -EINVAL;
	}

	memcpy(eid, fmdn_eid, FP_FMDN_STATE_EID_LEN);

	return 0;
}

int fp_fmdn_state_eik_read(uint8_t *eik)
{
	int err;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!fp_fmdn_state_is_provisioned()) {
		return -EINVAL;
	}

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("FMDN State: fp_storage_eik_get failed: %d", err);

		return err;
	}

	return 0;
}

uint8_t fp_fmdn_state_ecc_type_encode(void)
{
	/* Define the encoding for the ECC configuration. */
	enum ecc_type {
		ECC_TYPE_SECP160R1 = 0x00,
		ECC_TYPE_SECP256R1 = 0x01,
		ECC_TYPE_INVALID   = 0xFF,
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP160R1)) {
		return ECC_TYPE_SECP160R1;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_ECC_SECP256R1)) {
		return ECC_TYPE_SECP256R1;
	}

	__ASSERT(0, "FMDN State: incorrect ECC type selection");

	return ECC_TYPE_INVALID;
}

int8_t fp_fmdn_state_tx_power_encode(void)
{
	int32_t calibrated_tx_power;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	calibrated_tx_power = fmdn_adv_set_tx_power;
	calibrated_tx_power += FMDN_TX_POWER_CORRECTION_VAL;

	__ASSERT(calibrated_tx_power >= FMDN_TX_POWER_CALIBRATED_MIN,
		 "FMDN State: calibrated TX power is too low");
	__ASSERT(calibrated_tx_power <= FMDN_TX_POWER_CALIBRATED_MAX,
		 "FMDN State: calibrated TX power is too high");

	return calibrated_tx_power;
}

bool fp_fmdn_state_is_provisioned(void)
{
	int ret;

	ret = fp_storage_eik_is_provisioned();
	__ASSERT_NO_MSG(ret >= 0);

	return (ret > 0);
}

static void fmdn_conn_uninit_iterator(struct bt_conn *conn, void *user_data)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info conn_info;

	/* Validate the FMDN peer. */
	err = bt_conn_get_info(conn, &conn_info);
	__ASSERT_NO_MSG(!err);

	if ((conn_info.state != BT_CONN_STATE_CONNECTED) ||
	    !fmdn_conns[bt_conn_index(conn)]) {
		return;
	}

	/* Disconnect the FMDN peer. */
	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		LOG_ERR("FMDN State: bt_conn_disconnect returned error: %d", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("FMDN State: disconnecting peer: %s", addr);
}

static void fmdn_conn_state_reset(void)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, fmdn_conn_uninit_iterator, NULL);

	memset(fmdn_conns, 0, sizeof(fmdn_conns));
	fmdn_conn_cnt = 0;
}

static int fmdn_storage_unprovision(void)
{
	int err;

	err = fp_storage_eik_delete();
	if (err) {
		LOG_ERR("FMDN State: fp_storage_eik_delete failed: %d", err);
		return err;
	}

	memset(fmdn_eid, 0, FP_FMDN_STATE_EID_LEN);

	return 0;
}

static int fmdn_unprovision(void)
{
	int err;

	if (!fp_fmdn_state_is_provisioned()) {
		/* Ignore the request and stay in the unprovisioned state. */
		return 0;
	}

	err = fmdn_adv_stop();
	if (err) {
		LOG_ERR("FMDN State: fmdn_adv_stop failed: %d", err);
		return err;
	}

	fmdn_utp_mode_state_reset();

	err = fmdn_storage_unprovision();
	if (err) {
		LOG_ERR("FMDN State: fmdn_storage_unprovision failed: %d", err);
		return err;
	}

	fp_fmdn_callbacks_provisioning_state_changed_notify(false);

	LOG_DBG("FMDN State: Successful unprovisioning of the EIK");

	return 0;
}

static int fmdn_storage_provision(const uint8_t *eik)
{
	int err;

	err = fp_storage_eik_save(eik);
	if (err) {
		LOG_ERR("FMDN State: fp_storage_eik_save failed: %d", err);
		return err;
	}

	memset(fmdn_eid, 0, FP_FMDN_STATE_EID_LEN);

	return 0;
}

static int fmdn_reprovision(void)
{
	int err;

	/* Reprovision operation */
	err = fmdn_adv_payload_generate();
	if (err) {
		LOG_ERR("FMDN State: generation of advertising payload failed: %d", err);
		return err;
	}

	/* Update the advertising payload with the new EID. */
	err = bt_le_ext_adv_set_data(fmdn_adv_set,
				     fmdn_frame_data,
				     ARRAY_SIZE(fmdn_frame_data),
				     NULL,
				     0);
	if (err) {
		LOG_ERR("FMDN State: bt_le_ext_adv_set_data returned error: %d", err);
		return err;
	}

	LOG_DBG("FMDN State: Successful reprovisioning of the EIK");

	return 0;
}

static int fmdn_new_provision(void)
{
	int err;

	/* Provision operation */
	fp_fmdn_callbacks_provisioning_state_changed_notify(true);

	err = fmdn_adv_set_rotate();
	if (err) {
		LOG_ERR("FMDN State: fmdn_adv_set_rotate failed: %d", err);
		return err;
	}

	err = fmdn_adv_start();
	if (err) {
		LOG_ERR("FMDN State: fmdn_adv_start failed: %d", err);
		return err;
	}

	LOG_DBG("FMDN State: Successful provisioning of the EIK");

	return 0;
}

static int fmdn_provision(const uint8_t *eik)
{
	int err;
	bool was_provisioned = fp_fmdn_state_is_provisioned();

	__ASSERT_NO_MSG(eik);

	/* Refresh the existing EIK or store the new one. */
	err = fmdn_storage_provision(eik);
	if (err) {
		LOG_ERR("FMDN State: fmdn_storage_provision failed: %d", err);
		return err;
	}

	if (was_provisioned) {
		return fmdn_reprovision();
	} else {
		return fmdn_new_provision();
	}
}

int fp_fmdn_state_eik_provision(const uint8_t *eik)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!eik) {
		return fmdn_unprovision();
	} else {
		return fmdn_provision(eik);
	}
}

int bt_fast_pair_fmdn_adv_param_set(
	const struct bt_fast_pair_fmdn_adv_param *adv_param)
{
	int err = 0;
	bool new_param_valid = true;

	if (fmdn_adv_set) {
		/* Advertising Set is used:
		 * update parameters and restore the advertising state.
		 */
		struct bt_le_adv_param new_param;

		memcpy(&new_param, &fmdn_adv_param, sizeof(new_param));
		new_param.interval_max = adv_param->interval_max;
		new_param.interval_min = adv_param->interval_min;

		err = fmdn_adv_stop();
		if (err) {
			return err;
		}

		err = bt_le_ext_adv_update_param(fmdn_adv_set, &new_param);
		if (err) {
			LOG_ERR("FMDN State: bt_le_ext_adv_update_param returned error: %d", err);

			if (err == -EINVAL) {
				LOG_ERR("FMDN State: bt_le_ext_adv_update_param returning -EINVAL "
					"(%d) is typically caused by incorrect advertising "
					"parameters configuration that is passed to the "
					"bt_fast_pair_fmdn_adv_param_set API function", err);
			}

			new_param_valid = false;
		}

		if ((fmdn_conn_cnt < FMDN_MAX_CONN) && fp_fmdn_state_is_provisioned()) {
			int adv_start_err;

			adv_start_err = fmdn_adv_start();
			if (adv_start_err) {
				LOG_ERR("FMDN State: cannot restore advertising state: %d",
					adv_start_err);
				err = (err ? err : adv_start_err);
			}
		}
	}

	if (new_param_valid) {
		/* New advertising parameters accepted if the FMDN advertising set is used.
		 * Otherwise they are scheduled for validation during the bt_fast_pair_enable
		 * operation (in the fp_fmdn_state_init function).
		 */
		fmdn_adv_param.interval_max = adv_param->interval_max;
		fmdn_adv_param.interval_min = adv_param->interval_min;
	}

	return err;
}

int bt_fast_pair_fmdn_id_set(uint8_t id)
{
	if (bt_fast_pair_is_ready()) {
		/* It is not possible to switch the Bluetooth identity
		 * in the Fast Pair enabled state.
		 */
		return -EACCES;
	}

	fmdn_adv_param.id = id;

	return 0;
}

static void fmdn_state_battery_level_changed(void)
{
	/* Update the ongoing advertising payload asynchronously. */
	fmdn_adv_data_async_update();
}

static struct fp_fmdn_battery_cb fmdn_state_battery_cb = {
	.battery_level_changed = fmdn_state_battery_level_changed,
};

static void fmdn_post_init_work_handle(struct k_work *work)
{
	bool is_provisioned;

	/* Check provisioning state. */
	is_provisioned = fp_fmdn_state_is_provisioned();

	/* Notify the user about the initial provisioning state. */
	fp_fmdn_callbacks_provisioning_state_changed_notify(is_provisioned);

	if (is_provisioned) {
		int err;

		err = fmdn_adv_set_rotate();
		if (err) {
			LOG_ERR("FMDN State: fmdn_adv_set_rotate failed: %d", err);
			return;
		}

		err = fmdn_adv_start();
		if (err) {
			LOG_ERR("FMDN State: fmdn_adv_start failed: %d", err);
		}
	}
}

int fp_fmdn_state_init(void)
{
	int err;

	if (is_enabled) {
		LOG_WRN("FMDN State: module already initialized");
		return 0;
	}

	/* Set the default advertising parameters if they are not already set. */
	if (fmdn_adv_param.interval_min == 0) {
		struct bt_fast_pair_fmdn_adv_param *adv_param_default;

		adv_param_default = BT_FAST_PAIR_FMDN_ADV_PARAM_DEFAULT;
		err = bt_fast_pair_fmdn_adv_param_set(adv_param_default);
		if (err) {
			LOG_ERR("FMDN State: bt_fast_pair_fmdn_adv_param_set failed: %d", err);
			return err;
		}
	}

	/* Subscribe to the battery level changes. */
	err = fp_fmdn_battery_cb_register(&fmdn_state_battery_cb);
	if (err) {
		LOG_ERR("FMDN State: fp_fmdn_battery_cb_register failed: %d", err);
		return err;
	}

	/* Reserve the FMDN advertising set only in the Fast Pair enabled state. */
	if (!fmdn_adv_set) {
		err = fmdn_adv_set_setup();
		if (err) {
			LOG_ERR("FMDN State: fmdn_adv_set_setup failed: %d", err);
			return err;
		}
	}

	k_work_submit(&fmdn_post_init_work);

	is_enabled = true;

	LOG_DBG("FMDN State: enabled");

	return 0;
}

int fp_fmdn_state_uninit(void)
{
	int err;

	is_enabled = false;

	/* Release the FMDN advertising set only in the Fast Pair disabled state. */
	if (fmdn_adv_set) {
		err = fmdn_adv_stop();
		if (err) {
			LOG_ERR("FMDN State: fmdn_adv_stop failed: %d", err);
			return err;
		}

		err = fmdn_adv_set_teardown();
		if (err) {
			LOG_ERR("FMDN State: fmdn_adv_set_teardown failed: %d", err);
			return err;
		}
	}

	/* Reset the UTP mode. */
	fmdn_utp_mode_state_reset();

	/* Reset the connection state. */
	fmdn_conn_state_reset();

	/* Cancel the work for the provisioning_state_changed callback. */
	(void) k_work_cancel(&fmdn_post_init_work);

	LOG_DBG("FMDN State: disabled");

	return 0;
}

/* The state module requires EIK storage module for initialization. */
BUILD_ASSERT(CONFIG_BT_FAST_PAIR_STORAGE_INTEGRATION_INIT_PRIORITY <
	     FP_ACTIVATION_INIT_PRIORITY_DEFAULT);
FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_state,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      fp_fmdn_state_init,
			      fp_fmdn_state_uninit);
