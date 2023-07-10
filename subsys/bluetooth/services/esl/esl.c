/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/ead.h>
#include <bluetooth/services/esl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/net/buf.h>
#include <stdio.h>
#include "esl_internal.h"
#include "esl_settings.h"

LOG_MODULE_REGISTER(bt_esl, CONFIG_BT_ESL_LOG_LEVEL);

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

struct bt_esls *esl_obj_l;
static struct esl_adv_work_info {
	struct k_work_delayable work;
	enum esl_adv_mode mode;
} esl_adv_work;

static struct esl_ecp_work_info {
	struct k_work work;
	enum esl_ecp_command_src src;
} esl_ecp_work;

static struct esl_state_timeout_work_info {
	struct k_work_delayable work;
	enum esl_timeout_mode mode;
} esl_state_timeout_work;

static int64_t abs_time_anchor;
uint32_t sync_timeout_count;
uint32_t acl_timeout_count;
static struct bt_le_per_adv_sync_transfer_param past_param;
static struct bt_le_per_adv_sync *pawr_sync;
static uint8_t sync_count;
static uint8_t response_slot_delay;
static struct esl_pawr_sync_work_info {
	struct k_work_delayable work;
	struct net_buf_simple buf;
	uint16_t periodic_event_counter;
	uint8_t subevent;
} esl_pawr_sync_work;

/** ESL service State Machine */
static struct esl_sm_object sm_obj;

#define ESL_ADV_DELAY K_MSEC(150)

/** Thread for PAwR **/
#define PAWR_WQ_STACK_SIZE KB(2)
#define PAWR_WQ_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(pawr_wq_stack_area, PAWR_WQ_STACK_SIZE);
struct k_work_q pawr_work_q;

static uint8_t esl_ad_data[ESL_ENCRTYPTED_DATA_MAX_LEN] = {0};
static struct net_buf_simple pa_rsp;

/** Thread for LED **/
#define LED_WQ_STACK_SIZE KB(1)
#define LED_WQ_PRIORITY	  K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(led_wq_stack_area, LED_WQ_STACK_SIZE);
struct k_work_q led_work_q;

/** Thread for Display **/
#define DISPLAY_WQ_STACK_SIZE KB(1)
#define DISPLAY_WQ_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(display_wq_stack_area, DISPLAY_WQ_STACK_SIZE);
struct k_work_q display_work_q;

NET_BUF_SIMPLE_DEFINE(ecp_rsp_net_buf, ESL_ENCRTYPTED_DATA_MAX_LEN);
NET_BUF_SIMPLE_DEFINE(ecp_net_buf, ESL_ENCRTYPTED_DATA_MAX_LEN);

static uint8_t ecp_rsp_last_tlv_len;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ESL_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)};

static const struct bt_data sd[] = {BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)};
struct bt_conn *auth_conn;

static void ecp_command_handler(struct k_work *work);
static void esl_create_pawr_sync(void);
static void ecp_response_handler(struct bt_esls *esl_obj, enum esl_ecp_command_src src);
static void led_dwork_reset(uint8_t idx);
static void display_dwork_reset(uint8_t idx);

static int set_pawr_response(struct net_buf_simple *buf, int8_t rsp_slot_id)
{
	int err = 0;

	if (rsp_slot_id == -1) {
		LOG_INF("All broadcast TLV or no addressed TLV, no need replied");
	} else {
		struct bt_le_per_adv_response_params param;

		LOG_HEXDUMP_DBG(buf->data, buf->len, "set_pawr_response");
		pa_rsp.len = esl_compose_ad_data(pa_rsp.data, buf->data, buf->len,
						 esl_obj_l->esl_chrc.esl_randomizer,
						 esl_obj_l->esl_chrc.esl_rsp_key);
		param.request_event = esl_pawr_sync_work.periodic_event_counter;
		param.request_subevent = esl_pawr_sync_work.subevent;
		param.response_subevent = esl_pawr_sync_work.subevent;
		param.response_slot = rsp_slot_id;
		err = bt_le_per_adv_set_response_data(pawr_sync, &param, &pa_rsp);
	}

	net_buf_simple_remove_mem(buf, buf->len);
	return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	LOG_DBG("connected %d", err);
	if (err) {
		if (err == BT_HCI_ERR_ADV_TIMEOUT) {
			LOG_ERR("adv timeout");
			bt_esl_adv_start(ESL_ADV_DEMO);
		} else {
			LOG_ERR("Connection failed (err %u)", err);
			return;
		}
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);
	atomic_clear_bit(&esl_obj_l->configuring_state, ESL_UPDATE_COMPLETE_SET_BIT);
	esl_obj_l->conn = conn;

	if (atomic_test_bit(&esl_obj_l->basic_state, ESL_SYNCHRONIZED)) {
		esl_obj_l->esl_chrc.connect_from = ACL_FROM_PAWR;
	} else {
		esl_obj_l->esl_chrc.connect_from = ACL_FROM_SCAN;
	}

	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		led_dwork_reset(INDICATION_LED_IDX);
		esl_obj_l->cb.led_control(INDICATION_LED_IDX, 0, 1);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);
#if defined(CONFIG_BT_ESL_DEMO_SECURITY)
	bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
