/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#include <mpsl/mpsl_work.h>

#include "ecdh.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_sdc_ecdh);

static struct {
	uint8_t private_key_be[32];

	union {
		uint8_t public_key_be[64];
		uint8_t dhkey_be[32];
	};
} ecdh;

enum {
	GEN_PUBLIC_KEY  = BIT(0),
	GEN_DHKEY       = BIT(1),
	GEN_DHKEY_DEBUG = BIT(2),
};

static atomic_t cmd;

/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint8_t debug_private_key_be[32] = {
	0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38,
	0x74, 0xc9, 0xb3, 0xe3, 0xd2, 0x10, 0x3f, 0x50,
	0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99,
	0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd,
};

#if defined(CONFIG_BT_CTLR_ECDH_LIB_OBERON)
#include <ocrypto_ecdh_p256.h>

static uint8_t public_key(void)
{
	int err;

	do {
		err = bt_rand(ecdh.private_key_be, 32);
		if (err) {
			return BT_HCI_ERR_UNSPECIFIED;
		}

		if (!memcmp(ecdh.private_key_be, debug_private_key_be, 32)) {
			err = -1;
			continue;
		}

		err = ocrypto_ecdh_p256_public_key(ecdh.public_key_be,
						   ecdh.private_key_be);
	} while (err);

	return 0;
}

static uint8_t common_secret(bool use_debug)
{
	int err;

	err = ocrypto_ecdh_p256_common_secret(ecdh.dhkey_be,
					      use_debug ? debug_private_key_be :
							  ecdh.private_key_be,
					      ecdh.public_key_be);
	/* -1: public or private key was not a valid key */
	if (err) {
		/* If the remote P-256 public key is invalid
		 * (see [Vol 3] Part H, Section 2.3.5.6.1), the Controller shall
		 * return an error and should use the error code
		 * Invalid HCI Command Parameters (0x12).
		 */
		LOG_ERR("public key is not valid (err %d)", err);
		return BT_HCI_ERR_INVALID_PARAM;
	}

	return 0;
}
#endif /* defined(BT_CTLR_ECDH_LIB_OBERON) */

#if defined(CONFIG_BT_CTLR_ECDH_LIB_TINYCRYPT)
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>

int default_CSPRNG(uint8_t *dst, unsigned int len)
{
	return !bt_rand(dst, len);
}

static uint8_t public_key(void)
{
	do {
		int rc;

		rc = uECC_make_key(ecdh.public_key_be, ecdh.private_key_be,
				   &curve_secp256r1);
		if (rc != TC_CRYPTO_SUCCESS) {
			LOG_ERR("Failed to create ECC public/private pair");
			return BT_HCI_ERR_UNSPECIFIED;
		}

		/* make sure generated key isn't debug key */
	} while (memcmp(ecdh.private_key_be, debug_private_key_be, 32) == 0);

	return 0;
}

static uint8_t common_secret(bool use_debug)
{
	int err;

	err = uECC_valid_public_key(ecdh.public_key_be, &curve_secp256r1);
	if (err < 0) {
		/* If the remote P-256 public key is invalid
		 * (see [Vol 3] Part H, Section 2.3.5.6.1), the Controller shall
		 * return an error and should use the error code
		 * Invalid HCI Command Parameters (0x12).
		 */
		LOG_ERR("public key is not valid (err %d)", err);
		return BT_HCI_ERR_INVALID_PARAM;
	}

	err = uECC_shared_secret(ecdh.public_key_be,
				 use_debug ? debug_private_key_be :
					     ecdh.private_key_be,
				 ecdh.dhkey_be,
				 &curve_secp256r1);

	if (err != TC_CRYPTO_SUCCESS) {
		return BT_HCI_ERR_UNSPECIFIED;
	}

	return 0;
}
#endif /* defined(BT_CTLR_ECDH_LIB_TINYCRYPT) */

static struct net_buf *ecdh_p256_public_key(void)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;
	uint8_t status;

	status = public_key();

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;

	if (status) {
		(void)memset(evt->key, 0, sizeof(evt->key));
	} else {
		/* Reverse X */
		sys_memcpy_swap(&evt->key[0], &ecdh.public_key_be[0], 32);
		/* Reverse Y */
		sys_memcpy_swap(&evt->key[32], &ecdh.public_key_be[32], 32);
	}

	return buf;
}