#endif
	printk("#DISCONNECTED:0x%02x\n", reason);
	if (auth_conn) {
		LOG_INF("unref auth_conn");
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
	}

	do {
		if (reason != BT_HCI_ERR_LOCALHOST_TERM_CONN || reason == BT_HCI_ERR_AUTH_FAIL) {
			/** ESLP 5.4 Updating state the ESL shall not enter the Updating state while
			 * connected to the untrusted device.
			 **/
			if (reason == BT_HCI_ERR_AUTH_FAIL &&
			    atomic_test_bit(&esl_obj_l->basic_state, ESL_SYNCHRONIZED)) {
				break;
			} else if (esl_is_configuring_state(esl_obj_l)) {
				esl_obj_l->esl_sm_timeout_update_needed = false;
				bt_esl_state_transition(SM_UNSYNCHRONIZED);
				/** When link loss occurs before configuration of the ESL has been
				 * completed, the ESL reverts to the Unassociated state. ESL starts
				 * advertising
				 **/
			} else if (esl_obj_l->esl_state == SM_CONFIGURING ||
				   esl_obj_l->esl_state == SM_UNASSOCIATED) {
				bt_esl_state_transition(SM_UNASSOCIATED);
			}
		}
	} while (0);

	esl_obj_l->esl_chrc.connect_from = ACL_NOT_CONNECTED;
	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		esl_obj_l->cb.led_control(INDICATION_LED_IDX, 0, 0);
	}
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
		if (esl_obj_l->esl_state == SM_UNSYNCHRONIZED ||
		    esl_obj_l->esl_state == SM_SYNCHRONIZED) {
			bt_esl_state_transition(SM_UPDATING);
		}

	} else {
		LOG_WRN("Security failed: %s level %u err %d", addr, level, err);
	}
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_DBG("Updated MTU: TX: %d RX: %d bytes", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void esl_init_provisioned_data(void)
{
	esl_obj_l->esl_chrc.esl_addr = 0x8000;
	esl_obj_l->esl_chrc.connect_from = ACL_NOT_CONNECTED;
	esl_state_timeout_work.mode = ESL_UNASSOCIATED_TIMEOUT_MODE;
	memset(esl_obj_l->esl_chrc.esl_ap_key.key_v, 0, EAD_KEY_MATERIAL_LEN);
	memset(esl_obj_l->esl_chrc.esl_randomizer, 0, EAD_RANDOMIZER_LEN);
	memset(esl_obj_l->esl_chrc.esl_rsp_key.key_v, 0, EAD_KEY_MATERIAL_LEN);
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	for (size_t idx = 0; idx < CONFIG_BT_ESL_IMAGE_MAX; idx++) {
		esl_obj_l->stored_image_size[idx] = 0;
	}
#endif
	esl_obj_l->esl_sm_timeout_update_needed = true;
}

/* state machine function*/
void sm_unassociated_entry(void *obj)
{
	LOG_DBG("%s", __func__);
	esl_obj_l->esl_state = SM_UNASSOCIATED;
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */

	bt_esl_adv_start(ESL_ADV_DEMO);
}

void sm_unassociated_run(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_unassociated_exit(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_configuring_entry(void *obj)
{
	LOG_DBG("%s", __func__);
	esl_obj_l->esl_state = SM_CONFIGURING;
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_configuring_run(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_configuring_exit(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_configured_entry(void *obj)
{
	LOG_DBG("%s", __func__);
	esl_obj_l->esl_state = SM_CONFIGURED;
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
	esl_create_pawr_sync();
}

void sm_configured_run(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_configured_exit(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_synchronized_entry(void *obj)
{
	LOG_DBG("%s", __func__);
	esl_obj_l->esl_state = SM_SYNCHRONIZED;
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
	printk("#SYNC:1\n");
	atomic_set_bit(&esl_obj_l->basic_state, ESL_SYNCHRONIZED);
	bt_esl_disconnect();
	/* Explicity stop adv since TAG has 2 BT_CONN context */
	bt_esl_adv_stop();
	if (esl_obj_l->esl_sm_timeout_update_needed) {
		/** Reschedule esl state timeout work item when entering sync state */
		esl_state_timeout_work.mode = ESL_UNSYNCED_TIMEOUT_MODE;
		LOG_INF("Set unsynced timeout %d seconds", CONFIG_BT_ESL_UNSYNCHRONIZED_TIMEOUT);
		k_work_reschedule(&esl_state_timeout_work.work,
				  K_SECONDS(CONFIG_BT_ESL_UNSYNCHRONIZED_TIMEOUT));
	}
}

void sm_synchronized_run(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_synchronized_exit(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
	sync_count = 0;
}

void sm_updating_entry(void *obj)
{
	LOG_DBG("%s", __func__);
	esl_obj_l->esl_state = SM_UPDATING;
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
	sync_count = 0;
	/** Cancel esl state timeout work item when entering updating state */
	LOG_INF("Cancel unassociated timeout");
	k_work_cancel_delayable(&esl_state_timeout_work.work);
}

void sm_updating_run(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_updating_exit(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_unsynchronized_entry(void *obj)
{
	LOG_DBG("%s", __func__);
	esl_obj_l->esl_state = SM_UNSYNCHRONIZED;
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
	bt_esl_adv_start(ESL_ADV_DEMO);
	atomic_clear_bit(&esl_obj_l->basic_state, ESL_SYNCHRONIZED);
	if (esl_obj_l->esl_sm_timeout_update_needed) {
		/** Renew esl state timeout work item when entering unsynced state */
		esl_state_timeout_work.mode = ESL_UNASSOCIATED_TIMEOUT_MODE;
		LOG_INF("set unassociated timeout %d seconds", CONFIG_BT_ESL_UNASSOCIATED_TIMEOUT);
		k_work_reschedule(&esl_state_timeout_work.work,
				  K_SECONDS(CONFIG_BT_ESL_UNASSOCIATED_TIMEOUT));
	}
}

void sm_unsynchronized_run(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

void sm_unsynchronized_exit(void *obj)
{
	LOG_DBG("%s", __func__);
	/** retrieve esl state machine obj
	 * struct esl_sm_object *o = ((struct esl_sm_object *)obj);
	 */
}

/* Populate state table */
static const struct smf_state esl_states[] = {
	[SM_UNASSOCIATED] =
		SMF_CREATE_STATE(sm_unassociated_entry, sm_unassociated_run, sm_unassociated_exit),
	[SM_CONFIGURING] =
		SMF_CREATE_STATE(sm_configuring_entry, sm_configuring_run, sm_configuring_exit),
	[SM_CONFIGURED] =
		SMF_CREATE_STATE(sm_configured_entry, sm_configured_run, sm_configured_exit),
	[SM_SYNCHRONIZED] =
		SMF_CREATE_STATE(sm_synchronized_entry, sm_synchronized_run, sm_synchronized_exit),
	[SM_UPDATING] = SMF_CREATE_STATE(sm_updating_entry, sm_updating_run, sm_updating_exit),
	[SM_UNSYNCHRONIZED] = SMF_CREATE_STATE(sm_unsynchronized_entry, sm_unsynchronized_run,
					       sm_unsynchronized_exit),
};
void bt_esl_state_transition(uint8_t state)
{
	smf_set_state(SMF_CTX(&sm_obj), &esl_states[state]);
}

/* Parsing PA packet */
static void esl_parse_sync_packet(struct net_buf_simple *buf)
{
	uint8_t op_code, cp_tag, cp_len, esl_id, cur = 0;
	uint8_t payload_len;
	int8_t tlv_slot = -1, rsp_slot = -1;

	LOG_HEXDUMP_DBG(buf->data, buf->len, "esl_parse_sync_packet");

	/* Check packet rational here to reduce time in pawr recv callback */
	payload_len = buf->data[0] - 1;
	if ((buf->len != (buf->data[0] + 1)) || buf->data[1] != BT_DATA_ESL) {
		LOG_ERR("PAWR packet is not legit ESL AD data");
		return;
	}

	net_buf_simple_pull(buf, AD_HEADER_LEN);

	if (buf->data[cur++] != GROUPID_BYTE(esl_obj_l->esl_chrc.esl_addr)) {
		LOG_DBG("Group id is not for us 0x%02x 0x%02x", buf->data[0],
			GROUPID_BYTE(esl_obj_l->esl_chrc.esl_addr));
		return;
	}

	/* Clear response net_buf for new pawr sync packet */
	net_buf_simple_remove_mem(&ecp_rsp_net_buf, ecp_rsp_net_buf.len);

	/* Payload begin after group id*/
	while (cur < payload_len) {
		/* check OP code and length */
		op_code = buf->data[cur];
		cp_len = ECP_LEN(buf->data[cur]);
		cp_tag = ECP_TAG(buf->data[cur]);
		esl_id = (buf->data[cur + 1]);
		LOG_HEXDUMP_DBG((buf->data + cur), cp_len, "sync command");
		tlv_slot++;

		if (esl_id == LOW_BYTE(esl_obj_l->esl_chrc.esl_addr) ||
		    esl_id == ESL_ADDR_BROADCAST) {
			net_buf_simple_init_with_data(&ecp_net_buf, (buf->data + cur), cp_len);
			esl_ecp_work.src = ESL_PAWR;
			k_work_submit(&esl_ecp_work.work);
			if (esl_id != ESL_ADDR_BROADCAST) {
				rsp_slot = tlv_slot;
			}
		} else {
			LOG_DBG("esl id is not for us 0x%02x 0x%02x", esl_id,
				LOW_BYTE(esl_obj_l->esl_chrc.esl_addr));
		}

		if ((cur + cp_len) <= payload_len) {
			cur += cp_len;

		} else {
			LOG_DBG("the rest of data is MIC, stop parsing TLV");
			break;
		}
	}

	set_pawr_response(&ecp_rsp_net_buf, rsp_slot);
	LOG_INF("tlv counts %d rsp_slot %d", tlv_slot, rsp_slot);
}

static void sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_le_per_adv_sync_subevent_params param;
	uint8_t subevents[1] = {0};

	subevents[0] = GROUPID_BYTE(esl_obj_l->esl_chrc.esl_addr);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_INF("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
		"Interval 0x%04x (%u), PHY %d, sync info sid %d, service_data 0x%04x "
		"num_events %d, sub_int %d, resp delay %d, resp spacing %d",
		bt_le_per_adv_sync_get_index(sync), le_addr, info->interval, info->interval,
		info->phy, info->sid, info->service_data, info->num_subevents,
		info->subevent_interval, info->response_slot_delay, info->response_slot_spacing);
	pawr_sync = sync;
	param.num_subevents = 1;
	param.properties = 0;
	response_slot_delay = info->response_slot_delay;
	param.subevents = subevents;

	bt_le_per_adv_sync_subevent(sync, &param);
	if (esl_obj_l->esl_state != SM_SYNCHRONIZED) {
		esl_obj_l->esl_sm_timeout_update_needed = true;
		bt_esl_state_transition(SM_SYNCHRONIZED);
		if (esl_obj_l->cb.display_associated) {
			esl_obj_l->cb.display_associated(0);
		}
	}
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_DBG("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
		bt_le_per_adv_sync_get_index(sync), le_addr);
	printk("#SYNC:0\n");

	esl_stop_sync_pawr();
	esl_obj_l->esl_sm_timeout_update_needed = true;
	bt_esl_state_transition(SM_UNSYNCHRONIZED);
	atomic_clear_bit(&esl_obj_l->basic_state, ESL_SYNCHRONIZED);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	if (esl_obj_l->esl_state != SM_SYNCHRONIZED) {
		LOG_DBG("Ignore ESL sync data when not synchronized");
		return;
	}

	/* LED blinks one time when received sync */
	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		led_dwork_reset(INDICATION_LED_IDX);
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].color_brightness = 0;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].flash_pattern[0] = 0x1;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].bit_off_period = 50;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].bit_on_period = 50;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].repeat_type = 0;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].repeat_duration = 1;
		esl_obj_l->led_dworks[INDICATION_LED_IDX].pattern_idx =
			ESL_LED_FLASH_PATTERN_START_BIT_IDX;
		k_work_schedule_for_queue(
			&led_work_q, &esl_obj_l->led_dworks[INDICATION_LED_IDX].work, K_NO_WAIT);
	}

	if (buf->len == 0) {
		LOG_DBG("Empty sync packet");
		return;
	}

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_DBG("PER_ADV_SYNC[%u]: [DEVICE]: %s recved, tx_power %i, "
		"RSSI %i, CTE %u, data length %u",
		bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power, info->rssi,
		info->cte_type, buf->len);
	LOG_DBG("rsp evt %d sub %d len %d", info->periodic_event_counter, info->subevent, buf->len);

	esl_pawr_sync_work.periodic_event_counter = info->periodic_event_counter;
	esl_pawr_sync_work.subevent = info->subevent;
	net_buf_simple_init_with_data(&esl_pawr_sync_work.buf, buf->data, buf->len);
	k_work_schedule_for_queue(&pawr_work_q, &esl_pawr_sync_work.work, K_NO_WAIT);
}
static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb, .term = term_cb, .recv = recv_cb};

static void esl_setup_pawr_sync(void)
{
	LOG_DBG("ESL TAG Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
}

static void esl_create_pawr_sync(void)
{
	int err;

	/* Use maximun value to keep sync as long as possible */
	past_param.skip = 5;	   /* BT_GAP_PER_ADV_MAX_SKIP */
	past_param.timeout = 2000; /* BT_GAP_PER_ADV_MAX_TIMEOUT; 183.84 seconds */
	err = bt_le_per_adv_sync_transfer_subscribe(esl_obj_l->conn, &past_param);
	if (err != 0) {
		LOG_ERR("PAST subscribe failed (err %d)", err);
		return;
	}
}

static void esl_led_check_and_set_color(uint8_t *led_type, uint8_t color_brightness)
{
	uint8_t type;
	uint8_t color;

	type = (*led_type & 0xC0) >> 6;
	color = (color_brightness & 0x3f);

	if (type == ESL_LED_MONO) {
		LOG_DBG("Mono led, ignore color");
	} else {
		LOG_DBG("Set led color to 0x%02x r %u%u g %u%u b %u%u", color, CHECK_BIT(color, 1),
			CHECK_BIT(color, 0), CHECK_BIT(color, 3), CHECK_BIT(color, 2),
			CHECK_BIT(color, 5), CHECK_BIT(color, 4));
		*led_type = color;
	}
}

uint32_t esl_get_abs_time(void)
{
	return (uint32_t)(k_uptime_get_32() - abs_time_anchor);
}

static void esl_state_timeout_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_state_timeout_work_info *info =
		CONTAINER_OF(dwork, struct esl_state_timeout_work_info, work);

	/* Check if need to change to unsynchronized */
	switch (info->mode) {
	case ESL_UNSYNCED_TIMEOUT_MODE:
		/* transit to unsync and advertising */
		LOG_INF("Sync timeout\n");
		esl_obj_l->esl_sm_timeout_update_needed = true;
		bt_esl_state_transition(SM_UNSYNCHRONIZED);
		esl_stop_sync_pawr();
		printk("sync_timeout_count %d\n", ++sync_timeout_count);

		break;
	case ESL_UNASSOCIATED_TIMEOUT_MODE:
		/* transit to unsync and unassociated */
		LOG_INF("Unassociated timeout\n");
		bt_esl_unassociate();
		printk("unasso_timeout_count %d\n", ++acl_timeout_count);

		break;
	default:
		LOG_WRN("No such timeout mode %d", info->mode);
		break;
	}
}

void esl_abs_time_update(uint32_t new_abs_time)
{
	abs_time_anchor = k_uptime_get_32() - new_abs_time;
	LOG_DBG("abs_time_anchor %lld new_abs_time %d abs_time %d\n", abs_time_anchor, new_abs_time,
		esl_get_abs_time());
}

void led_delay_worker_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_dwork *ctx = CONTAINER_OF(dwork, struct led_dwork, work);
	uint64_t pattern;
	int err;

	CHECKIF(ctx->led_idx >= CONFIG_BT_ESL_LED_MAX) {
		LOG_ERR("LED work index is out of range");
		return;
	}

	esl_obj_l->led_dworks[ctx->led_idx].start_abs_time = 0;
	/** ESLS 3.10.2.10.2.4 Repeat_Type and Repeats_Duration
	 * However, if the value of the Repeats_Duration field is set to the special value of 0,
	 * the Flashing_Pattern field shall be ignored and the Repeat_Type field shall have the
	 * following interpretation: • If Repeat_Type = 0, then the LED shall be turned off
	 * continuously. • If Repeat_Type = 1, then the LED shall be turned on continuously.
	 **/
	if (esl_obj_l->esl_chrc.led_objs[ctx->led_idx].repeat_duration == 0) {
		LOG_DBG("ignore flashing pattern");
		if (esl_obj_l->esl_chrc.led_objs[ctx->led_idx].repeat_type == 1) {
			LOG_DBG("LED on continuously");
			esl_obj_l->cb.led_control(
				ctx->led_idx,
				esl_obj_l->esl_chrc.led_objs[ctx->led_idx].color_brightness, true);
			esl_obj_l->led_dworks[ctx->led_idx].active = true;
		} else {
			LOG_DBG("LED off continuously");
			esl_obj_l->cb.led_control(
				ctx->led_idx,
				esl_obj_l->esl_chrc.led_objs[ctx->led_idx].color_brightness, false);
			esl_obj_l->led_dworks[ctx->led_idx].active = false;
		}

		/* Continuously control no need to repeat */
		return;
	} else if (esl_obj_l->led_dworks[ctx->led_idx].work_enable != true) {
		if (esl_obj_l->esl_chrc.led_objs[ctx->led_idx].repeat_type == 1) {
			esl_obj_l->led_dworks[ctx->led_idx].stop_abs_time =
				esl_get_abs_time() +
				esl_obj_l->esl_chrc.led_objs[ctx->led_idx].repeat_duration *
					MSEC_PER_SEC;
			LOG_DBG("LED repeat type Time duration seconds");
		} else {
			esl_obj_l->led_dworks[ctx->led_idx].stop_abs_time = 0;
			LOG_DBG("LED repeat type Number of times");
		}

		esl_obj_l->led_dworks[ctx->led_idx].work_enable = true;
		esl_obj_l->led_dworks[ctx->led_idx].active = true;
	}

	pattern = ((uint64_t)sys_get_le32(
			   &esl_obj_l->esl_chrc.led_objs[ctx->led_idx].flash_pattern[1])
		   << 8) |
		  esl_obj_l->esl_chrc.led_objs[ctx->led_idx].flash_pattern[0];

	if (esl_obj_l->led_dworks[ctx->led_idx].stop_abs_time != 0 &&
	    esl_get_abs_time() > esl_obj_l->led_dworks[ctx->led_idx].stop_abs_time) {
		LOG_DBG("timed led control stop at abs_time %d, stop_abs_time %d",
			esl_get_abs_time(), esl_obj_l->led_dworks[ctx->led_idx].stop_abs_time);
		esl_obj_l->cb.led_control(
			ctx->led_idx, esl_obj_l->esl_chrc.led_objs[ctx->led_idx].color_brightness,
			false);
		esl_obj_l->led_dworks[ctx->led_idx].work_enable = false;
		esl_obj_l->led_dworks[ctx->led_idx].active = false;
	}

	/* check if pattern finished */
	if (esl_obj_l->led_dworks[ctx->led_idx].pattern_idx == 0) {
		LOG_DBG("pattern done %d times", esl_obj_l->led_dworks[ctx->led_idx].repeat_counts);
		esl_obj_l->led_dworks[ctx->led_idx].repeat_counts += 1;
		esl_obj_l->led_dworks[ctx->led_idx].pattern_idx = (63 - __builtin_clzll(pattern));
		if (esl_obj_l->led_dworks[ctx->led_idx].repeat_counts >=
		    esl_obj_l->esl_chrc.led_objs[ctx->led_idx].repeat_duration) {
			LOG_DBG("pattern finished");
			esl_obj_l->cb.led_control(
				ctx->led_idx,
				esl_obj_l->esl_chrc.led_objs[ctx->led_idx].color_brightness, false);
			esl_obj_l->led_dworks[ctx->led_idx].work_enable = false;
			esl_obj_l->led_dworks[ctx->led_idx].active = false;
			return;
		}
		/**
		 *  Check if led control started
		 * if pattern_idx = ESL_LED_FLASH_PATTERN_START_BIT_IDX means led control is not
		 *start
		 **/
	} else if (esl_obj_l->led_dworks[ctx->led_idx].pattern_idx ==
		   ESL_LED_FLASH_PATTERN_START_BIT_IDX) {

		/* find right most set bit*/
		esl_obj_l->led_dworks[ctx->led_idx].pattern_idx = (63 - __builtin_clzll(pattern));
		LOG_DBG("LED control not started. Most right set bit %d",
			esl_obj_l->led_dworks[ctx->led_idx].pattern_idx);
		LOG_DBG("pattern %llu", pattern);
		LOG_HEXDUMP_DBG(esl_obj_l->esl_chrc.led_objs[ctx->led_idx].flash_pattern,
				ESL_LED_FLASH_PATTERN_LEN, "led pattern");
	}

	/* led control is finished */
	if (esl_obj_l->led_dworks[ctx->led_idx].work_enable == false) {
		return;
	}

	if (FIELD_GET((1ULL << (esl_obj_l->led_dworks[ctx->led_idx].pattern_idx)), pattern) == 1) {
		esl_obj_l->cb.led_control(
			ctx->led_idx, esl_obj_l->esl_chrc.led_objs[ctx->led_idx].color_brightness,
			true);
		err = k_work_schedule_for_queue(
			&led_work_q, dwork,
			K_MSEC(esl_obj_l->esl_chrc.led_objs[ctx->led_idx].bit_on_period * 2));

	} else {
		esl_obj_l->cb.led_control(
			ctx->led_idx, esl_obj_l->esl_chrc.led_objs[ctx->led_idx].color_brightness,
			false);
		err = k_work_schedule_for_queue(
			&led_work_q, dwork,
			K_MSEC(esl_obj_l->esl_chrc.led_objs[ctx->led_idx].bit_off_period * 2));
	}

	if (esl_obj_l->led_dworks[ctx->led_idx].pattern_idx != 0) {
		esl_obj_l->led_dworks[ctx->led_idx].pattern_idx--;
	}
}

void display_delay_worker_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct display_dwork *ctx = CONTAINER_OF(dwork, struct display_dwork, work);

	CHECKIF(ctx->display_idx >= CONFIG_BT_ESL_DISPLAY_MAX) {
		LOG_ERR("Display work index is out of range");
		return;
	}

	ctx->active = true;
	ctx->start_abs_time = 0;
	if (esl_obj_l->cb.display_control) {
		esl_obj_l->cb.display_control(ctx->display_idx, ctx->img_idx, true);
	}
}

bool esl_is_configuring_state(struct bt_esls *esl_obj)
{
	bool configured = atomic_test_bit(&esl_obj->configuring_state, ESL_ADDR_SET_BIT) &&
			  atomic_test_bit(&esl_obj->configuring_state, ESL_AP_SYNC_KEY_SET_BIT) &&
			  atomic_test_bit(&esl_obj->configuring_state, ESL_RESPONSE_KEY_SET_BIT) &&
			  atomic_test_bit(&esl_obj->configuring_state, ESL_ABS_TIME_SET_BIT);

	LOG_DBG("configuring_state %ld", atomic_get(&esl_obj->configuring_state));
	if (configured) {
		LOG_DBG("All data needed for configuring is set");
		printk("#CONFIGURED:1\n");
		bt_esl_state_transition(SM_CONFIGURING);
	}

	return configured;
}

void esl_stop_sync_pawr(void)
{
	int err;

	sync_count = 0;
	LOG_INF("Unsubscribe ESL TAG Periodic Advertising Sync...");
	err = bt_le_per_adv_sync_transfer_unsubscribe(NULL);
	if (err) {
		LOG_ERR("bt_le_per_adv_sync_delete failed (err %d)", err);
	}

	err = bt_le_per_adv_sync_delete(pawr_sync);
	if (err) {
		LOG_ERR("bt_le_per_adv_sync_delete failed (err %d)", err);
	}
}

static void ecp_notify_unassociated_cb(struct bt_conn *conn, void *user_data)
{
	LOG_DBG("");
	bt_esl_unassociate();
	esl_obj_l->ecp_notify_params.func = NULL;
}

void ecp_notify_cb(struct bt_conn *conn, void *user_data)
{
	LOG_DBG("");
	uint8_t *buf = user_data;

	LOG_HEXDUMP_DBG(buf, ESL_RESPONSE_MAX_LEN, "");
	LOG_HEXDUMP_DBG(ecp_rsp_net_buf.data, ESL_RESPONSE_MAX_LEN, "");
}

static void ecp_command_handler(struct k_work *work)
{
	struct esl_ecp_work_info *info = CONTAINER_OF(work, struct esl_ecp_work_info, work);
	uint8_t op_code, cp_tag, cp_len, esl_id;
	uint8_t sensor_idx, display_idx, image_idx, led_idx, led_color;
	size_t img_size = 0;
	uint32_t new_abs_time, offset_time;
	struct bt_esls *esl_obj = esl_get_esl_obj();
	uint8_t *command = ecp_net_buf.data;
	uint16_t len = ecp_net_buf.len;

	op_code = command[0];
	cp_len = ECP_LEN(command[0]);
	cp_tag = ECP_TAG(command[0]);
	esl_obj->ecp_notify_params.func = NULL;
	esl_obj->esl_ecp_resp_needed = true;
	/* Give response op code a default value */
	esl_obj->resp.resp_op = OP_BASIC_STATE;
	/* Check total control command length with tag and length */
	if (len != (cp_len)) {
		LOG_ERR("ESL control command length not match %d %d", len, (cp_len));
		esl_obj->resp.resp_op = OP_ERR;
		esl_obj->resp.error_code = ERR_INV_PARAMS;
		goto ecp_response;
	}

	if (((op_code & OP_VENDOR_SPECIFIC_STATE_MASK) == OP_VENDOR_SPECIFIC_STATE_MASK) &&
	    !IS_ENABLED(CONFIG_BT_ESL_VENDOR_SPECIFIC_SUPPORT)) {
		LOG_ERR("Vendor specific command is not supported");
		esl_obj->resp.resp_op = OP_ERR;
		esl_obj->resp.error_code = ERR_INV_OPCODE;
		goto ecp_response;
	}

	esl_id = command[1];
	if (esl_id != (LOW_BYTE(esl_obj->esl_chrc.esl_addr)) && esl_id != ESL_ADDR_BROADCAST) {
		LOG_ERR("ESL_ID not match 0x%02x 0x%02x", LOW_BYTE(esl_id),
			LOW_BYTE(esl_obj->esl_chrc.esl_addr));
		esl_obj->resp.resp_op = OP_ERR;
		/**
		 *  ESLS/SR/ECP/BI-14-C [Reject Invalid ESL ID] says Invalid Parameter(s)
		 *  ESLS spec says Invalid Parameter(s) page 28
		 */
		esl_obj->resp.error_code = ERR_INV_PARAMS;
		goto ecp_response;
	}

	if (esl_id == ESL_ADDR_BROADCAST) {
		LOG_DBG("BROADCAST");
	}

	if (esl_obj->busy == true) {
		esl_obj->resp.resp_op = OP_ERR;
		esl_obj->resp.error_code = ERR_RETRY;
		esl_obj->busy = false;
	}

	if (esl_obj->resp.resp_op != OP_ERR) {
		switch (op_code) {
		case OP_PING:
			LOG_DBG("OP_PING");
			esl_obj->resp.resp_op = OP_BASIC_STATE;
			break;
		case OP_UNASSOCIATE:
			LOG_DBG("OP_UNASSOCIATE");
			if (esl_id != ESL_ADDR_BROADCAST) {
				atomic_clear_bit(&esl_obj->basic_state, ESL_SYNCHRONIZED);
				esl_obj->resp.resp_op = OP_BASIC_STATE;
				/* execute this after ecp response cb bt_esl_unassociate(); */
				esl_obj->ecp_notify_params.func = ecp_notify_unassociated_cb;
			} else {
				LOG_DBG("BROADCAST can't unassociated TAG");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_RETRY;
			}

			break;
		case OP_SERVICE_RESET:
			LOG_DBG("OP_SERVICE_RESET");
			atomic_clear_bit(&esl_obj->basic_state, ESL_SERVICE_NEEDED_BIT);
			esl_obj->resp.resp_op = OP_BASIC_STATE;
			break;
		case OP_FACTORY_RESET:
			LOG_DBG("OP_FACTORY_RESET");
			if (esl_obj->esl_state == SM_SYNCHRONIZED) {
				LOG_ERR("FACTORY_RESET is invalid in synced mode");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_STATE;
				break;
			}

			if (esl_id == ESL_ADDR_BROADCAST) {
				LOG_ERR("BROADCAST can't factory reset TAG");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_RETRY;
				break;
			}

			bt_esl_factory_reset();
			esl_obj->esl_ecp_resp_needed = false;
			break;
		case OP_UPDATE_COMPLETE:
			LOG_DBG("OP_UPDATE_COMPLETE");
			atomic_set_bit(&esl_obj->configuring_state, ESL_UPDATE_COMPLETE_SET_BIT);
			printk("#UPDATE_COMPLETE:1\n");
			if (esl_obj->esl_state == SM_UPDATING &&
			    atomic_test_bit(&esl_obj->basic_state, ESL_SYNCHRONIZED)) {
				printk("enter sync\n");
				esl_obj_l->esl_sm_timeout_update_needed = true;
				bt_esl_state_transition(SM_SYNCHRONIZED);
			} else if (esl_is_configuring_state(esl_obj)) {
				printk("enter configured\n");
				bt_esl_state_transition(SM_CONFIGURED);
			}

			break;
		case OP_READ_SENSOR_DATA:
			LOG_DBG("OP_READ_SENSOR_DATA");
			sensor_idx = command[ESL_OP_READ_SENSOR_IDX];
			LOG_DBG("read sensor_idx %d", sensor_idx);
			if (CONFIG_BT_ESL_SENSOR_MAX == 0) {
				LOG_ERR("No sensor, invalid OP Code");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
			} else if (sensor_idx >= CONFIG_BT_ESL_SENSOR_MAX) {
				LOG_ERR("sensor_Idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_PARAMS;
			} else {
				esl_obj->resp.resp_op = OP_SENSOR_VALUE_STATE_MASK;
				esl_obj->resp.sensor_idx = sensor_idx;
			}

			break;
		case OP_REFRESH_DISPLAY:
			LOG_DBG("OP_REFRESH_DISPLAY");
			display_idx = command[ESL_OP_REFRESH_DISPLAY_IDX];
			if (CONFIG_BT_ESL_DISPLAY_MAX == 0) {
				LOG_ERR("No display, invalid OP Code");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
			} else if (display_idx >= CONFIG_BT_ESL_DISPLAY_MAX) {
				LOG_ERR("display_idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_PARAMS;
			} else if (esl_obj->display_works[display_idx].img_idx ==
				   CONFIG_BT_ESL_IMAGE_MAX) {
				LOG_ERR("image_idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_IMG_IDX;
			} else {
				esl_obj->resp.display_idx = display_idx;
				esl_obj->resp.img_idx = esl_obj->display_works[display_idx].img_idx;
				esl_obj->resp.resp_op = OP_DISPLAY_STATE;
				k_work_reschedule_for_queue(
					&display_work_q, &esl_obj->display_works[display_idx].work,
					K_NO_WAIT);
			}

			break;
		case OP_DISPLAY_IMAGE:
			LOG_DBG("OP_DISPLAY_IMAGE");
			display_idx = command[ESL_OP_DISPLAY_IDX];
			image_idx = command[ESL_OP_DISPLAY_IMAGE_IDX];
			if (esl_obj_l->cb.read_img_size_from_storage) {
				img_size = esl_obj_l->cb.read_img_size_from_storage(image_idx);
			}

			if (CONFIG_BT_ESL_DISPLAY_MAX == 0) {
				LOG_ERR("No display, invalid OP Code");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
			} else if (display_idx >= CONFIG_BT_ESL_DISPLAY_MAX) {
				LOG_ERR("display_idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_PARAMS;
			} else if (image_idx > esl_obj->esl_chrc.max_image_index) {
				LOG_ERR("img_idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				/** 2023/5/23 Workaround ESLS/SR/ECP/BI-12-C
				 *  Errata is under CR
				 *reviewhttps://bluetooth.atlassian.net/jira/software/c/projects/ES/issues/ES-23182
				 *  Should be ERR_INV_IMG_IDX after CR review.
				 **/
				esl_obj->resp.error_code = ERR_INV_PARAMS;
				/* Simulated display ignore image size */
			} else if (img_size == 0 && !IS_ENABLED(CONFIG_ESL_SIMULATE_DISPLAY)) {
				LOG_ERR("The requested image contained no image data");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_IMG_NOT_AVAIL;
			} else {
				esl_obj->resp.resp_op = OP_DISPLAY_STATE;
				esl_obj->resp.display_idx = display_idx;
				esl_obj->resp.img_idx = image_idx;
				esl_obj->display_works[display_idx].img_idx = image_idx;
				k_work_reschedule_for_queue(
					&display_work_q,
					&esl_obj_l->display_works[display_idx].work, K_NO_WAIT);
			}

			break;
		case OP_DISPLAY_TIMED_IMAGE:
			LOG_DBG("OP_DISPLAY_TIMED_IMAGE");
			display_idx = command[ESL_OP_DISPLAY_IDX];
			image_idx = command[ESL_OP_DISPLAY_IMAGE_IDX];
			new_abs_time = sys_get_le32((command + ESL_OP_DISPLAY_TIMED_ABS_TIME_IDX));
			LOG_DBG("new_abs_time %u", new_abs_time);
			offset_time = new_abs_time - esl_get_abs_time();
			if (esl_obj_l->cb.read_img_size_from_storage) {
				img_size = esl_obj_l->cb.read_img_size_from_storage(image_idx);
			}

			if (CONFIG_BT_ESL_DISPLAY_MAX == 0) {
				LOG_ERR("No display, invalid OP Code");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
				break;
			} else if (display_idx >= CONFIG_BT_ESL_DISPLAY_MAX) {
				LOG_ERR("display_idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_PARAMS;
				break;
				/* Simulated display ignore image size */
			} else if (image_idx > esl_obj->esl_chrc.max_image_index) {
				LOG_ERR("image_idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				/** 2023/5/23 Workaround ESLS/SR/ECP/BI-12-C
				 *  Errata is under CR
				 *reviewhttps://bluetooth.atlassian.net/jira/software/c/projects/ES/issues/ES-23182
				 *  Should be ERR_INV_IMG_IDX after CR review.
				 **/
				esl_obj->resp.error_code = ERR_INV_PARAMS;
				break;
			} else if (img_size == 0 && !IS_ENABLED(CONFIG_ESL_SIMULATE_DISPLAY)) {
				LOG_ERR("The requested image contained no image data");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_IMG_NOT_AVAIL;
				break;
			}

			/* TIMED command check if absolute time is zero first */
			if (new_abs_time == 0) {
				/* Delete queue timed display*/
				LOG_DBG("Delete Display 0x%2x in queue", display_idx);
				esl_obj->resp.resp_op = OP_DISPLAY_STATE;
				esl_obj->resp.display_idx = display_idx;
				esl_obj->resp.img_idx = image_idx;
				display_dwork_reset(display_idx);
				bt_esl_basic_state_update();
				break;
			}

			/* TIMED Command check absolute time rational then */
			if (offset_time >= ESL_TIMED_ABS_MAX) {
				LOG_ERR("Implausible Absolute Time new_abs_time %d offset_time %d "
					"esl_get_abs_time %d",
					new_abs_time, offset_time, esl_get_abs_time());
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_IMPLAUSIBLE_ABS;
				break;
			}

			/** Check if ABS time is the same, then we can replace Display config
			 * otherwise, reject TIMED command
			 **/
			if ((esl_obj->display_works[display_idx].start_abs_time == 0) ||
			    (esl_obj->display_works[display_idx].start_abs_time == new_abs_time)) {
				esl_obj->resp.resp_op = OP_DISPLAY_STATE;
				esl_obj->resp.display_idx = display_idx;
				esl_obj->resp.img_idx = image_idx;
				display_dwork_reset(display_idx);
				esl_obj->display_works[display_idx].img_idx = image_idx;
				esl_obj->display_works[display_idx].start_abs_time = new_abs_time;
				LOG_DBG("TIMED DISPLAY abs time anchor %lld abs time %d offset "
					"time %d",
					abs_time_anchor, esl_get_abs_time(), offset_time);
				k_work_reschedule_for_queue(
					&display_work_q, &esl_obj->display_works[display_idx].work,
					K_MSEC(offset_time));
				atomic_set_bit(&esl_obj->basic_state, ESL_PENDING_DISPLAY_UPDATE);
			} else {
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_QUEUE_FULL;
			}

			break;
		case OP_LED_CONTROL:
			LOG_DBG("LED_CONTROL");
			led_idx = command[ESL_OP_LED_CONTROL_LED_IDX];
			led_color = command[ESL_OP_LED_CONTROL_LED_COLOR_IDX];
			if (CONFIG_BT_ESL_LED_MAX == 0) {
				LOG_ERR("No LED, invalid OP Code");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
				break;
			} else if (led_idx >= CONFIG_BT_ESL_LED_MAX) {
				LOG_ERR("LED idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_PARAMS;
				break;
			}

			esl_obj->esl_chrc.led_objs[led_idx].color_brightness = led_color;
			memcpy(esl_obj->esl_chrc.led_objs[led_idx].flash_pattern,
			       (command + ESL_OP_LED_CONTROL_FLASHING_PATTERN),
			       ESL_LED_FLASH_PATTERN_LEN);
			esl_obj->esl_chrc.led_objs[led_idx].bit_off_period =
				command[ESL_OP_LED_CONTROL_BIT_OFF_PERIOD];
			esl_obj->esl_chrc.led_objs[led_idx].bit_on_period =
				command[ESL_OP_LED_CONTROL_BIT_ON_PERIOD];
			unpack_repeat_type_duration(
				(uint8_t *)(command + ESL_OP_LED_CONTROL_REPEAT_TYPE),
				&esl_obj->esl_chrc.led_objs[led_idx].repeat_type,
				&esl_obj->esl_chrc.led_objs[led_idx].repeat_duration);
			LOG_DBG("LED %d, color_brightness 0x%02x", led_idx, led_color);
			esl_led_check_and_set_color(&esl_obj->esl_chrc.led_type[led_idx],
						    led_color);
			LOG_HEXDUMP_DBG(esl_obj->esl_chrc.led_objs[led_idx].flash_pattern,
					ESL_LED_FLASH_PATTERN_LEN, "flash pattern");
			LOG_DBG("off %dms on %dms repeat_type %d repeat_duration %d",
				esl_obj->esl_chrc.led_objs[led_idx].bit_off_period * 2,
				esl_obj->esl_chrc.led_objs[led_idx].bit_on_period * 2,
				esl_obj->esl_chrc.led_objs[led_idx].repeat_type,
				esl_obj->esl_chrc.led_objs[led_idx].repeat_duration);
			esl_obj->resp.resp_op = OP_LED_STATE;
			esl_obj->resp.led_idx = led_idx;
			esl_obj->led_dworks[led_idx].pattern_idx =
				ESL_LED_FLASH_PATTERN_START_BIT_IDX;
			/* cancel previous schedule led work */
			led_dwork_reset(led_idx);
			k_work_schedule_for_queue(&led_work_q, &esl_obj->led_dworks[led_idx].work,
						  K_NO_WAIT);

			break;
		case OP_LED_TIMED_CONTROL:
			LOG_DBG("TIMED_LED_CONTROL");
			led_idx = command[ESL_OP_LED_CONTROL_LED_IDX];
			led_color = command[ESL_OP_LED_CONTROL_LED_COLOR_IDX];

			if (CONFIG_BT_ESL_LED_MAX == 0) {
				LOG_ERR("No LED, invalid OP Code");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
				break;
			} else if (led_idx >= CONFIG_BT_ESL_LED_MAX) {
				LOG_ERR("LED idx is invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_PARAMS;
				break;
			}

			new_abs_time = sys_get_le32((command + ESL_OP_LED_TIMED_ABS_TIME_IDX));
			offset_time = new_abs_time - esl_get_abs_time();
			LOG_DBG("new_abs_time %u", new_abs_time);
			/* TIMED command check if absolute time is zero first */
			if (new_abs_time == 0) {
				/* Delete queue timed display*/
				LOG_DBG("Delete LED 0x%2x in queue", led_idx);
				led_dwork_reset(led_idx);
				esl_obj->resp.resp_op = OP_LED_STATE;
				esl_obj->resp.led_idx = led_idx;
				bt_esl_basic_state_update();
				break;
			}

			/* TIMED Command check absolute time rational then */
			if (offset_time >= ESL_TIMED_ABS_MAX) {
				LOG_ERR("Implausible Absolute Time new_abs_time %d offset_time %d "
					"esl_get_abs_time %d",
					new_abs_time, offset_time, esl_get_abs_time());
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_IMPLAUSIBLE_ABS;
				break;
			}

			/** Check if ABS time is the same, then we can replace LED config
			 * otherwise, reject TIMED command
			 **/
			if ((esl_obj->led_dworks[led_idx].start_abs_time == 0) ||
			    (esl_obj->led_dworks[led_idx].start_abs_time == new_abs_time)) {
				/* cancel previous schedule led work */
				led_dwork_reset(led_idx);
				esl_obj->led_dworks[led_idx].start_abs_time = new_abs_time;
				esl_obj->resp.resp_op = OP_LED_STATE;
				esl_obj->resp.led_idx = led_idx;
				esl_obj->led_dworks[led_idx].pattern_idx =
					ESL_LED_FLASH_PATTERN_START_BIT_IDX;
				bt_esl_basic_state_update();
			} else {
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_QUEUE_FULL;
				break;
			}

			esl_obj->esl_chrc.led_objs[led_idx].color_brightness = led_color;
			memcpy(esl_obj->esl_chrc.led_objs[led_idx].flash_pattern,
			       (command + ESL_OP_LED_CONTROL_FLASHING_PATTERN),
			       ESL_LED_FLASH_PATTERN_LEN);
			esl_obj->esl_chrc.led_objs[led_idx].bit_off_period =
				command[ESL_OP_LED_CONTROL_BIT_OFF_PERIOD];
			esl_obj->esl_chrc.led_objs[led_idx].bit_on_period =
				command[ESL_OP_LED_CONTROL_BIT_ON_PERIOD];
			unpack_repeat_type_duration(
				(uint8_t *)(command + ESL_OP_LED_CONTROL_REPEAT_TYPE),
				&esl_obj->esl_chrc.led_objs[led_idx].repeat_type,
				&esl_obj->esl_chrc.led_objs[led_idx].repeat_duration);
			LOG_DBG("LED %d, color %d", led_idx, led_color);
			esl_led_check_and_set_color(&esl_obj->esl_chrc.led_type[led_idx],
						    led_color);
			LOG_HEXDUMP_DBG(esl_obj->esl_chrc.led_objs[led_idx].flash_pattern,
					ESL_LED_FLASH_PATTERN_LEN, "flash pattern");
			LOG_DBG("off %x on %x repeat_type %d repeat_duration %d abs time %u",
				esl_obj->esl_chrc.led_objs[led_idx].bit_off_period,
				esl_obj->esl_chrc.led_objs[led_idx].bit_on_period,
				esl_obj->esl_chrc.led_objs[led_idx].repeat_type,
				esl_obj->esl_chrc.led_objs[led_idx].repeat_duration,
				esl_obj->led_dworks[led_idx].start_abs_time);
			LOG_DBG("TIMED LED abs time anchor %lld abs time %d offset time %d",
				abs_time_anchor, esl_get_abs_time(), offset_time);
			k_work_schedule_for_queue(&led_work_q, &esl_obj->led_dworks[led_idx].work,
						  K_MSEC(offset_time));
			break;
		default:
			if ((op_code & OP_VENDOR_SPECIFIC_STATE_MASK)) {
				LOG_DBG("vendor-specific TAG");
				/* TODO: Do something about vendor-specific command */
				esl_obj->resp.resp_op = OP_VENDOR_SPECIFIC_STATE_MASK;
			} else {
				LOG_ERR("OP CODE invalid");
				esl_obj->resp.resp_op = OP_ERR;
				esl_obj->resp.error_code = ERR_INV_OPCODE;
			}

			break;
		}
	}

ecp_response:
	/* Finished process ECP, prepare response*/
	ecp_response_handler(esl_obj, info->src);
}

static void ecp_response_handler(struct bt_esls *esl_obj, enum esl_ecp_command_src src)
{
	uint8_t len, sensor_data[ESL_RESPONSE_SENSOR_LEN];
	uint8_t buf[ESL_EAD_PAYLOAD_MAX_LEN];

	LOG_DBG("send ECP resp");

	if (esl_obj->ecp_notify_params.func == NULL) {
		esl_obj->ecp_notify_params.func = ecp_notify_cb;
	}

	if (src == ESL_ECP) {
		/* clear ecp response net_buf if response for ECP */
		net_buf_simple_remove_mem(&ecp_rsp_net_buf, ecp_rsp_net_buf.len);
	}

	buf[0] = esl_obj->resp.resp_op;

	switch (esl_obj->resp.resp_op) {
	case OP_ERR:
		LOG_DBG("OP_ERR");
		buf[1] = esl_obj->resp.error_code;
		esl_obj->ecp_notify_params.len = 2;
		break;
	case OP_LED_STATE:
		LOG_DBG("OP_LED_STATE");
		buf[1] = esl_obj->resp.led_idx;
		esl_obj->ecp_notify_params.len = 2;
		break;
	case OP_BASIC_STATE:
		LOG_DBG("OP_BASIC_STATE");
		bt_esl_basic_state_update();
		buf[1] = LOW_BYTE(esl_obj->basic_state);
		buf[2] = HIGH_BYTE(esl_obj->basic_state);
		esl_obj->ecp_notify_params.len = 3;
		break;
	case OP_DISPLAY_STATE:
		LOG_DBG("OP_DISPLAY_STATE");
		buf[1] = esl_obj->resp.display_idx;
		buf[2] = esl_obj->resp.img_idx;
		esl_obj->ecp_notify_params.len = 3;
		break;
	case OP_SENSOR_VALUE_STATE_MASK:
		LOG_DBG("OP_SENSOR_VALUE");
		/* read some sensor data here */
		if (esl_obj->cb.sensor_control) {
			if (esl_obj->cb.sensor_control(esl_obj->resp.sensor_idx, &len,
						       sensor_data) == -EBUSY) {
				LOG_ERR("Sensor Busy");
				buf[0] = OP_ERR;
				buf[1] = ERR_RETRY;
				esl_obj->ecp_notify_params.len = 2;
			} else {
				buf[0] = ((len & 0x0f) << 4) | OP_SENSOR_VALUE_STATE_MASK;
				buf[1] = esl_obj->resp.sensor_idx;
				memcpy((buf + 2), sensor_data, len);
				esl_obj->ecp_notify_params.len = 2 + len;
			}
		}
		break;
	default:
		LOG_DBG("OP_ERR");
		buf[0] = OP_ERR;
		buf[1] = ERR_UNSPEC;
		esl_obj->ecp_notify_params.len = 2;
		break;
	}

	/* Check if capacity of response TLV exceeds 48 bytes */
	if (src == ESL_PAWR) {
		if ((ecp_rsp_net_buf.len + esl_obj->ecp_notify_params.len) >
		    ESL_SYNC_PKT_PAYLOAD_MAX_LEN) {
			LOG_WRN("ERR_CAP_LIMIT");
			/** Reach Capacify limit , replace last TLV as ERR OP + Capaity Limit error
			 *  code.
			 **/
			net_buf_simple_remove_mem(&ecp_rsp_net_buf, ecp_rsp_last_tlv_len);
			net_buf_simple_add_u8(&ecp_rsp_net_buf, OP_ERR);
			net_buf_simple_add_u8(&ecp_rsp_net_buf, ERR_CAP_LIMIT);
			ecp_rsp_last_tlv_len = 2;
		} else {
			/* Remember last TLV length for rollback */
			net_buf_simple_add_mem(&ecp_rsp_net_buf, buf,
					       esl_obj->ecp_notify_params.len);
			ecp_rsp_last_tlv_len = esl_obj->ecp_notify_params.len;
		}
	} else {
		net_buf_simple_add_mem(&ecp_rsp_net_buf, buf, esl_obj->ecp_notify_params.len);
	}

	esl_obj->ecp_notify_params.data = ecp_rsp_net_buf.data;
	switch (src) {
	case ESL_ECP:
		if (esl_obj->esl_ecp_resp_needed) {
			bt_gatt_notify_cb(NULL, &esl_obj->ecp_notify_params);
		} else {
			LOG_DBG("ECP command has no response ,might be factory_reset");
		}
		break;
	case ESL_PAWR:
		break;
	default:
		LOG_ERR("Unknown ECP command source %d", src);
		break;
	}
}

static ssize_t esl_addr_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	LOG_DBG("Got read req for ESL address CHRC.");

	uint16_t *esl_addr = attr->user_data;
	ssize_t ret_len;

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, esl_addr, sizeof(*esl_addr));
	return ret_len;
}

static ssize_t esl_addr_write(struct bt_conn *conn, struct bt_gatt_attr const *attr,
			      void const *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Got write req for ESL address CHRC.");

	uint16_t *esl_addr = attr->user_data;
	uint16_t new_esl_addr = sys_get_le16(buf);

	/* Check GROUP ID sanity */
	if (CHECK_BIT(new_esl_addr, ESL_ADDR_RFU_BIT)) {
		LOG_WRN("Groupid RFU set, ignore");
		/* Ignore GROUP_ID RFU bit */
		BIT_UNSET(new_esl_addr, ESL_ADDR_RFU_BIT);
	}

	/* Check ESL ID sanity*/
	if (LOW_BYTE(new_esl_addr) == 0xff) {
		LOG_ERR("Broadcast esl_id 0xff is not allowaced");
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	struct bt_esls *esl_obj =
		CONTAINER_OF((uint8_t *)attr->user_data, struct bt_esls, esl_chrc.esl_addr);

	LOG_INF("new esl_addr 0x%04x", new_esl_addr);
	*esl_addr = new_esl_addr;
	atomic_set_bit(&esl_obj->configuring_state, ESL_ADDR_SET_BIT);
	if (!IS_ENABLED(CONFIG_BT_ESL_FORGET_PROVISION_DATA)) {
		esl_settings_addr_save();
	}

	(void)(esl_is_configuring_state(esl_obj));

	return ESL_ADDR_LEN;
}

static ssize_t ap_key_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	struct bt_esl_key_material *ap_sync_key = attr->user_data;
	ssize_t ret_len;

	LOG_DBG("Got read req for AP SYNC KEY CHRC.");
	LOG_HEXDUMP_DBG(ap_sync_key->key_v, EAD_KEY_MATERIAL_LEN, "ap_key_key");

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, ap_sync_key->key_v,
				    EAD_KEY_MATERIAL_LEN);
	return ret_len;
}

static ssize_t ap_key_write(struct bt_conn *conn, struct bt_gatt_attr const *attr, void const *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Got write for AP SYNC KEY.");

	struct bt_esl_key_material *ap_sync_key = attr->user_data;
	struct bt_esls *esl_obj =
		CONTAINER_OF((uint8_t *)attr->user_data, struct bt_esls, esl_chrc.esl_ap_key);
	memcpy(ap_sync_key->key_v, buf, EAD_KEY_MATERIAL_LEN);
	/* Reverse endiness of session key to in advance to avoid change endiness everytime use it
	 */
	sys_mem_swap((void *)ap_sync_key->key.session_key, EAD_SESSION_KEY_LEN);
	LOG_HEXDUMP_DBG((uint8_t *)buf, EAD_KEY_MATERIAL_LEN, "new_ap_sync_key");

	atomic_set_bit(&esl_obj->configuring_state, ESL_AP_SYNC_KEY_SET_BIT);
	if (!IS_ENABLED(CONFIG_BT_ESL_FORGET_PROVISION_DATA)) {
		esl_settings_ap_key_save();
	}

	(void)(esl_is_configuring_state(esl_obj));

	return EAD_KEY_MATERIAL_LEN;
}

static ssize_t esl_rsp_key_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	struct bt_esl_key_material *esl_resp_key = attr->user_data;
	ssize_t ret_len;

	LOG_DBG("Got read req for ESL RESP KEY CHRC.");
	LOG_HEXDUMP_DBG(esl_resp_key->key_v, EAD_KEY_MATERIAL_LEN, "esl_resp_key");

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, esl_resp_key->key_v,
				    EAD_KEY_MATERIAL_LEN);

	return ret_len;
}

static ssize_t esl_rsp_key_write(struct bt_conn *conn, struct bt_gatt_attr const *attr,
				 void const *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Got write for ESL RESP KEY CHRC.");

	struct bt_esl_key_material *esl_resp_key = attr->user_data;
	struct bt_esls *esl_obj =
		CONTAINER_OF((uint8_t *)attr->user_data, struct bt_esls, esl_chrc.esl_rsp_key);

	memcpy(esl_resp_key->key_v, buf, EAD_KEY_MATERIAL_LEN);
	/* Reverse endiness of session key to in advance to avoid change endiness everytime use it
	 */
	sys_mem_swap((void *)esl_resp_key->key.session_key, EAD_SESSION_KEY_LEN);

	LOG_HEXDUMP_DBG((uint8_t *)buf, EAD_KEY_MATERIAL_LEN, "new_esl_resp_key");
	atomic_set_bit(&esl_obj->configuring_state, ESL_RESPONSE_KEY_SET_BIT);
	if (!IS_ENABLED(CONFIG_BT_ESL_FORGET_PROVISION_DATA)) {
		esl_settings_rsp_key_save();
	}

	(void)(esl_is_configuring_state(esl_obj));

	return EAD_KEY_MATERIAL_LEN;
}

static ssize_t abs_time_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint32_t *abs_time = attr->user_data;
	ssize_t ret_len;

	LOG_DBG("Got read req for Absolute Time CHRC.");

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, abs_time, sizeof(*abs_time));
	return sizeof(*abs_time);
}
static ssize_t abs_time_write(struct bt_conn *conn, struct bt_gatt_attr const *attr,
			      void const *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Got write req for Absolute Time CHRC.");

	uint32_t const *new_abs_time = (uint32_t const *)buf;

	LOG_DBG("%u", *new_abs_time);
	printk("#ABS_TIME: %u\n", *new_abs_time);
	esl_abs_time_update(*new_abs_time);
	atomic_set_bit(&esl_obj_l->configuring_state, ESL_ABS_TIME_SET_BIT);
	if (!IS_ENABLED(CONFIG_BT_ESL_FORGET_PROVISION_DATA)) {
		esl_settings_abs_save();
	}

	(void)(esl_is_configuring_state(esl_obj_l));

	return sizeof(*new_abs_time);
}

#if (CONFIG_BT_ESL_SENSOR_MAX > 0)
static ssize_t esl_sensor_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	struct esl_sensor_inf *sensors = attr->user_data;
	ssize_t ret_len;
	uint8_t sensor_idx;
	uint8_t real_sensor_inf_data[sizeof(struct esl_sensor_inf) * CONFIG_BT_ESL_SENSOR_MAX];
	uint16_t real_sensor_inf_size = 0;

	LOG_DBG("Got read req for Sensor Information CHRC.");

	/** Not just pass struct array ,since it will packed additional bytes
	 * when sensor type is Property ID
	 */
	for (sensor_idx = 0; sensor_idx < CONFIG_BT_ESL_SENSOR_MAX; sensor_idx++) {
		/* Mesh property ID */
		if (sensors[sensor_idx].size == 0) {
			memcpy((real_sensor_inf_data + real_sensor_inf_size),
			       (uint8_t *)&sensors[sensor_idx], 3);
			real_sensor_inf_size += 3;

			/* vendor_specific_sensor */
		} else if (sensors[sensor_idx].size == 1) {
			memcpy((real_sensor_inf_data + real_sensor_inf_size),
			       (uint8_t *)&sensors[sensor_idx], 5);
			real_sensor_inf_size += 5;
		}
	}
	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, real_sensor_inf_data,
				    real_sensor_inf_size);

	return ret_len;
}

static void esl_sensor_register(struct bt_esls *esl_obj, const struct bt_esl_init_param *init_param)
{
	LOG_DBG("%s", __func__);
	memcpy(&esl_obj->esl_chrc.sensors, &init_param->sensors, sizeof(esl_obj->esl_chrc.sensors));
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_SENSOR_INF, BT_GATT_CHRC_READ,
			  ESL_GATT_PERM_DEFAULT, esl_sensor_read, NULL, esl_obj->esl_chrc.sensors);
	memcpy(&esl_obj->sensor_data, &init_param->sensor_data, sizeof(init_param->sensor_data));
}

#endif /* BT_ESL_SENSOR_MAX */

#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
static ssize_t esl_image_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	uint8_t *max_image_index = attr->user_data;
	ssize_t ret_len;

	LOG_DBG("Got read req for Image Information CHRC. len %d, off %d", len, offset);

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, max_image_index,
				    sizeof(*max_image_index));
	return sizeof(*max_image_index);
}
static ssize_t esl_disp_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct esl_disp_inf *displays = attr->user_data;
	ssize_t ret_len;

	LOG_DBG("Got read req for Diaply Information CHRC. len %d, off %d", len, offset);

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, displays,
				    DISP_INF_SIZE * CONFIG_BT_ESL_DISPLAY_MAX);
	return ret_len;
}
static void esl_display_register(struct bt_esls *esl_obj,
				 const struct bt_esl_init_param *init_param)
{
	LOG_DBG("%s", __func__);
	memcpy(&esl_obj->esl_chrc.displays, &init_param->displays,
	       sizeof(esl_obj->esl_chrc.displays));
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_DISP_INF, BT_GATT_CHRC_READ, ESL_GATT_PERM_DEFAULT,
			  esl_disp_read, NULL, esl_obj->esl_chrc.displays);
}
static uint32_t obj_cnt;

struct object_creation_data {
	struct bt_ots_obj_size size;
	char *name;
	uint32_t props;
};
static struct object_creation_data *object_being_created;
static int ots_obj_created(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			   const struct bt_ots_obj_add_param *add_param,
			   struct bt_ots_obj_created_desc *created_desc)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint64_t index;

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	if (obj_cnt >= ARRAY_SIZE(esl_obj_l->img_objects)) {
		LOG_ERR("No item from Object pool is available for Object "
			"with %s ID",
			id_str);
		return -ENOMEM;
	}

	if (add_param->size > OBJ_IMG_MAX_SIZE) {
		LOG_ERR("Object pool item is too small for Object with %s ID", id_str);
		return -ENOMEM;
	}

	if (object_being_created) {
		created_desc->name = object_being_created->name;
		created_desc->size = object_being_created->size;
		created_desc->props = object_being_created->props;
	} else {
		index = id - BT_OTS_OBJ_ID_MIN;
		esl_obj_l->img_objects[index].name[0] = '\0';

		created_desc->name = esl_obj_l->img_objects[index].name;
		created_desc->size.alloc = OBJ_IMG_MAX_SIZE;
		BT_OTS_OBJ_SET_PROP_READ(created_desc->props);
		BT_OTS_OBJ_SET_PROP_WRITE(created_desc->props);
		BT_OTS_OBJ_SET_PROP_PATCH(created_desc->props);
	}

	LOG_INF("Object with %s ID has been created", id_str);
	obj_cnt++;

	return 0;
}