static struct net_buf *ecdh_p256_common_secret(bool use_debug)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;
	uint8_t status;

	status = common_secret(use_debug);

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;

	if (status) {
		memset(evt->dhkey, 0xff, sizeof(evt->dhkey));
	} else {
		sys_memcpy_swap(evt->dhkey, ecdh.dhkey_be,
				sizeof(ecdh.dhkey_be));
	}

	return buf;
}

void ecdh_cmd_process(void)
{
	struct net_buf *buf;

	switch (atomic_get(&cmd)) {
	case GEN_PUBLIC_KEY:
		buf = ecdh_p256_public_key();
		break;
	case GEN_DHKEY:
		buf = ecdh_p256_common_secret(false);
		break;
	case GEN_DHKEY_DEBUG:
		buf = ecdh_p256_common_secret(true);
		break;
	default:
		LOG_WRN("Unknown command");
		buf = NULL;
		break;
	}

	atomic_set(&cmd, 0);
	if (buf) {
		bt_recv(buf);
	}
}

#if defined(CONFIG_BT_CTLR_ECDH_IN_MPSL_WORK)
static struct k_work ecdh_work;

static void work_submit(void)
{
	mpsl_work_submit(&ecdh_work);
}

void ecdh_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	ecdh_cmd_process();
}
#else
struct k_poll_signal ecdh_signal;

static struct k_thread ecdh_thread_data;
static K_KERNEL_STACK_DEFINE(ecdh_thread_stack, CONFIG_BT_CTLR_ECDH_STACK_SIZE);

static void work_submit(void)
{
	k_poll_signal_raise(&ecdh_signal, 0);
}

static void ecdh_thread(void *p1, void *p2, void *p3)
{
	struct k_poll_event events[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &ecdh_signal),
	};

	while (true) {
		k_poll(events, 1, K_FOREVER);

		k_poll_signal_reset(&ecdh_signal);
		events[0].state = K_POLL_STATE_NOT_READY;

		ecdh_cmd_process();
	}
}
#endif /* !defined(CONFIG_BT_CTLR_ECDH_IN_MPSL_WORK) */

void hci_ecdh_init(void)
{
#if !defined(CONFIG_BT_CTLR_ECDH_IN_MPSL_WORK)
	k_poll_signal_init(&ecdh_signal);

	k_thread_create(&ecdh_thread_data, ecdh_thread_stack,
			K_KERNEL_STACK_SIZEOF(ecdh_thread_stack), ecdh_thread,
			NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	k_thread_name_set(&ecdh_thread_data, "BT CTLR ECDH");
#else
	k_work_init(&ecdh_work, ecdh_work_handler);
#endif /* !defined(CONFIG_BT_CTLR_ECDH_IN_MPSL_WORK) */
}

void hci_ecdh_uninit(void)
{
#if !defined(CONFIG_BT_CTLR_ECDH_IN_MPSL_WORK)
	k_thread_abort(&ecdh_thread_data);
#endif /* !defined(CONFIG_BT_CTLR_ECDH_IN_MPSL_WORK) */
}


uint8_t hci_cmd_le_read_local_p256_public_key(void)
{
	if (!atomic_cas(&cmd, 0, GEN_PUBLIC_KEY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	work_submit();

	return 0;
}

uint8_t cmd_le_generate_dhkey(uint8_t *key, uint8_t key_type)
{
	if (!atomic_cas(&cmd, 0, key_type ? GEN_DHKEY_DEBUG : GEN_DHKEY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sys_memcpy_swap(&ecdh.public_key_be[0], &key[0], 32);
	sys_memcpy_swap(&ecdh.public_key_be[32], &key[32], 32);

	work_submit();

	return 0;
}

uint8_t hci_cmd_le_generate_dhkey(struct bt_hci_cp_le_generate_dhkey *p_params)
{
	return cmd_le_generate_dhkey(p_params->key,
				     BT_HCI_LE_KEY_TYPE_GENERATED);
}

uint8_t hci_cmd_le_generate_dhkey_v2(struct bt_hci_cp_le_generate_dhkey_v2 *p_params)
{
	if (p_params->key_type > BT_HCI_LE_KEY_TYPE_DEBUG) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	return cmd_le_generate_dhkey(p_params->key, p_params->key_type);
}