static void ots_obj_selected(struct bt_ots *ots, struct bt_conn *conn, uint64_t id)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	LOG_INF("Object with %s ID has been selected", id_str);
}

static ssize_t ots_obj_read(struct bt_ots *ots, struct bt_conn *conn, uint64_t id, void **data,
			    size_t len, off_t offset)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint16_t obj_index = (id - BT_OTS_OBJ_ID_MIN);

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	if (!data) {
		LOG_INF("Object with %s ID has been successfully read", id_str);

		return 0;
	}

	memset(esl_obj_l->img_obj_buf, 0, CONFIG_ESL_IMAGE_BUFFER_SIZE);

	if (esl_obj_l->cb.read_img_from_storage) {
		esl_obj_l->cb.read_img_from_storage(obj_index, esl_obj_l->img_obj_buf, len, offset);
	} else {
		LOG_ERR("no read_img_from_storage cb");
	}

	LOG_DBG("Object with %s ID is being read\n"
		"Offset = %lu, Length = %zu",
		id_str, (long)offset, len);

	return len;
}

static ssize_t ots_obj_write(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			     const void *data, size_t len, off_t offset, size_t rem)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint8_t obj_index = id - BT_OTS_OBJ_ID_MIN;

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	LOG_INF("Object with %s ID %d obj_index is being written "
		"Offset = %lu, Length = %zu, Remaining = %zu",
		id_str, obj_index, (long)offset, len, rem);
#if (CONFIG_ESL_OTS_NVS)
	if (esl_obj_l->cb.buffer_img) {
		esl_obj_l->cb.buffer_img(data, len, offset);
	} else {
		LOG_ERR("no buffer_img cb");
	}
#else
	if (esl_obj_l->cb.write_img_to_storage) {
		esl_obj_l->cb.write_img_to_storage(obj_index, len, offset);
	} else {
		LOG_ERR("no write_img_to_storage cb");
	}
#endif /* (CONFIG_ESL_OTS_NVS) */

	if (rem == 0) {
		LOG_INF("ots_obj_write %d done %ld", obj_index, offset + len);
		esl_obj_l->stored_image_size[obj_index] = offset + len;
#if (CONFIG_ESL_OTS_NVS)

		if (esl_obj_l->cb.write_img_to_storage) {
			esl_obj_l->cb.write_img_to_storage(
				obj_index, esl_obj_l->stored_image_size[obj_index], 0);
		} else {
			LOG_ERR("no write_img_to_storage cb");
		}
#endif
	}

	return len;
}

int ots_obj_cal_checksum(struct bt_ots *ots, struct bt_conn *conn, uint64_t id, off_t offset,
			 size_t len, void **data)
{
	uint8_t obj_index = id - BT_OTS_OBJ_ID_MIN;

	if (obj_index > CONFIG_BT_ESL_IMAGE_MAX) {
		return -ENOENT;
	}

	memset(esl_obj_l->img_obj_buf, 0, CONFIG_ESL_IMAGE_BUFFER_SIZE);
	if (esl_obj_l->cb.read_img_from_storage) {
		esl_obj_l->cb.read_img_from_storage(obj_index, esl_obj_l->img_obj_buf, len, offset);
	} else {
		LOG_ERR("no read_img_from_storage cb");
	}

	*data = &esl_obj_l->img_obj_buf;

	return 0;
}

static void ots_obj_name_written(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
				 const char *cur_name, const char *new_name)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	LOG_DBG("Name for object with %s ID is being changed from '%s' to '%s'", id_str, cur_name,
		new_name);
}

static struct bt_ots_cb ots_callbacks = {
	.obj_created = ots_obj_created,
	.obj_selected = ots_obj_selected,
	.obj_read = ots_obj_read,
	.obj_write = ots_obj_write,
	.obj_name_written = ots_obj_name_written,
	.obj_cal_checksum = ots_obj_cal_checksum,
};

static int ots_create_obj(void)
{
	int err;
	struct object_creation_data obj_data;
	struct bt_ots_obj_add_param param;
	char object_name[15];
	uint32_t cur_size;
	uint32_t alloc_size;

	/* Prepare first object demo data and add it to the instance. */
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_IMAGE_MAX; idx++) {
		if (esl_obj_l->cb.read_img_size_from_storage) {
			/** esl_obj_l->stored_image_size[idx] =
			 *  esl_obj_l->cb.read_img_size_from_storage(idx);
			 **/
			;
		}

		cur_size = esl_obj_l->stored_image_size[idx];
		alloc_size = OBJ_IMG_MAX_SIZE;
		sprintf(object_name, OBJECT_NAME_TEMPLETE, idx);
		(void)memset(&obj_data, 0, sizeof(obj_data));
		__ASSERT(strlen(object_name) <= CONFIG_BT_OTS_OBJ_MAX_NAME_LEN,
			 "Object name length is larger than the allowed maximum of %u",
			 CONFIG_BT_OTS_OBJ_MAX_NAME_LEN);
		(void)strcpy(esl_obj_l->img_objects[idx].name, object_name);
		obj_data.name = esl_obj_l->img_objects[idx].name;
		obj_data.size.cur = cur_size;
		obj_data.size.alloc = alloc_size;
		BT_OTS_OBJ_SET_PROP_READ(obj_data.props);
		BT_OTS_OBJ_SET_PROP_WRITE(obj_data.props);
		BT_OTS_OBJ_SET_PROP_PATCH(obj_data.props);

		object_being_created = &obj_data;

		param.size = alloc_size;
		param.type.uuid.type = BT_UUID_TYPE_16;
		param.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
		err = bt_ots_obj_add(esl_obj_l->ots, &param);
		object_being_created = NULL;
		if (err < 0) {
			LOG_ERR("Failed to add an object to OTS (err: %d)", err);
			return err;
		}
	}

	return 0;
}

static int ots_init_func(void)
{
	int err;
	struct bt_ots_init ots_init;

	esl_obj_l->ots = bt_ots_free_instance_get();
	if (!esl_obj_l->ots) {
		LOG_ERR("Failed to retrieve OTS instance");
		return -ENOMEM;
	}

	/* Configure OTS initialization. */
	(void)memset(&ots_init, 0, sizeof(ots_init));
	BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_WRITE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_PATCH(ots_init.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = &ots_callbacks;

	/* Initialize OTS instance. */
	err = bt_ots_init(esl_obj_l->ots, &ots_init);
	if (err) {
		LOG_ERR("Failed to init OTS (err:%d)", err);
		return err;
	}

	ots_create_obj();

	return 0;
}
void bt_esl_delete_images(void)
{
	int err;
	/* call OTS or storage to delete image*/
	LOG_DBG("");
	if (esl_obj_l->cb.delete_imgs) {
		esl_obj_l->cb.delete_imgs();
		for (size_t idx = 0; idx < CONFIG_BT_ESL_IMAGE_MAX; idx++) {
			err = bt_ots_obj_delete(esl_obj_l->ots, idx + BT_OTS_OBJ_ID_MIN);
			if (err != 0) {
				LOG_ERR("delete obj 0x %08llx failed (err %d)",
					idx + BT_OTS_OBJ_ID_MIN, err);
			}
			obj_cnt--;
		}
	} else {
		LOG_ERR("no delete imgs cb");
	}
}
#endif /* CONFIG_BT_ESL_IMAGE_AVAILABLE */
#if (CONFIG_BT_ESL_LED_MAX > 0)
static ssize_t esl_led_read(struct bt_conn *conn, struct bt_gatt_attr const *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint8_t *leds = attr->user_data;
	ssize_t ret_len;

	LOG_DBG("Got read for LED Information CHRC.");

	ret_len = bt_gatt_attr_read(conn, attr, buf, len, offset, leds, CONFIG_BT_ESL_LED_MAX);
	return ret_len;
}
static void esl_led_register(struct bt_esls *esl_obj, const struct bt_esl_init_param *init_param)
{
	LOG_DBG("%s", __func__);
	memcpy(&esl_obj->esl_chrc.led_type, &init_param->led_type, CONFIG_BT_ESL_LED_MAX);
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_LED_INF, BT_GATT_CHRC_READ, ESL_GATT_PERM_DEFAULT,
			  esl_led_read, NULL, esl_obj->esl_chrc.led_type);
}
#endif /* CONFIG_BT_ESL_LED_MAX */

static ssize_t esl_control_write(struct bt_conn *conn, struct bt_gatt_attr const *attr,
				 void const *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Got write req for Control Point CHRC.");

	uint8_t const *command = (uint8_t const *)buf;
	/* current unused
	 *	uint8_t *cont_point = attr->user_data;
	 */
	struct bt_esls *esl_obj =
		CONTAINER_OF((uint8_t *)attr->user_data, struct bt_esls, cont_point);
	LOG_DBG("esl_control_write len %d", len);
	LOG_HEXDUMP_DBG(buf, len, "esl_control_write");

	/* One control command should be at least 2 byte */
	if (len < 2) {
		LOG_ERR("ESL control command less than 2 bytes");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	esl_obj->ecp_notify_params.attr = attr;
	net_buf_simple_init_with_data(&ecp_net_buf, (void *)command, len);
	esl_ecp_work.src = ESL_ECP;
	k_work_submit(&esl_ecp_work.work);

	return len;
}

static void esl_cont_point_ccc_changed(struct bt_gatt_attr const *attr, uint16_t value)
{
	LOG_DBG("ESL Control Point CCCD has changed.");

	/* Current unused
	 * struct bt_esls *esl_obj =
	 *	CONTAINER_OF((struct _bt_gatt_ccc *)attr->user_data,
	 *		     struct bt_esls, ecp_ccc);
	 */
	if (value == BT_GATT_CCC_NOTIFY) {
		LOG_DBG("Notification for Control Point has been turned "
			"on.");
	} else {
		LOG_DBG("Notification for Control Point has been turned "
			"off.");
	}
}

static void lookup_bond_peer(const struct bt_bond_info *info, void *user_data)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr, sizeof(addr));
	printk("Bonded AP\t\t%s\n", addr);
	if (user_data != NULL) {
		bt_addr_le_copy(user_data, &info->addr);
	}
}
void bt_esl_bond_dump(void *user_data)
{
	bt_foreach_bond(BT_ID_DEFAULT, lookup_bond_peer, user_data);
}

void bt_esl_adv_start(enum esl_adv_mode mode)
{
	LOG_DBG("busy %d", k_work_delayable_busy_get(&esl_adv_work.work));

	esl_adv_work.mode = mode;
	k_work_reschedule(&esl_adv_work.work, ESL_ADV_DELAY);
}

void bt_esl_adv_stop(void)
{
	LOG_DBG("");
	bt_le_adv_stop();

	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		/* Stop led blink pattern */
		led_dwork_reset(INDICATION_LED_IDX);
	}
}

static void esl_advertising_fn(struct k_work *work)
{
	int err;
	bt_addr_le_t peer;
	struct bt_le_adv_param adv_param;
	char addr[BT_ADDR_LE_STR_LEN];
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_adv_work_info *info = CONTAINER_OF(dwork, struct esl_adv_work_info, work);

	/* bonded use direct*/
	bt_esl_bond_dump(&peer);
	switch (info->mode) {
	case ESL_ADV_DEFAULT:
		/* Default slow advertising interval 10s duration 108 minutes */
		LOG_DBG("ESL_ADV_DEFAULT");
		adv_param = *BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
					     0x1F20, 0x1F60, NULL);
		break;
	case ESL_ADV_UNSYNCED:
		LOG_DBG("Directed adv");
		adv_param = *BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
					     BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2,
					     &peer);
		bt_addr_le_to_str(&peer, addr, sizeof(addr));
		LOG_DBG("peer %s", addr);
		break;
	case ESL_ADV_DEMO:
	default:
		/* Less than 1 second */
		LOG_DBG("Undirected adv");
		adv_param = *BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
					     720, 800, NULL);
		break;
	}

	/* LED brlink along with advertising */
	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].color_brightness = 0;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].flash_pattern[0] = 0x5a;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].bit_off_period =
			MAX(0xff, (adv_param.interval_max & 0xff));
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].bit_on_period = 100;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].repeat_type = 1;
		esl_obj_l->esl_chrc.led_objs[INDICATION_LED_IDX].repeat_duration = 120 * 60;
		esl_obj_l->led_dworks[INDICATION_LED_IDX].pattern_idx =
			ESL_LED_FLASH_PATTERN_START_BIT_IDX;

		k_work_schedule_for_queue(
			&led_work_q, &esl_obj_l->led_dworks[INDICATION_LED_IDX].work, K_NO_WAIT);
	}

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err != 0) {
		LOG_ERR("bt_le_adv_start (err %d)", err);
	}
}

static void led_dwork_reset(uint8_t idx)
{
	esl_obj_l->led_dworks[idx].work_enable = false;
	esl_obj_l->led_dworks[idx].active = false;
	esl_obj_l->led_dworks[idx].start_abs_time = 0;
	esl_obj_l->led_dworks[idx].stop_abs_time = 0;
	esl_obj_l->led_dworks[idx].pattern_idx = 0;
	esl_obj_l->led_dworks[idx].repeat_counts = 0;
	k_work_cancel_delayable(&esl_obj_l->led_dworks[idx].work);
	esl_obj_l->cb.led_control(idx, 0, false);
}

static void led_dwork_init(struct bt_esls *esl_obj)
{
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_LED_MAX; idx++) {
		esl_obj->led_dworks[idx].led_idx = idx;
		k_work_init_delayable(&esl_obj->led_dworks[idx].work, led_delay_worker_fn);
		led_dwork_reset(idx);
	}
}

static void display_dwork_reset(uint8_t idx)
{
	esl_obj_l->display_works[idx].start_abs_time = 0;
	esl_obj_l->display_works[idx].active = false;
#if defined(CONFIG_BT_ESL_PTS)
	/* set display has valid image displayed */
	esl_obj_l->display_works[idx].img_idx = 0;
#else
	esl_obj_l->display_works[idx].img_idx = (uint8_t)CONFIG_BT_ESL_IMAGE_MAX;
#endif /* (CONFIG_BT_ESL_PTS)*/
	k_work_cancel_delayable(&esl_obj_l->display_works[idx].work);
}

static void display_dwork_init(struct bt_esls *esl_obj)
{
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_DISPLAY_MAX; idx++) {
		esl_obj_l->display_works[idx].display_idx = idx;
		k_work_init_delayable(&esl_obj->display_works[idx].work, display_delay_worker_fn);
		display_dwork_reset(idx);
	}
}
/* 3.10.2.4 Factory Reset Looks the same with unassoiciated ,but delete image*/
void bt_esl_factory_reset(void)
{
	LOG_DBG("");
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	bt_esl_delete_images();
	ots_create_obj();
#endif
	esl_stop_sync_pawr();
	bt_esl_unassociate();
}

void bt_esl_unassociate(void)
{
	LOG_DBG("");

	bt_esl_disconnect();
	/* ESL tag can only pair and bond to one AP, so unpairing all devices is good*/
	bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	esl_init_provisioned_data();
	esl_settings_delete_provisioned_data();
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_LED_MAX; idx++) {
		led_dwork_reset(idx);
	}

	if (esl_obj_l->cb.display_unassociated) {
		esl_obj_l->cb.display_unassociated(0);
	}

	bt_esl_state_transition(SM_UNASSOCIATED);
}
void bt_esl_disconnect(void)
{
	int err;

	LOG_DBG("");

	/* Diesonnect BLE connection*/
	err = bt_conn_disconnect(esl_obj_l->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		LOG_DBG("disconnect ble fail %d", err);
	}
}

static void esl_pawr_sync_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_pawr_sync_work_info *pawr_work =
		CONTAINER_OF(dwork, struct esl_pawr_sync_work_info, work);
	uint8_t decrypted_data[ESL_ENCRTYPTED_DATA_MAX_LEN];
	size_t esl_payload_len;
	int err;

	esl_payload_len = BT_EAD_DECRYPTED_PAYLOAD_SIZE(pawr_work->buf.len) - AD_HEADER_LEN;
	(void)net_buf_simple_pull_le16(&pawr_work->buf);
	err = bt_ead_decrypt(esl_obj_l->esl_chrc.esl_ap_key.key.session_key,
			     esl_obj_l->esl_chrc.esl_ap_key.key.iv, pawr_work->buf.data,
			     pawr_work->buf.len, decrypted_data);
	if (err != 0) {
		LOG_ERR("Decrypt EAD (err %d)", err);
		return;
	}

	net_buf_simple_init_with_data(&pawr_work->buf, decrypted_data, esl_payload_len);

	if (pawr_work->buf.len != 0) {
		esl_parse_sync_packet(&pawr_work->buf);
		/** Renew esl state timeout work item when received valid ESL sync packet */
		esl_state_timeout_work.mode = ESL_UNSYNCED_TIMEOUT_MODE;
		LOG_INF("Renew unsycned timeout %d seconds", CONFIG_BT_ESL_UNSYNCHRONIZED_TIMEOUT);
		k_work_reschedule(&esl_state_timeout_work.work,
				  K_SECONDS(CONFIG_BT_ESL_UNSYNCHRONIZED_TIMEOUT));
	} else {
		LOG_DBG("Empty PAwR packet");
	}
}

static void woker_init(void)
{
	k_work_init_delayable(&esl_adv_work.work, esl_advertising_fn);
	k_work_init(&esl_ecp_work.work, ecp_command_handler);
	esl_setup_pawr_sync();
	k_work_init_delayable(&esl_pawr_sync_work.work, esl_pawr_sync_work_fn);
	esl_pawr_sync_work.buf = ecp_net_buf;
	led_dwork_init(esl_obj_l);
	display_dwork_init(esl_obj_l);
	k_work_init_delayable(&esl_state_timeout_work.work, esl_state_timeout_work_fn);
	net_buf_simple_init_with_data(&pa_rsp, esl_ad_data, ESL_ENCRTYPTED_DATA_MAX_LEN);
}

void bt_esl_basic_state_update(void)
{
	uint8_t idx;

	atomic_clear_bit(&esl_obj_l->basic_state, ESL_ACTIVE_LED);
	atomic_clear_bit(&esl_obj_l->basic_state, ESL_PENDING_LED_UPDATE);
	atomic_clear_bit(&esl_obj_l->basic_state, ESL_PENDING_DISPLAY_UPDATE);

	for (idx = 0; idx < CONFIG_BT_ESL_LED_MAX; idx++) {
		if (esl_obj_l->led_dworks[idx].active == true) {
			atomic_set_bit(&esl_obj_l->basic_state, ESL_ACTIVE_LED);
		}

		if (esl_obj_l->led_dworks[idx].start_abs_time != 0) {
			LOG_DBG("led idx %02d pending", idx);
			atomic_set_bit(&esl_obj_l->basic_state, ESL_PENDING_LED_UPDATE);
		}
	}

	for (idx = 0; idx < CONFIG_BT_ESL_DISPLAY_MAX; idx++) {
		if (esl_obj_l->display_works[idx].start_abs_time != 0) {
			atomic_set_bit(&esl_obj_l->basic_state, ESL_PENDING_DISPLAY_UPDATE);
		}
	}
}
#if defined(CONFIG_ESL_SHELL) && defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
struct ots_img_object *bt_esl_otc_inst(void)
{
	return esl_obj_l->img_objects;
}
#endif

struct bt_esls *esl_get_esl_obj(void)
{
	return esl_obj_l;
}

int bt_esl_init(struct bt_esls *esl_obj, const struct bt_esl_init_param *init_param)
{
	int err;
	size_t idx;

	/* PAwR workqueue init */
	k_work_queue_start(&pawr_work_q, pawr_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(pawr_wq_stack_area), PAWR_WQ_PRIORITY, NULL);
	/* LED workqueue init */
	k_work_queue_start(&led_work_q, led_wq_stack_area, K_THREAD_STACK_SIZEOF(led_wq_stack_area),
			   LED_WQ_PRIORITY, NULL);
	/* DISPLAY workqueue init */
	k_work_queue_start(&display_work_q, display_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(display_wq_stack_area), DISPLAY_WQ_PRIORITY, NULL);

	bt_conn_cb_register(&conn_callbacks);
	bt_gatt_cb_register(&gatt_callbacks);

	esl_obj->basic_state = ATOMIC_INIT(0);
	esl_obj->configuring_state = ATOMIC_INIT(0);

	esl_obj->cb = init_param->cb;
	int (*module_init[])(void) = {esl_obj->cb.display_init, esl_obj->cb.led_init,
				      esl_obj->cb.sensor_init};
	for (idx = 0; idx < ARRAY_SIZE(module_init); idx++) {
		if (*module_init[idx]) {
			err = (*module_init[idx])();
		}

		if (err != 0) {
			LOG_ERR("Init func %d err %d", idx, err);
			return err;
		}
	}

	esl_obj->busy = false;
	/* Register primary service. */
	BT_GATT_POOL_SVC(&esl_obj->gp, BT_UUID_ESL_SERVICE);

	/* Register ESL Address characteristic. */
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_ESL_ADDR, BT_GATT_CHRC_WRITE, ESL_GATT_PERM_DEFAULT,
			  esl_addr_read, esl_addr_write, &esl_obj->esl_chrc.esl_addr);

	/* Register ESL AP Sync Key characteristic. */
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_AP_SYNC_KEY, BT_GATT_CHRC_WRITE,
			  ESL_GATT_PERM_DEFAULT, ap_key_read, ap_key_write,
			  &esl_obj->esl_chrc.esl_ap_key);

	/* Register ESL AP Response Key characteristic. */
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_ESL_RESP_KEY, BT_GATT_CHRC_WRITE,
			  ESL_GATT_PERM_DEFAULT, esl_rsp_key_read, esl_rsp_key_write,
			  &esl_obj->esl_chrc.esl_rsp_key);

	/* Register Absolute Time characteristic. */
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_ESL_ABS_TIME, BT_GATT_CHRC_WRITE,
			  ESL_GATT_PERM_DEFAULT, abs_time_read, abs_time_write, NULL);
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	/* Register ESL display characteristics. */
	esl_display_register(esl_obj, init_param);

	esl_obj->esl_chrc.max_image_index = CONFIG_BT_ESL_IMAGE_MAX - 1;
	/* Register Image characteristic. */
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_IMG_INF, BT_GATT_CHRC_READ, ESL_GATT_PERM_DEFAULT,
			  esl_image_read, NULL, &esl_obj->esl_chrc.max_image_index);
#endif /* CONFIG_BT_ESL_IMAGE_AVAILABLE */
#if (CONFIG_BT_ESL_SENSOR_MAX > 0)
	/* Register ESL sensor characteristics. */
	esl_sensor_register(esl_obj, init_param);
#endif

	if (IS_ENABLED(CONFIG_BT_ESL_COND_SERVICE_REQUIRED)) {
		atomic_set_bit(&esl_obj->basic_state, ESL_SERVICE_NEEDED_BIT);
	}
#if (CONFIG_BT_ESL_LED_MAX > 0)
	/* Register LED characteristic. */
	esl_led_register(esl_obj, init_param);
#endif
	/* Register Control Point characteristic. */
	BT_GATT_POOL_CHRC(&esl_obj->gp, BT_UUID_ESL_CONT_POINT,
			  BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE |
				  BT_GATT_CHRC_NOTIFY,
			  ESL_GATT_PERM_DEFAULT, NULL, esl_control_write, &esl_obj->cont_point);
	BT_GATT_POOL_CCC(&esl_obj->gp, esl_obj->ecp_ccc, esl_cont_point_ccc_changed,
			 ESL_GATT_PERM_DEFAULT);

	esl_obj_l = esl_obj;
	woker_init();
	/* Set initial state */
	smf_set_initial(SMF_CTX(&sm_obj), &esl_states[SM_UNASSOCIATED]);
	esl_init_provisioned_data();
	/* Init and load settings */
	err = esl_settings_init();
	if (err) {
		LOG_ERR("Failed to init esl settings: %d", err);
		return err;
	}

#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	err = ots_init_func();
	if (err != 0) {
		LOG_ERR("Failed to init OTS (err:%d)", err);
		return err;
	}
#endif
	sync_count = 0;
	response_slot_delay = 0;
	LOG_DBG("bt_esl_init done");
	if (!esl_is_configuring_state(esl_obj_l)) {
		if (esl_obj_l->cb.display_unassociated) {
			esl_obj_l->cb.display_unassociated(0);
		}

	} else {
		if (esl_obj_l->cb.display_associated) {
			esl_obj_l->cb.display_associated(0);
		}

		bt_esl_state_transition(SM_UNSYNCHRONIZED);
	}

	/* Register ESL attributes in GATT database. */
	return bt_gatt_service_register(&esl_obj->gp.svc);
}
