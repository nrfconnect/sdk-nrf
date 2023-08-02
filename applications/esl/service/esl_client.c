/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/ead.h>


#include <zephyr/sys/check.h>
#include <zephyr/sys/printk.h>
#include <bluetooth/scan.h>
#include <zephyr/logging/log.h>
#include "dk_buttons_and_leds.h"

#include "esl_client.h"
#include "esl_internal.h"
#include "esl_client_internal.h"
#include "esl_client_settings.h"
#include "esl_client_tag_storage.h"
#include "esl_dummy_cmd.h"

LOG_MODULE_REGISTER(esl_c, CONFIG_BT_ESL_CLIENT_LOG_LEVEL);
#define ESL_GATT_TIMEOUT K_MSEC(150)
#define ESL_CHRC_MULTI
CREATE_FLAG(read_disp_chrc);
CREATE_FLAG(read_img_chrc);
CREATE_FLAG(read_sensor_chrc);
CREATE_FLAG(read_led_chrc);
CREATE_FLAG(read_chrc);
CREATE_FLAG(write_chrc);
CREATE_FLAG(esl_ots_select_flag);
CREATE_FLAG(esl_ots_write_flag);

K_SEM_DEFINE(esl_write_sem, 1, 1);
K_SEM_DEFINE(esl_ecp_write_sem, 1, 1);
K_SEM_DEFINE(esl_read_sem, 0, 1);
K_SEM_DEFINE(esl_sync_buf_sem, 1, 1);

/** Thread for PAwR **/
#define PAWR_WQ_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO + 1
static K_THREAD_STACK_DEFINE(pawr_wq_stack_area, KB(4));
struct k_work_q pawr_work_q;

enum {
	ESL_C_INITIALIZED,
	ESL_C_ECP_NOTIF_ENABLED,
	ESL_C_ECP_WRITE_PENDING,
	ESL_C_ECP_READ_PENDING
};

struct esl_ap_pawr_work_info {
	struct k_work_delayable work;
	bool onoff;
} esl_ap_pawr_work;

static struct bt_le_ext_adv *adv_pawr;
static struct bt_le_ext_adv_cb pawr_cbs;
static bool is_pawr_init;

/** Thread for Configuring Tag **/
#define AP_CONFIG_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(ap_config_wq_stack_area, KB(4));
struct k_work_q ap_config_work_q;

static struct esl_ap_config_work_info {
	struct k_work_delayable work;
	uint8_t conn_idx;
	struct bt_esl_tag_configure_param param;
} esl_ap_config_work;

static struct esl_ap_disc_work_info {
	struct k_work_delayable work;
	uint8_t conn_idx;
} esl_ap_disc_work;

K_MSGQ_DEFINE(kdisc_worker_msgq, sizeof(uint8_t), CONFIG_BT_ESL_PERIPHERAL_MAX, sizeof(uint8_t));

static struct esl_ap_sync_fill_buf_work_info {
	struct k_work work;
	uint8_t group_id;
	struct bt_le_per_adv_data_request info;
} esl_ap_sync_fill_buf_work;

static struct esl_ap_resp_fill_buf_work_info {
	struct k_work work;
	struct bt_le_per_adv_response_info info;
} esl_ap_resp_fill_buf_work;

static struct esl_tag_load_work_info {
	struct k_work_delayable work;
	const bt_addr_le_t *ble_addr;
	uint8_t conn_idx;
} esl_tag_load_work;

static struct esl_list_tag_work_info {
	struct k_work_delayable work;
	uint8_t conn_idx;
} esl_list_tag_work;

#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
K_SEM_DEFINE(sem_auto_config, 1, 1);
#define MAX_IMAGE_PER_TAG 3
struct image_worker_data {
	uint8_t conn_idx;
	uint8_t img_idx;
	uint8_t tag_img_idx;
};

static struct esl_ots_write_img_work_info {
	struct k_work_delayable work;
} esl_ots_write_img_work;

K_MSGQ_DEFINE(kimage_worker_msgq, sizeof(struct image_worker_data), MAX_IMAGE_PER_TAG,
	      sizeof(uint8_t));
static struct k_thread esl_auto_ap_connect_thread;
static K_THREAD_STACK_DEFINE(esl_auto_ap_connect_thread_stack, KB(4));
#endif /* (CONFIG_BT_ESL_AP_AUTO_MODE) */

#if defined(CONFIG_BT_ESL_LED_INDICATION)
#define BLINK_FREQ_MS	   500
#define INDICATION_LED_NUM 3
static void led_blink_work_handler(struct k_work *work);
struct led_unit_cfg {
	bool blink;
};

static struct led_unit_cfg ind_led[INDICATION_LED_NUM];

K_WORK_DEFINE(led_blink_work, led_blink_work_handler);

/**
 * @brief Submit a k_work on timer expiry.
 */
void led_blink_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&led_blink_work);
}

K_TIMER_DEFINE(led_blink_timer, led_blink_timer_handler, NULL);

/**
 * @brief Periodically invoked by the timer to blink LEDs.
 */
static void led_blink_work_handler(struct k_work *work)
{
	static bool on_phase;

	for (uint8_t i = 0; i < INDICATION_LED_NUM; i++) {
		if (ind_led[i].blink) {
			if (on_phase) {
				dk_set_led(DK_LED1 + i, 1);
			} else {
				dk_set_led(DK_LED1 + i, 0);
			}
		}
	}
	on_phase = !on_phase;
}
#endif /* (CONFIG_BT_ESL_LED_INDICATION) */

struct bt_esl_client *esl_c_obj_l;
static int64_t abs_time_anchor;
static uint8_t ecp_buf[ESL_ECP_COMMAND_MAX_LEN] = {0};

static uint8_t sensor_temp_buf[BT_ATT_MAX_ATTRIBUTE_LEN];
static uint16_t chrc_read_offset;
/* 52840dk uses CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT */
/* 5340dk has to retrieve CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT from netcore*/
#ifndef CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT
#define CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT 5
#endif
static struct bt_le_per_adv_subevent_data_params
	subevent_data_params[CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT];
static struct net_buf_simple subevent_bufs[CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT];
static uint8_t subevent_bufs_data[CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT]
				 [ESL_ENCRTYPTED_DATA_MAX_LEN];

bt_security_t esl_ap_security_level;
bool esl_c_auto_ap_mode = IS_ENABLED(CONFIG_BT_ESL_AP_AUTO_MODE);
uint16_t esl_c_tags_per_group =
	COND_CODE_1(CONFIG_BT_ESL_AP_AUTO_MODE, (CONFIG_BT_ESL_AP_AUTO_TAG_PER_GROUP),
		    (CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER));
uint8_t groups_per_button =
	COND_CODE_1(CONFIG_BT_ESL_AP_AUTO_MODE, (CONFIG_BT_ESL_AP_GROUP_PER_BUTTON), 1);
bool esl_c_write_wo_rsp;
/* 0 = PAST success, 1 = no need PAST, non-zero means error */
static int esl_c_past_unsynced_tag(uint8_t conn_idx);

int esl_c_push_sync_to_controller(uint8_t group_id, uint8_t count)
{
	int err = 0;
	uint8_t current_subevent;
	uint8_t num_subevents = 0;

	LOG_DBG("subevent %d count %d", group_id, count);
	for (size_t i = 0; i < count; i++) {
		/* Deal with subevent wrap-around*/
		current_subevent = (group_id + i) % CONFIG_ESL_CLIENT_MAX_GROUP;
		LOG_DBG("CURRENT subevent %d STATUS %d sync_buf len %d", current_subevent,
			esl_c_obj_l->sync_buf[current_subevent].status,
			esl_c_obj_l->sync_buf[current_subevent].data_len);

		if (esl_c_obj_l->sync_buf[current_subevent].status != SYNC_READY_TO_PUSH) {
			continue;
		}

		LOG_DBG("group %d status %d ready to push", current_subevent,
			esl_c_obj_l->sync_buf[current_subevent].status);

		subevent_data_params[num_subevents].subevent = current_subevent;
		subevent_data_params[num_subevents].response_slot_start = DEFAULT_RESPONSE_START;
		subevent_data_params[num_subevents].response_slot_count =
			CONFIG_ESL_NUM_RESPONSE_SLOTS;
		net_buf_simple_reset(&subevent_bufs[num_subevents]);
		net_buf_simple_add_mem(&subevent_bufs[num_subevents],
				       esl_c_obj_l->sync_buf[current_subevent].data,
				       esl_c_obj_l->sync_buf[current_subevent].data_len);
		subevent_data_params[num_subevents].data = &subevent_bufs[num_subevents];
		LOG_DBG("push_sync %d group %d subevent data_len %d sync_buf data_len %d\n", i,
			current_subevent, subevent_data_params[num_subevents].data->len,
			esl_c_obj_l->sync_buf[current_subevent].data_len);
		esl_c_obj_l->sync_buf[current_subevent].status = SYNC_PUSHED;
		num_subevents++;
	}

	if (num_subevents > 0) {
		err = bt_le_per_adv_set_subevent_data(adv_pawr, num_subevents,
						      subevent_data_params);
		if (err != 0) {
			LOG_ERR("bt_le_pawr_set_subevent_data failed (err %d)", err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		dk_set_led(DK_LED3, 0);
	}

	return err;
}

static void esl_ap_sync_fill_buf_work_fn(struct k_work *work)
{
	struct esl_ap_sync_fill_buf_work_info *sync_buf_work =
		CONTAINER_OF(work, struct esl_ap_sync_fill_buf_work_info, work);
	(void)esl_c_push_sync_to_controller(sync_buf_work->info.start, sync_buf_work->info.count);
}

static void esl_ap_resp_fill_buf_work_fn(struct k_work *work)
{
	struct esl_ap_resp_fill_buf_work_info *resp_buf_work =
		CONTAINER_OF(work, struct esl_ap_resp_fill_buf_work_info, work);
	uint8_t group_id = resp_buf_work->info.subevent;
	uint8_t response_slot = resp_buf_work->info.response_slot;
	struct net_buf_simple buf;
	char decrypted_data[ESL_ENCRTYPTED_DATA_MAX_LEN];
	size_t esl_payload_len;
	int err;

	LOG_INF("group_id %d slot %d buf len %d", group_id, response_slot,
		esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_len);
	esl_c_obj_l->sync_buf[group_id].status = SYNC_RESP_FULL;
	esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_slot = response_slot;
	net_buf_simple_init_with_data(
		&buf, esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].data,
		esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_len);

	/* Check packet rational here to reduce time in pawr recv callback */

	if ((esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_len !=
	     (esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].data[0] + 1)) ||
	    esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].data[1] !=
		    BT_DATA_ENCRYPTED_AD_DATA) {
		LOG_ERR("PAWR response packet is not legit EAD AD data");
		return;
	}

	esl_payload_len = BT_EAD_DECRYPTED_PAYLOAD_SIZE(buf.len) - AD_HEADER_LEN;
	net_buf_simple_pull(&buf, AD_HEADER_LEN);
	err = bt_ead_decrypt(
		esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_key.key.session_key,
		esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_key.key.iv, buf.data,
		buf.len, decrypted_data);
	if (err != 0) {
		LOG_ERR("Decrypt EAD (err %d)", err);
	}

	printk("#RESPONSE:%03d", group_id);
	printk("#SLOT:%d,0x", esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_slot);
	print_hex(decrypted_data, esl_payload_len);

	LOG_HEXDUMP_INF(esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].data,
			esl_c_obj_l->sync_buf[group_id].rsp_buffer[response_slot].rsp_len,
			"response data");
}
/* OTS client related start */
static struct bt_ots_client_cb otc_cb;
void on_obj_selected(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err);

void on_obj_metadata_read(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err,
			  uint8_t metadata_read);

void on_obj_data_written(struct bt_ots_client *ots_inst, struct bt_conn *conn, uint32_t len);
static int otc_handles_assign(struct bt_gatt_dm *dm, struct bt_esl_client *esl_c, uint8_t conn_idx);

void on_obj_selected(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err)
{
	LOG_INF("Current object selected conn_idx %d", bt_conn_index(conn));
	if (err == BT_GATT_OTS_OLCP_RES_OUT_OF_BONDS) {
		char id_str[BT_OTS_OBJ_ID_STR_LEN];

		bt_ots_obj_id_to_str(ots_inst->cur_object.id, id_str, sizeof(id_str));

		LOG_ERR("BT_GATT_OTS_OLCP_RES_OUT_OF_BONDS %d %s", err, id_str);
	}
	err = bt_ots_client_read_object_metadata(ots_inst, conn, BT_OTS_METADATA_REQ_ALL);
	LOG_DBG("bt_ots_client_read_object_metadata (err %d)", err);
}

void on_obj_metadata_read(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err,
			  uint8_t metadata_read)
{
	LOG_INF("Object's meta data: conn_idx %d", bt_conn_index(conn));

	bt_ots_metadata_display(&ots_inst->cur_object, 1);
	SET_FLAG(esl_ots_select_flag);
}

void on_obj_data_written(struct bt_ots_client *ots_inst, struct bt_conn *conn, uint32_t len)
{
	uint8_t conn_idx = bt_conn_index(conn);
	uint64_t tag_img_idx = (esl_c_obj_l->gatt[conn_idx].otc.cur_object.id - BT_OTS_OBJ_ID_MIN);

	LOG_DBG("Object 0x%16llx written", esl_c_obj_l->gatt[conn_idx].otc.cur_object.id);
	printk("#OTS_WRITTEN:1,%02d,%d,%d,0x%08x\n", conn_idx, (uint8_t)tag_img_idx, len,
	       esl_c_obj_l->gatt[conn_idx].last_image_crc);
	SET_FLAG(esl_ots_write_flag);
}

void obj_checksum_calculated(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err,
			     uint32_t crc)
{
	uint8_t conn_idx = bt_conn_index(conn);

	LOG_DBG("Object checksum OACP result (%d) crc 0x%08x last crc 0x%08x\n", err, crc,
		esl_c_obj_l->gatt[conn_idx].last_image_crc);
	if (crc != esl_c_obj_l->gatt[conn_idx].last_image_crc) {
		LOG_ERR("Last transferred image not valid\n");
		printk("Last transferred image not valid\n");
	} else {
		LOG_DBG("Last transferred image is valid\n");
		printk("Last transferred image is valid\n");
	}
}

static void bt_otc_init(void)
{
	otc_cb.obj_selected = on_obj_selected;
	otc_cb.obj_metadata_read = on_obj_metadata_read;
	otc_cb.obj_data_written = on_obj_data_written;
	otc_cb.obj_checksum_calculated = obj_checksum_calculated;
	LOG_DBG("Current object selected callback: %p", otc_cb.obj_selected);
	LOG_DBG("Content callback: %p", otc_cb.obj_data_read);
	LOG_DBG("Metadata callback: %p", otc_cb.obj_metadata_read);
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_PERIPHERAL_MAX; idx++) {
		esl_c_obj_l->gatt[idx].otc.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		esl_c_obj_l->gatt[idx].otc.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		esl_c_obj_l->gatt[idx].otc.cb = &otc_cb;
	}
}

void esl_c_obj_write(uint16_t conn_idx, uint8_t tag_img_idx, uint16_t img_idx)
{
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];

	snprintk(fname, sizeof(fname), OBJECT_NAME_TEMPLETE, img_idx);
	(void)esl_c_obj_write_by_name(conn_idx, tag_img_idx, fname);
}

int esl_c_obj_write_by_name(uint16_t conn_idx, uint8_t tag_img_idx, uint8_t *img_name)
{
	int err;
	size_t size;
	uint64_t obj_id = BT_OTS_OBJ_ID_MIN + tag_img_idx;
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	char fname[CONFIG_MCUMGR_GRP_FS_PATH_LEN];

	bt_ots_obj_id_to_str(obj_id, id_str, sizeof(id_str));
	UNSET_FLAG(esl_ots_select_flag);
	err = bt_ots_client_select_id(&esl_c_obj_l->gatt[conn_idx].otc, esl_c_obj_l->conn[conn_idx],
				      obj_id);
	if (err) {
		LOG_ERR("OTS client select %s failed %d\n", id_str, err);
		printk("#OTS_WRITE:0,%02x,Tag Img IDX out of range", tag_img_idx);
		return -ENOENT;
	}

	WAIT_FOR_FLAG(esl_ots_select_flag);
	snprintk(fname, sizeof(fname), "%s/%s", MNT_POINT, img_name);

	size = esl_c_obj_l->cb.ap_read_img_size_from_storage(fname);
	memset(esl_c_obj_l->img_obj_buf, 0, size);
	err = esl_c_obj_l->cb.ap_read_img_from_storage(fname, esl_c_obj_l->img_obj_buf, &size);
	if (err) {
		LOG_ERR("Image %s not exists\n", img_name);
		printk("OTS_WRITE:0,%s,No Image\n", img_name);
		return -ENXIO;
	}

	esl_c_obj_l->gatt[conn_idx].last_image_crc = crc32_ieee(esl_c_obj_l->img_obj_buf, size);
	UNSET_FLAG(esl_ots_write_flag);
	LOG_INF("conn_idx %d curr_obj alloc %d", conn_idx,
		esl_c_obj_l->gatt[conn_idx].otc.cur_object.size.alloc);
	err = bt_ots_client_write_object_data(&esl_c_obj_l->gatt[conn_idx].otc,
					      esl_c_obj_l->conn[conn_idx], esl_c_obj_l->img_obj_buf,
					      size, 0, BT_OTS_OACP_WRITE_OP_MODE_NONE);
	if (err) {
		LOG_ERR("bt_ots_client_write_object_data (err %d)", err);
		printk("Write Image (err %d)\n", err);
		/* Release wait flag after error */
		SET_FLAG(esl_ots_write_flag);

	} else {
		printk("#OTS_WRITE:1,%d,%d,%s\n", conn_idx, tag_img_idx, img_name);
	}

	return err;
}
/* OTS client related end*/

static void lookup_bond_peer(const struct bt_bond_info *info, void *user_data)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr, sizeof(addr));
	printk("Bonded TAG\t\t%s", addr);
}

void bt_esl_c_bond_dump(void *user_data)
{
	bt_foreach_bond(BT_ID_DEFAULT, lookup_bond_peer, user_data);
}

/* Service Discovery related start */
enum discovery_state {
	DISCOVERY_STATE_START,
	DISCOVERY_STATE_ESL,
	DISCOVERY_STATE_OTS,
	DISCOVERY_STATE_DIS,

	DISCOVERY_STATE_COUNT,
};
static enum discovery_state disc_state;
static void esl_gatt_discover(struct bt_conn *conn);
static void ots_gatt_discover(struct bt_conn *conn);
static void dis_gatt_discover(struct bt_conn *conn);
/* Demo Auto AP mode related start */
#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
static void esl_auto_ap_connect_work_fn(void *p1, void *p2, void *p3)
{
	bt_addr_le_t addr;
	struct tag_addr_node *cur = NULL, *next = NULL;

	while (true) {
		if (esl_c_auto_ap_mode == false) {
			k_msleep(1000);
			continue;
		}

		if (sys_slist_peek_head(&esl_c_obj_l->scanned_tag_addr_list) == NULL) {
			k_msleep(100);
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&esl_c_obj_l->scanned_tag_addr_list, cur, next,
						  node) {
			if (cur && !cur->connected) {
				bt_addr_le_copy(&addr, &cur->addr);
				if (!atomic_test_and_set_bit(&esl_c_obj_l->acl_state,
							     ESL_C_CONNECTING)) {
					esl_c_connect(&addr, ESL_ADDR_BROADCAST);
				}

				k_sem_take(&sem_auto_config, K_SECONDS(60));
			} else {
				continue;
			}
		}
	}
}

static void esl_ots_write_img_work_fn(struct k_work *work)
{
	int ret;
	uint32_t msg_left;
	struct image_worker_data qk_data;

	ret = k_msgq_get(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
	if (ret) {
		LOG_WRN("No more image need to be transferred");
		goto past;
	}

	esl_c_obj_write(qk_data.conn_idx, qk_data.tag_img_idx, qk_data.img_idx);
	WAIT_FOR_FLAG(esl_ots_write_flag);

	msg_left = k_msgq_num_used_get(&kimage_worker_msgq);
	if (msg_left) {
		LOG_INF("Continue to send images");
		k_work_reschedule_for_queue(&ap_config_work_q, &esl_ots_write_img_work.work,
					    K_NO_WAIT);
		return;
	}
past:
	LOG_INF("Finish to send images. Let's PAST.");
	ret = esl_c_past(qk_data.conn_idx);
	if (ret) {
		LOG_ERR("Send PAST %02d (ret %d)", qk_data.conn_idx, ret);
	}
}

static void esl_c_auto_configuring_tag(uint8_t conn_idx)
{
	int ret;
	struct image_worker_data qk_data;

	/**
	 * 0x5479 = Display simulated by LED
	 * 0x5480 = Minew, 128x296 3 colors
	 * 0x5481 = papyr, 200x200 2 colors
	 * 0x5482 = 52833, waveshare gdeh0213b72 250x122 2 colors
	 * 0x5483 = 52833, waveshare HINK-E0213A20-A2 104x212 3 colors
	 * 0x5484 = 21540, Lite LED simulated display
	 * 0x5485 = 52833, Power profiler
	 * 0x5486 = 52832dk, LED simulated display
	 * 0x5487 = thingy52, LED simulated display
	 * 0x5488 = 52833, waveshare gdew042t2 400x300 4 grayscale
	 * 0x5489 = 52840 dongle, LED simulated display
	 * 0x548A = 52832dk, waveshare gdeh0213b72 250x122 2 colors
	 * ...
	 **/

	LOG_INF("Auto configuring ESL_ADDR 0x%04x", esl_c_obj_l->esl_addr_start);

	bt_esl_configure(conn_idx, esl_c_obj_l->esl_addr_start);

	qk_data.conn_idx = conn_idx;
	/* Send different image based on pnp_pid */
	switch (esl_c_obj_l->gatt[conn_idx].esl_device.dis_pnp_id.pnp_pid) {
	case 0x5479:
		LOG_INF("Display Tag Simulated Display");
		LOG_INF("No image");

		break;
	case 0x5480:
		/* Minew use image 4,5,6 */
		LOG_INF("Minew");
		LOG_INF("image 4");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 4;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 5");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 5;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 6");
		qk_data.tag_img_idx = 2;
		qk_data.img_idx = 6;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;
	case 0x5481:
		/* Papyr use image 2,3 */
		LOG_INF("papyr");
		LOG_INF("image 2");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 2;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 3");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 3;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;

	case 0x5482:
		/* Waveshare 2.13" use image 0,1 */
		LOG_INF("52DK+Waveshare");
		LOG_INF("image 0");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 0;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 1");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 1;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;
	case 0x5483:
		/* Waveshare 2.13"b 3 colors use image 7,8 */
		LOG_INF("52DK+Waveshare 2.13 RBW 3 colors");
		LOG_INF("image 7");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 7;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 8");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 8;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;
	case 0x5484:
		/* LED display use image 0 */
		LOG_INF("Lite tag Simulated Dieplay");
		LOG_INF("No image");

		break;
	case 0x5485:
		/* Power profiler display use image 0 */
		LOG_INF("Power Profiler");
		LOG_INF("image 0");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 0;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 1");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 1;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;
	case 0x5486:
		/* Thin Simulate no image use image 0 */
		LOG_INF("Thin tag on nRF52DK");
		LOG_INF("No Image");

		break;
	case 0x5487:
		/* Thiny52 use image 0 */
		LOG_INF("Thingy tag on nRF52DK");
		LOG_INF("No image");

		break;
	case 0x5488:
		LOG_INF("52DK+Waveshare 4.2 inches");
		LOG_INF("image 9");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 9;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image A");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 0xA;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;
	case 0x5489:
		/* Thiny52 no image */
		LOG_INF("52840 dongle tag");
		LOG_INF("No image");

		break;
	case 0x548A:
		/* Thin 2in13 use image 0 and 1 */
		LOG_INF("Thin tag with 2.13 inches on nRF52DK");
		LOG_INF("image 0");
		qk_data.tag_img_idx = 0;
		qk_data.img_idx = 0;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		LOG_INF("image 1");
		qk_data.tag_img_idx = 1;
		qk_data.img_idx = 1;
		ret = k_msgq_put(&kimage_worker_msgq, &qk_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for qk_data");
		}

		break;
	default:
		LOG_WRN("Not predefied tag, should manually configuring");
		break;
	}

	k_work_reschedule_for_queue(&ap_config_work_q, &esl_ots_write_img_work.work, K_NO_WAIT);

	/** Tag number in group should be AP ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER if demo decrypt
	 *on-the-fly or number greater than ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER if demo large amount
	 *of tag controlling.
	 **/
	if (LOW_BYTE(++esl_c_obj_l->esl_addr_start) >=
	    CONFIG_ESL_CLIENT_DEFAULT_ESL_ID + esl_c_tags_per_group) {
		esl_c_obj_l->esl_addr_start =
			(((GROUPID_BYTE(esl_c_obj_l->esl_addr_start) + 1) & 0xff) << 8 |
			 (CONFIG_ESL_CLIENT_DEFAULT_ESL_ID & 0xff));
		LOG_INF("exceed esl_c_tags_per_group %d , change to next group %d",
			esl_c_tags_per_group, GROUPID_BYTE(esl_c_obj_l->esl_addr_start));
	}
}

#endif /* (CONFIG_BT_ESL_AP_AUTO_MODE) */
/* Demo Auto AP mode related end */

static void esl_ap_disc_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_ap_disc_work_info *disc_work =
		CONTAINER_OF(dwork, struct esl_ap_disc_work_info, work);
	/** uint8_t conn_idx;
	 *  int ret;
	 **/

	BUILD_ASSERT((DISCOVERY_STATE_DIS + 1) == DISCOVERY_STATE_COUNT,
		     "DIS must be discovered last - after device is verified");
	LOG_INF("Conn_idx:0x%02X disc_state %d", disc_work->conn_idx, disc_state);
	disc_state++;

	switch (disc_state) {
	case DISCOVERY_STATE_ESL:
		/** Remove conn_idx in queue
		 * ret = k_msgq_get(&kdisc_worker_msgq, &conn_idx, K_NO_WAIT);
		 *  disc_work->conn_idx = conn_idx;
		 **/

		esl_gatt_discover(esl_c_obj_l->conn[disc_work->conn_idx]);
		break;
	case DISCOVERY_STATE_OTS:
		ots_gatt_discover(esl_c_obj_l->conn[disc_work->conn_idx]);
		break;

	case DISCOVERY_STATE_DIS:
		dis_gatt_discover(esl_c_obj_l->conn[disc_work->conn_idx]);
		break;
	case DISCOVERY_STATE_COUNT:
		/* Wait PNPID read finished*/
		k_sem_take(&esl_read_sem, K_SECONDS(10));
		printk("#DISCOVERY: 1,0x%02x\n", disc_work->conn_idx);
		if (IS_ENABLED(CONFIG_BT_ESL_AP_AUTO_PAST_TAG)) {
			if (esl_c_past_unsynced_tag(disc_work->conn_idx) == 0) {
				LOG_INF("Auto PAST succeeded");
				return;
			}
		}

		disc_state = DISCOVERY_STATE_START;

#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
		if (esl_c_auto_ap_mode &&
		    esl_c_obj_l->gatt[disc_work->conn_idx].esl_device.connect_from ==
			    ACL_FROM_SCAN) {
			esl_c_auto_configuring_tag(disc_work->conn_idx);
		}
#endif
		break;
	case DISCOVERY_STATE_START:
	default:
		__ASSERT(false, "disc_state is out of range %d", disc_state);
	}
}

static void esl_discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	uint8_t conn_idx;
	struct bt_esl_client *esl_c = context;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	LOG_INF("ESL Service discovery completed");
	bt_gatt_dm_data_print(dm);
	/* see if Tag has already in our stored tag */
	conn_idx = ble_addr_to_conn_idx(bt_conn_get_dst(conn));
	LOG_DBG("ble_addr_to_conn_idx %d", conn_idx);

	bt_esl_handles_assign(dm, esl_c, conn_idx);
	bt_esl_c_subscribe_receive(esl_c, conn_idx);

	bt_gatt_dm_data_release(dm);
	esl_ap_disc_work.conn_idx = bt_conn_index(conn);
	k_work_reschedule(&esl_ap_disc_work.work, K_NO_WAIT);
}

static void ots_discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	int err;
	uint8_t conn_idx;
	struct bt_esl_client *esl_c = context;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	LOG_INF("OTS Service discovery completed");
	bt_gatt_dm_data_print(dm);
	/* see if dst addr has already in our stored tag */
	conn_idx = bt_conn_index(conn);

	LOG_DBG("%s conn_idx %d", __func__, conn_idx);
	err = otc_handles_assign(dm, esl_c, conn_idx);

	bt_gatt_dm_data_release(dm);
	esl_ap_disc_work.conn_idx = bt_conn_index(conn);
	k_work_reschedule(&esl_ap_disc_work.work, K_NO_WAIT);
}

#define VID_POS_IN_PNP_ID sizeof(uint8_t)
#define PID_POS_IN_PNP_ID (VID_POS_IN_PNP_ID + sizeof(uint16_t))

static uint8_t dis_read_attr(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
			     const void *data, uint16_t length)
{
	const uint8_t *data_ptr = data;
	uint8_t conn_idx = bt_conn_index(conn);

	if (data != NULL) {
		esl_c_obj_l->gatt[conn_idx].esl_device.dis_pnp_id.pnp_vid =
			sys_get_le16(&data_ptr[VID_POS_IN_PNP_ID]);
		esl_c_obj_l->gatt[conn_idx].esl_device.dis_pnp_id.pnp_pid =
			sys_get_le16(&data_ptr[PID_POS_IN_PNP_ID]);
		LOG_INF("DIS PNPID:VID: 0x%04x PID: 0x%04x",
			sys_get_le16(&data_ptr[VID_POS_IN_PNP_ID]),
			sys_get_le16(&data_ptr[PID_POS_IN_PNP_ID]));
		k_sem_give(&esl_read_sem);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void dis_discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	int err;
	uint8_t conn_idx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	const struct bt_gatt_dm_attr *attr;

	LOG_INF("DIS Service discovery completed");
	bt_gatt_dm_data_print(dm);
	conn_idx = bt_conn_index(conn);
	LOG_DBG("bt_conn_index %d", conn_idx);
	attr = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DIS_PNP_ID);
	if (!attr) {
		LOG_WRN("DIS_PNP_ID Characteristic not found");
	}

	static struct bt_gatt_read_params rp = {
		.func = dis_read_attr,
		.handle_count = 1,
		.single.offset = 0,
	};

	esl_c_obj_l->gatt[conn_idx].gatt_handles.handles[HANDLE_DIS_PNPID] = attr->handle + 1;
	rp.single.handle = attr->handle + 1;

	/* Need to know PNPID before auto configuring */
	err = bt_gatt_read(conn, &rp);
	if (err != 0) {
		LOG_ERR("DIS PNP ID GATT read problem (err:%d)", err);
	}

	bt_gatt_dm_data_release(dm);
	esl_ap_disc_work.conn_idx = conn_idx;
	k_work_reschedule(&esl_ap_disc_work.work, K_NO_WAIT);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	/* Except ESL service, every service is optional */
	if (disc_state == DISCOVERY_STATE_ESL) {
		LOG_ERR("Service ESL not found. Disconnect");
		peer_disconnect(conn);
	} else {
		LOG_INF("Service (%d) not found", disc_state);
		esl_ap_disc_work.conn_idx = bt_conn_index(conn);
		k_work_reschedule(&esl_ap_disc_work.work, K_NO_WAIT);
	}
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_ERR("Error while discovering %d service GATT database: (%d)", disc_state, err);
}

static struct bt_gatt_dm_cb esl_discovery_cb = {
	.completed = esl_discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static struct bt_gatt_dm_cb ots_discovery_cb = {
	.completed = ots_discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

struct bt_gatt_dm_cb dis_discovery_cb = {
	.completed = dis_discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static void esl_gatt_discover(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_dm_start(conn, BT_UUID_ESL_SERVICE, &esl_discovery_cb, esl_c_obj_l);
	if (err != 0) {
		LOG_ERR("could not start the esl discovery procedure, conn_idx %d, error "
			"code: %d",
			bt_conn_index(conn), err);
	}
}

static void ots_gatt_discover(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_dm_start(conn, BT_UUID_OTS, &ots_discovery_cb, esl_c_obj_l);
	if (err != 0) {
		LOG_ERR("could not start the OTS discovery procedure, conn_idx %d error "
			"code: %d",
			bt_conn_index(conn), err);
	}
}
static void dis_gatt_discover(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_dm_start(conn, BT_UUID_DIS, &dis_discovery_cb, esl_c_obj_l);
	if (err != 0) {
		LOG_ERR("could not start the DIS discovery procedure, conn_idx %d error "
			"code: %d",
			bt_conn_index(conn), err);
	}
}

static void discovery_work_init(void)
{
	k_work_init_delayable(&esl_ap_disc_work.work, esl_ap_disc_work_fn);
	disc_state = DISCOVERY_STATE_START;
}

void esl_discovery_start(uint8_t conn_idx)
{
	disc_state = DISCOVERY_STATE_START;
	esl_ap_disc_work.conn_idx = conn_idx;
	k_work_schedule(&esl_ap_disc_work.work, K_NO_WAIT);
}

/* Service Discovery end */
static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (err != 0) {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	} else {
		LOG_DBG("ATT MTU %d ", bt_gatt_get_mtu(conn));
	}

	esl_discovery_start(bt_conn_index(conn));
}

static void scan_filter_no_match(struct bt_scan_device_info *device_info, bool connectable)
{
	int err;
	struct bt_conn *conn;
	char addr[BT_ADDR_LE_STR_LEN];

	if (device_info->recv_info->adv_type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		int conn_idx = esl_c_get_free_conn_idx();

		bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
		LOG_DBG("Direct advertising received from %s\n", addr);
		esl_c_scan(false, false);

		err = bt_conn_le_create(device_info->recv_info->addr, BT_CONN_LE_CREATE_CONN,
					device_info->conn_param, &conn);

		if (!err) {
			esl_c_obj_l->conn[conn_idx] = bt_conn_ref(conn);
			bt_conn_unref(conn);
		}
	}
}
static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable)
{
	int ret;

	if (esl_c_obj_l->scan_one_shot == true) {
		LOG_DBG("Scan oneshot");
		esl_c_scan(false, false);
	}

	ret = esl_c_scanned_tag_add(device_info->recv_info->addr);

	if (ret < 0) {
		LOG_ERR("Add tag to scanned list failed (%d)", ret);
	}
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("scan Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_DBG("Found %s", addr);
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;
	uint8_t conn_idx = bt_conn_index(conn);
	struct tag_addr_node *tag;
	const bt_addr_le_t *peer;

	peer = bt_conn_get_dst(conn);
	bt_addr_le_to_str(peer, addr, sizeof(addr));
	atomic_clear_bit(&esl_c_obj_l->acl_state, ESL_C_CONNECTING);
	if (conn_err) {
		LOG_INF("Failed to connect to %s (err %d)", addr, conn_err);
		printk("Connected Failed:Conn_idx:%02d\n", conn_idx);
		bt_conn_unref(esl_c_obj_l->conn[conn_idx]);
#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
		k_sem_give(&sem_auto_config);
#endif
		goto scan;
	}

	printk("Connected:Conn_idx:%02d\n", conn_idx);
	LOG_INF("Connected: %s", addr);
	esl_tag_load_work.ble_addr = peer;
	esl_tag_load_work.conn_idx = conn_idx;
	k_work_reschedule(&esl_tag_load_work.work, K_NO_WAIT);

	tag = esl_c_get_tag_addr(-1, peer);
	if (tag) {
		LOG_DBG("Get tag %p set connected", tag);
		tag->connected = true;
	} else {
		LOG_WRN("tag addr not found");
	}

	if (IS_ENABLED(CONFIG_BT_ESL_SECURITY_ENABLED)) {
		err = bt_conn_set_security(conn, esl_ap_security_level);
		if (err != 0) {
			LOG_WRN("Failed to set security: %d", err);
		}
	} else {
		static struct bt_gatt_exchange_params exchange_params;

		exchange_params.func = exchange_func;
		err = bt_gatt_exchange_mtu(conn, &exchange_params);
		if (err != 0) {
			/** See zephyr\include\zephyr\bluetooth\att.h Error codes for Error response
			 * PDU
			 **/
			LOG_WRN("MTU exchange failed (err %d)", err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		dk_set_led(DK_LED2, 1);
	}

scan:
	if (atomic_test_and_clear_bit(&esl_c_obj_l->acl_state, ESL_C_SCAN_PENDING)) {
		esl_c_scan(true, esl_c_obj_l->scan_one_shot);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	uint8_t conn_idx = bt_conn_index(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	atomic_clear_bit(&esl_c_obj_l->acl_state, ESL_C_CONNECTING);
	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);
	/* Release all flag*/
	SET_FLAG(read_disp_chrc);
	SET_FLAG(read_img_chrc);
	SET_FLAG(read_sensor_chrc);
	SET_FLAG(read_led_chrc);
	SET_FLAG(read_chrc);
	SET_FLAG(write_chrc);
	SET_FLAG(esl_ots_select_flag);
	SET_FLAG(esl_ots_write_flag);

	memset(esl_c_obj_l->gatt[conn_idx].esl_device.esl_rsp_key.key_v, 0, EAD_KEY_MATERIAL_LEN);
	bt_conn_unref(esl_c_obj_l->conn[conn_idx]);
	printk("Disconnected:Conn_idx:0x%02X (reason 0x%02x)\n", bt_conn_index(conn), reason);
#if defined(CONFIG_BT_ESL_DEMO_SECURITY)
	bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
#endif
	esl_c_scanned_tag_remove_with_ble_addr(bt_conn_get_dst(conn));
#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
	k_sem_give(&sem_auto_config);
	if (atomic_test_and_clear_bit(&esl_c_obj_l->acl_state, ESL_C_SCAN_PENDING)) {
		esl_c_scan(true, esl_c_obj_l->scan_one_shot);
	}

#endif
	esl_c_obj_l->gatt[conn_idx].esl_device.past_needed = false;
	esl_c_obj_l->gatt[conn_idx].esl_device.connect_from = ACL_NOT_CONNECTED;
	esl_c_obj_l->conn[conn_idx] = NULL;
	bt_ots_client_unregister(0);
	atomic_clear_bit(&esl_c_obj_l->gatt[conn_idx].notify_state, ESL_C_ECP_NOTIF_ENABLED);

	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		dk_set_led(DK_LED2, 0);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("security_changed: %s level %u err %d", addr, level, err);
	if (esl_ap_security_level == BT_SECURITY_L1) {
		LOG_DBG("this is for IOP ESLP/ESL/CFG/BI-01-I test ,return");
		return;
	}

	if (err != BT_SECURITY_ERR_SUCCESS) {
		err = bt_conn_set_security(conn, esl_ap_security_level | BT_SECURITY_FORCE_PAIR);
		LOG_WRN("Security failed: %s level %u err %d", addr, level, err);
		LOG_WRN("force pair");
	} else {
		static struct bt_gatt_exchange_params exchange_params;

		LOG_INF("Security changed: %s level %u", addr, level);
		exchange_params.func = exchange_func;
		err = bt_gatt_exchange_mtu(conn, &exchange_params);
		if (err != 0) {
			/** See zephyr\include\zephyr\bluetooth\att.h for error response PDU
			 **/
			LOG_WRN("MTU exchange failed (err %d)", err);
		}
	}
}
static struct bt_conn_cb conn_callbacks = {
	.connected = connected, .disconnected = disconnected, .security_changed = security_changed};
void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_DBG("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

uint32_t esl_c_get_abs_time(void)
{
	return (k_uptime_get_32() - abs_time_anchor) % UINT32_MAX;
}

void esl_list_tag_work_fn(struct k_work *work)
{
	esl_c_list_scanned(false);
	k_work_reschedule(&esl_list_tag_work.work,
			  K_MSEC(CONFIG_BT_ESL_SCAN_REPORT_INTERVAL * MSEC_PER_SEC));
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match, scan_connecting_error,
		scan_connecting);

void esl_c_auto_ap_toggle(bool onoff)
{
	esl_c_auto_ap_mode = onoff;
	printk("New ESL_AP_AUTO_MODE %d\n", esl_c_auto_ap_mode);
	if (esl_c_auto_ap_mode) {
		esl_c_scan(true, false);
	} else {
		esl_c_scan(false, false);
	}
}

static int esl_c_scan_init(void)
{
	int err;
	uint8_t mode = BT_SCAN_UUID_FILTER;
	struct bt_scan_init_param scan_init = {
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
		.connect_if_match = 0,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_ESL_SERVICE);
	if (err != 0) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(mode, false);
	if (err != 0) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}

void esl_c_abs_timer_update(uint32_t new_abs_time)
{
	abs_time_anchor = k_uptime_get_32() - new_abs_time;
	LOG_DBG("abs_time_anchor %lld new_abs_time %d abs_time %d\n", abs_time_anchor, new_abs_time,
		esl_c_get_abs_time());
}

static void ecp_rsp_handler(uint8_t conn_idx, struct bt_esl_client *esl_obj, const uint8_t *command,
			    uint16_t len)
{
	uint8_t const *ecp_command = esl_c_obj_l->gatt[conn_idx].ecp_write_params.data;

	LOG_DBG("received ECP resp for OP 0x%02x, ecp len %d\n", ecp_command[0],
		esl_c_obj_l->gatt[conn_idx].ecp_write_params.length);
	switch (command[0]) {
	case OP_ERR:
		LOG_DBG("OP_ERR\n");
		printk("#ECP_ERR:0x%02x,%s\n", command[1], esl_rcp_error_to_string(command[1]));
		if (command[1] == ERR_RETRY) {
			LOG_WRN("TODO:AUTO retry\n");
		}

		break;
	case OP_LED_STATE:
		LOG_DBG("OP_LED_STATE\n");
		printk("#LED:%02d\n", command[1]);
		break;
	case OP_BASIC_STATE:
		LOG_DBG("OP_BASIC_STATE\n");
		esl_obj->gatt[conn_idx].basic_state = sys_get_le16(command + 1);
		printk("#BASIC_STATE:1,0x%04x\n", esl_c_obj_l->gatt[conn_idx].esl_device.esl_addr);
		print_basic_state((atomic_t)esl_obj->gatt[conn_idx].basic_state);
		if (ecp_command[0] == OP_UNASSOCIATE) {
			bt_c_esl_post_unassociate(esl_c_obj_l->pending_unassociated_tag);
		}

		break;
	case OP_DISPLAY_STATE:
		LOG_DBG("OP_DISPLAY_STATE\n");
		printk("#DISPLAY:%02d,%02d\n", command[1], command[2]);
		break;
	default:
		if (command[0] & OP_SENSOR_VALUE_STATE_MASK) {
			LOG_DBG("OP_SENSOR_VALUE\n");
			/* get some sensor data here */
			dump_sensor_data(command, len);
		} else if (command[0] & OP_VENDOR_SPECIFIC_STATE_MASK) {
			LOG_DBG("OP_VENDOR_SPECIFIC_STATE_MASK\n");
			LOG_HEXDUMP_DBG(command, len, "vender value");
			printk("#VENDOR:");
			print_hex(command, (len - 2));
		} else {
			LOG_DBG("OP_ERR\n");
			printk("#ECP_ERR:0x%02x,%s\n", command[1],
			       esl_rcp_error_to_string(command[1]));
		}

		break;
	}
}

static uint8_t on_ecp_received(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			       const void *data, uint16_t length)
{
	uint8_t conn_idx = bt_conn_index(conn);

	if (!data) {
		LOG_DBG("[UNSUBSCRIBED]");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[NOTIFICATION] data %p length %u", data, length);
	ecp_rsp_handler(conn_idx, esl_c_obj_l, data, length);

	return BT_GATT_ITER_CONTINUE;
}

static void esl_c_write_attr(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_write_params *write_params)
{
	LOG_DBG("esl_c_write_attr %d err 0x%02x\n", write_params->length, err);
	SET_FLAG(write_chrc);
}

int bt_esl_handles_assign(struct bt_gatt_dm *dm, struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	const struct bt_gatt_dm_attr *gatt_service_attr = bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_ESL_SERVICE)) {
		LOG_ERR("ESL SERVICE UUID not match\n");
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from ESL service. conn_idx %d", conn_idx);
	/* 0x0000 denotes an invalid handle */
	memset(&esl_c->gatt[conn_idx].gatt_handles, BT_ATT_INVALID_HANDLE,
	       sizeof(esl_c->gatt[conn_idx].gatt_handles));

	/* ESL ADDRESS Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_ESL_ADDR);
	if (!gatt_chrc) {
		LOG_ERR("Missing ESL ADDRESS characteristic.");
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_ESL_ADDR);
	if (!gatt_desc) {
		LOG_ERR("Missing ESL ADDRESS value descriptor in characteristic.");
		return -EINVAL;
	}
	esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ADDR] = gatt_desc->handle;
	LOG_DBG("Found handle for ESL ADDRESS characteristic. 0x%x",
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ADDR]);

	/* AP SYNC KEY Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AP_SYNC_KEY);
	if (!gatt_chrc) {
		LOG_ERR("Missing AP SYNC KEY characteristic.");
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_AP_SYNC_KEY);
	if (!gatt_desc) {
		LOG_ERR("Missing AP SYNC KEY value descriptor in characteristic.");
		return -EINVAL;
	}
	esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_AP_SYNC_KEY] = gatt_desc->handle;
	LOG_DBG("Found handle for AP SYNC KEY characteristic. 0x%x",
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_AP_SYNC_KEY]);

	/* ESL RESP KEY Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_ESL_RESP_KEY);
	if (!gatt_chrc) {
		LOG_ERR("Missing ESL RESPONSE KEY characteristic.");
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_ESL_RESP_KEY);
	if (!gatt_desc) {
		LOG_ERR("Missing ESL RESPONSE KEY value descriptor in characteristic.");
		return -EINVAL;
	}
	esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_RESP_KEY] = gatt_desc->handle;
	LOG_DBG("Found handle for ESL RESPONSE KEY characteristic. 0x%x",
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_RESP_KEY]);

	/* ESL ABSOLUTE TIME Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_ESL_ABS_TIME);
	if (!gatt_chrc) {
		LOG_ERR("Missing ESL ABS TIME characteristic.");
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_ESL_ABS_TIME);
	if (!gatt_desc) {
		LOG_ERR("Missing ESL ABS TIME value descriptor in characteristic.");
		return -EINVAL;
	}
	esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ABS_TIME] = gatt_desc->handle;
	LOG_DBG("Found handle for ESL ABS TIME characteristic. 0x%x",
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ABS_TIME]);

	/* ESL Control Point Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_ESL_CONT_POINT);
	if (!gatt_chrc) {
		LOG_ERR("Missing ESL Control Point characteristic.");
		return -EINVAL;
	}
	/* ESL Control Point */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_ESL_CONT_POINT);
	if (!gatt_desc) {
		LOG_ERR("Missing ESL Control Point value descriptor in characteristic.");
		return -EINVAL;
	}
	esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP] = gatt_desc->handle;
	LOG_DBG("Found handle for ESL Control Point characteristic. 0x%x",
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP]);

	/* ESL Control Point CCC */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);

	if (!gatt_desc) {
		LOG_ERR("Missing ESL Control Point CCC in characteristic.");
		return -EINVAL;
	}
	esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP_CCC] = gatt_desc->handle;
	LOG_DBG("Found handle for CCC of ESL Control Point characteristic. 0x%x",
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP_CCC]);

	/* Conditional Characteristics */
	/* Display Information characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DISP_INF);
	if (!gatt_chrc) {
		LOG_DBG("No Display Information characteristic found");

	} else {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DISP_INF);
		if (!gatt_desc) {
			LOG_ERR("Found handle for Information characteristic value found.");
			return -EINVAL;
		}
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_DISP_INF] = gatt_desc->handle;
		LOG_DBG("Found handle for Display Information characteristic. 0x%x",
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_DISP_INF]);
	}

	/* Image Information characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_IMG_INF);
	if (!gatt_chrc) {
		LOG_DBG("No Image Information characteristic found");
	} else {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_IMG_INF);
		if (!gatt_desc) {
			LOG_ERR("No Image Information characteristic value found.");
			return -EINVAL;
		}
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_IMAGE_INF] = gatt_desc->handle;
		LOG_DBG("Found handle for Image Information characteristic. 0x%x",
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_IMAGE_INF]);
	}

	/* Sensor Information characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_SENSOR_INF);
	if (!gatt_chrc) {
		LOG_DBG("No Sensor Information characteristic found");
	} else {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_SENSOR_INF);
		if (!gatt_desc) {
			LOG_ERR("No Sensor Information characteristic value found.");
			return -EINVAL;
		}
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_SENSOR_INF] = gatt_desc->handle;
		LOG_DBG("Found handle for Sensor Information characteristic. 0x%x",
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_SENSOR_INF]);
	}

	/* LED Information characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_LED_INF);
	if (!gatt_chrc) {
		LOG_DBG("No LED Information characteristic found");
	} else {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_LED_INF);
		if (!gatt_desc) {
			LOG_ERR("No LED Information characteristic value found.");
			return -EINVAL;
		}
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_LED_INF] = gatt_desc->handle;
		LOG_DBG("Found handle for LED Information characteristic. 0x%x",
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_LED_INF]);
	}
	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_ESL_SERVICE)) {
		LOG_ERR("ESL SERVICE UUID not match\n");
		return -ENOTSUP;
	}

	/* Assign connection instance. */
	esl_c->conn[conn_idx] = bt_gatt_dm_conn_get(dm);
	return 0;
}
static int otc_handles_assign(struct bt_gatt_dm *dm, struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	const struct bt_gatt_dm_attr *gatt_service_attr = bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;
	struct bt_gatt_subscribe_params *sub_params_1 = NULL;
	struct bt_gatt_subscribe_params *sub_params_2 = NULL;
	int err = 0;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_OTS)) {
		LOG_ERR("ESL SERVICE UUID not match\n");
		return -ENOTSUP;
	}
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_FEATURE);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_FEATURE characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_FEATURE);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_FEATURE value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.feature_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_NAME);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_NAME characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_NAME);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_NAME value descriptor in characteristic.");
	}
	esl_c->gatt[conn_idx].otc.obj_name_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_TYPE);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_TYPE characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_TYPE);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_TYPE value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.obj_type_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_SIZE);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_SIZE characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_SIZE);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_SIZE value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.obj_size_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_ID);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_ID characteristic.");
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_ID);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_ID value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.obj_id_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_PROPERTIES);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_PROPERTIES characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_PROPERTIES);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_PROPERTIES value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.obj_properties_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_ACTION_CP);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_ACTION_CP characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_ACTION_CP);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_ACTION_CP value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.oacp_handle = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_OTS_LIST_CP);
	if (!gatt_chrc) {
		LOG_WRN("Missing BT_UUID_OTS_LIST_CP characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_OTS_LIST_CP);
	if (!gatt_desc) {
		LOG_WRN("Missing BT_UUID_OTS_LIST_CP value descriptor in characteristic.");
	}

	esl_c->gatt[conn_idx].otc.olcp_handle = gatt_desc->handle;

	sub_params_1 = &esl_c->gatt[conn_idx].otc.oacp_sub_params;
	sub_params_1->disc_params = &esl_c->gatt[conn_idx].otc.oacp_sub_disc_params;
	if (sub_params_1) {
		/* With ccc_handle == 0 it will use auto discovery */
		sub_params_1->ccc_handle = 0;
		sub_params_1->end_handle = esl_c->gatt[conn_idx].otc.end_handle;
		sub_params_1->value = BT_GATT_CCC_INDICATE;
		sub_params_1->value_handle = esl_c->gatt[conn_idx].otc.oacp_handle;
		sub_params_1->notify = bt_ots_client_indicate_handler;
		bt_gatt_subscribe(bt_gatt_dm_conn_get(dm), sub_params_1);
	}
	sub_params_2 = &esl_c->gatt[conn_idx].otc.olcp_sub_params;
	sub_params_2->disc_params = &esl_c->gatt[conn_idx].otc.olcp_sub_disc_params;
	if (sub_params_2) {
		/* With ccc_handle == 0 it will use auto discovery */
		sub_params_2->ccc_handle = 0;
		sub_params_2->end_handle = esl_c->gatt[conn_idx].otc.end_handle;
		sub_params_2->value = BT_GATT_CCC_INDICATE;
		sub_params_2->value_handle = esl_c->gatt[conn_idx].otc.olcp_handle;
		sub_params_2->notify = bt_ots_client_indicate_handler;
		bt_gatt_subscribe(bt_gatt_dm_conn_get(dm), sub_params_2);
	}

	err = bt_ots_client_register(&esl_c->gatt[conn_idx].otc);

	if (err) {
		LOG_ERR("Setup complete for OTS bt_ots_client_register failed (err %d)", err);
	} else {
		LOG_DBG("Setup complete for OTS");
	}

	return err;
}

static uint8_t esl_c_read_disp_inf(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params, const void *data,
				   uint16_t length)
{
	uint8_t conn_idx = bt_conn_index(conn);

	if (err != 0) {
		LOG_ERR("Problem reading GATT (err:%" PRIu8 ")", err);
		SET_FLAG(read_disp_chrc);
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("esl_c_read_disp_inf len %d offset %d\n", length, chrc_read_offset);
	if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "esl_c_read_disp_inf");
		memcpy((uint8_t *)esl_c_obj_l->gatt[conn_idx].esl_device.displays +
			       chrc_read_offset,
		       data, length);
		chrc_read_offset += length;
		return BT_GATT_ITER_CONTINUE;
	}

	esl_c_obj_l->gatt[conn_idx].esl_device.display_count = chrc_read_offset / DISP_INF_SIZE;
	chrc_read_offset = 0;
	SET_FLAG(read_disp_chrc);
	return BT_GATT_ITER_STOP;
}

static uint8_t esl_c_read_img_inf(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t length)
{
	uint8_t *max_img = (uint8_t *)data;
	uint8_t conn_idx = bt_conn_index(conn);

	/** Retrieve ESL Client module context.
	 * struct bt_esl_client *esl_c;
	 * esl_c = CONTAINER_OF(params, struct bt_esl_client, esl_read_params);
	 **/

	if (err != 0) {
		LOG_ERR("Problem reading GATT (err:%" PRIu8 ")", err);
		SET_FLAG(read_img_chrc);
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("esl_c_read_img_inf len %d img_idx %d\n", length, *max_img);

	LOG_HEXDUMP_DBG(data, length, "esl_c_read_img_inf");
	memcpy(&esl_c_obj_l->gatt[conn_idx].esl_device.max_image_index, data, length);
	SET_FLAG(read_img_chrc);
	return BT_GATT_ITER_STOP;
}

static uint8_t esl_c_read_sensor_inf(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_read_params *params, const void *data,
				     uint16_t length)
{
	uint8_t conn_idx = bt_conn_index(conn);

	/** Retrieve ESL Client module context.
	 * struct bt_esl_client *esl_c;
	 * esl_c = CONTAINER_OF(params, struct bt_esl_client, esl_read_params);
	 **/

	if (err != 0) {
		LOG_ERR("Problem reading GATT (err:%" PRIu8 ")", err);
		SET_FLAG(read_sensor_chrc);
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("esl_c_read_sensor_inf len %d offset %d\n", length, chrc_read_offset);
	if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "esl_c_read_sensor_inf");
		memcpy(sensor_temp_buf + chrc_read_offset, data, length);
		chrc_read_offset += length;
		return BT_GATT_ITER_CONTINUE;
	}

	esl_c_obj_l->gatt[conn_idx].esl_device.sensor_count = parse_sensor_info_from_raw(
		esl_c_obj_l->gatt[conn_idx].esl_device.sensors, sensor_temp_buf, chrc_read_offset);
	chrc_read_offset = 0;
	SET_FLAG(read_sensor_chrc);
	return BT_GATT_ITER_STOP;
}

static uint8_t esl_c_read_led_inf(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t length)
{
	uint8_t conn_idx = bt_conn_index(conn);

	/** Retrieve ESL Client module context.
	 * struct bt_esl_client *esl_c;
	 * esl_c = CONTAINER_OF(params, struct bt_esl_client, esl_read_params);
	 **/

	if (err != 0) {
		LOG_ERR("Problem reading GATT (err:%" PRIu8 ")", err);
		SET_FLAG(read_led_chrc);
		return BT_GATT_ITER_STOP;
	}
	LOG_INF("esl_c_read_led_inf len %d offset %d\n", length, chrc_read_offset);

	if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "esl_c_read_led_inf");
		memcpy((uint8_t *)(esl_c_obj_l->gatt[conn_idx].esl_device.led_type +
				   chrc_read_offset),
		       data, length);
		chrc_read_offset += length;
		return BT_GATT_ITER_CONTINUE;
	}

	esl_c_obj_l->gatt[conn_idx].esl_device.led_count = chrc_read_offset;
	chrc_read_offset = 0;
	SET_FLAG(read_led_chrc);
	return BT_GATT_ITER_STOP;
}

void peer_disconnect(struct bt_conn *conn)
{
	int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err && (err != -ENOTCONN)) {
		LOG_ERR("Cannot disconnect peer (err:%d)", err);
	}
}

int bt_esl_c_subscribe_receive(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;

	CHECKIF(!check_ble_connection(esl_c->conn[conn_idx])) {
		LOG_ERR("no valid connection ,quit subscribe TAG ecp");
		return -EBUSY;
	}

	if (atomic_test_and_set_bit(&esl_c->gatt[conn_idx].notify_state, ESL_C_ECP_NOTIF_ENABLED)) {
		LOG_ERR("Already subscribe ECP");
		return -EALREADY;
	}

	esl_c->gatt[conn_idx].ecp_notif_params.notify = on_ecp_received;
	esl_c->gatt[conn_idx].ecp_notif_params.value = BT_GATT_CCC_NOTIFY;
	esl_c->gatt[conn_idx].ecp_notif_params.value_handle =
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP];
	esl_c->gatt[conn_idx].ecp_notif_params.ccc_handle =
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP_CCC];
	atomic_set_bit(esl_c->gatt[conn_idx].ecp_notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].ecp_notif_params);
	if (err != 0) {
		LOG_ERR("Subscribe failed (err %d)", err);
		atomic_clear_bit(&esl_c->gatt[conn_idx].notify_state, ESL_C_ECP_NOTIF_ENABLED);
	} else {
		LOG_DBG("bt_esl_c_subscribe_receive [SUBSCRIBED]");
	}

	LOG_INF("ECP Subscribed %d (err %d)", conn_idx, err);
	return err;
}

int esl_c_read_disp(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err = 0;

	/* Read Display Information characteristic */
	esl_c->gatt[conn_idx].esl_read_params.func = esl_c_read_disp_inf;
	esl_c->gatt[conn_idx].esl_read_params.handle_count = 1;
	esl_c->gatt[conn_idx].esl_read_params.single.offset = 0;
	esl_c->gatt[conn_idx].esl_read_params.single.handle =
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_DISP_INF];
	err = bt_gatt_read(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].esl_read_params);
	if (err != 0) {
		LOG_ERR("esl_c_read_disp GATT read problem (err:%d)", err);
	}

	return 0;
}

static uint8_t esl_c_read_multi(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params, const void *data,
				uint16_t length)
{
	uint8_t conn_idx = bt_conn_index(conn);

	if (err != 0) {
		LOG_ERR("Problem reading GATT (err:%" PRIu8 ")", err);
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("esl_c_read_multi len %d offset %d", length, chrc_read_offset);
	if (length != 0) {
		LOG_HEXDUMP_DBG(data, length, "esl_c_read_multi");
		switch (chrc_read_offset) {
		/* DISPLAY INFO */
		case 0:
			memcpy((uint8_t *)esl_c_obj_l->gatt[conn_idx].esl_device.displays, data,
			       length);
			esl_c_obj_l->gatt[conn_idx].esl_device.display_count =
				length / DISP_INF_SIZE;
			break;
		/* Image INFO */
		case 1:
			memcpy(&esl_c_obj_l->gatt[conn_idx].esl_device.max_image_index, data,
			       length);
			break;
		/* Sensor INFO */
		case 2:
			memcpy(sensor_temp_buf, data, length);
			esl_c_obj_l->gatt[conn_idx].esl_device.sensor_count =
				parse_sensor_info_from_raw(
					esl_c_obj_l->gatt[conn_idx].esl_device.sensors,
					sensor_temp_buf, length);

			break;
		/* LED INFO */
		case 3:
			memcpy((uint8_t *)(esl_c_obj_l->gatt[conn_idx].esl_device.led_type), data,
			       length);
			esl_c_obj_l->gatt[conn_idx].esl_device.led_count = length;
			break;
		/* PNPID */
		case 4:
			const uint8_t *data_ptr = data;

			esl_c_obj_l->gatt[conn_idx].esl_device.dis_pnp_id.pnp_vid =
				sys_get_le16(&data_ptr[VID_POS_IN_PNP_ID]);
			esl_c_obj_l->gatt[conn_idx].esl_device.dis_pnp_id.pnp_pid =
				sys_get_le16(&data_ptr[PID_POS_IN_PNP_ID]);
			LOG_DBG("DIS PNPID:VID: 0x%04x PID: 0x%04x",
				sys_get_le16(&data_ptr[VID_POS_IN_PNP_ID]),
				sys_get_le16(&data_ptr[PID_POS_IN_PNP_ID]));
			break;
		default:
			LOG_ERR("Read count is out of range %d", chrc_read_offset);
			SET_FLAG(read_chrc);
			return BT_GATT_ITER_STOP;
		}

		chrc_read_offset++;
		return BT_GATT_ITER_CONTINUE;
	}

	chrc_read_offset = 0;
	SET_FLAG(read_chrc);
	return BT_GATT_ITER_STOP;
}

/** This API assumes that sum length of information charateristic of Tag would not exceed ATT_MTU
 * -1. So that we could use 4.8.5 Read Multiple Variable Length Characteristic Values
 * ATT_READ_MULTIPLE_VARIABLE_REQ to save transaction nunmber in demo scenario.
 **/
int bt_esl_c_read_chrc_multi(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
#define READ_COUNT 5
	int err;
	struct bt_esl_client_handles gatt_handles = esl_c->gatt[conn_idx].gatt_handles;
	uint16_t handles[READ_COUNT] = {
		gatt_handles.handles[HANDLE_DISP_INF], gatt_handles.handles[HANDLE_IMAGE_INF],
		gatt_handles.handles[HANDLE_SENSOR_INF], gatt_handles.handles[HANDLE_LED_INF],
		gatt_handles.handles[HANDLE_DIS_PNPID]};

	CHECKIF(!check_ble_connection(esl_c->conn[conn_idx])) {
		LOG_ERR("no valid connection ,quit read chrc");
		return -EACCES;
	}

	esl_c->gatt[conn_idx].esl_read_params.handle_count = READ_COUNT;
	esl_c->gatt[conn_idx].esl_read_params.func = esl_c_read_multi;
	esl_c->gatt[conn_idx].esl_read_params.multiple.handles = handles;
	esl_c->gatt[conn_idx].esl_read_params.multiple.variable = true;
	esl_c->gatt[conn_idx].esl_read_params.chan_opt = BT_ATT_CHAN_OPT_NONE;

	memset(sensor_temp_buf, 0, BT_ATT_MAX_ATTRIBUTE_LEN);

	err = bt_gatt_read(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].esl_read_params);
	LOG_DBG("bt_gatt_read (err %d)", err);
	if (err) {
		SET_FLAG(read_chrc);
	}

	return err;
}

/** This API assumes that sum length of information charateristic of Tag would exceed ATT_MTU -1.
 *  So that we should use 4.8.3 Read Long Characteristic Values for those Tag who might have
 *  information exceeds ATT_MTU -1.
 **/
int bt_esl_c_read_chrc(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;

	CHECKIF(!check_ble_connection(esl_c->conn[conn_idx])) {
		LOG_ERR("no valid connection ,quit read chrc");
		return -EACCES;
	}

	if (esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_DISP_INF] != BT_ATT_INVALID_HANDLE) {
		UNSET_FLAG(read_disp_chrc);
		/* Read Display Information characteristic */
		chrc_read_offset = 0;
		err = esl_c_read_disp(esl_c, conn_idx);
		if (err != 0) {
			LOG_ERR("esl_c_read_disp GATT read problem (err:%d)", err);
		}
		WAIT_FOR_FLAG(read_disp_chrc);

	} else {
		LOG_WRN("Display element is not available on Tag");
	}

	if (esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_IMAGE_INF] != BT_ATT_INVALID_HANDLE) {
		UNSET_FLAG(read_img_chrc);
		/* Read Image Information characteristic */
		chrc_read_offset = 0;
		esl_c->gatt[conn_idx].esl_read_params.func = esl_c_read_img_inf;
		esl_c->gatt[conn_idx].esl_read_params.handle_count = 1;
		esl_c->gatt[conn_idx].esl_read_params.single.offset = 0;
		esl_c->gatt[conn_idx].esl_read_params.single.handle =
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_IMAGE_INF];

		err = bt_gatt_read(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].esl_read_params);
		if (err != 0) {
			LOG_ERR("GATT read problem (err:%d)", err);
		}
		WAIT_FOR_FLAG(read_img_chrc);

	} else {
		LOG_WRN("Image info is not available on Tag");
	}

	if (esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_SENSOR_INF] !=
	    BT_ATT_INVALID_HANDLE) {
		UNSET_FLAG(read_sensor_chrc);
		/* Read Sensor Information characteristic */
		memset(sensor_temp_buf, 0, BT_ATT_MAX_ATTRIBUTE_LEN);
		chrc_read_offset = 0;
		esl_c->gatt[conn_idx].esl_read_params.func = esl_c_read_sensor_inf;
		esl_c->gatt[conn_idx].esl_read_params.handle_count = 1;
		esl_c->gatt[conn_idx].esl_read_params.single.offset = 0;
		esl_c->gatt[conn_idx].esl_read_params.single.handle =
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_SENSOR_INF];

		err = bt_gatt_read(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].esl_read_params);
		if (err != 0) {
			LOG_ERR("GATT read problem (err:%d)", err);
		}
		WAIT_FOR_FLAG(read_sensor_chrc);

	} else {
		LOG_WRN("Sensor element is not available on Tag");
	}

	if (esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_LED_INF] != BT_ATT_INVALID_HANDLE) {
		UNSET_FLAG(read_led_chrc);
		/* Read LED Information characteristic */
		chrc_read_offset = 0;
		esl_c->gatt[conn_idx].esl_read_params.func = esl_c_read_led_inf;
		esl_c->gatt[conn_idx].esl_read_params.handle_count = 1;
		esl_c->gatt[conn_idx].esl_read_params.single.offset = 0;
		esl_c->gatt[conn_idx].esl_read_params.single.handle =
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_LED_INF];

		err = bt_gatt_read(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].esl_read_params);
		if (err != 0) {
			LOG_ERR("GATT read problem (err:%d)", err);
		}
		WAIT_FOR_FLAG(read_led_chrc);
	} else {
		LOG_WRN("LED element is not available on Tag");
	}

	esl_c_dump_chrc(&esl_c_obj_l->gatt[conn_idx].esl_device);
	return 0;
}

/* Prerequisitere connect and discover service */
void bt_esl_configure(uint8_t conn_idx, uint16_t esl_addr)
{
	esl_ap_config_work.conn_idx = conn_idx;
	esl_ap_config_work.param.esl_addr = esl_addr;
	k_work_schedule_for_queue(&ap_config_work_q, &esl_ap_config_work.work, K_NO_WAIT);
}

int bt_esl_c_write_esl_addr(uint8_t conn_idx, uint16_t esl_addr)
{
	int err;
	uint8_t data[ESL_ADDR_LEN];

	sys_put_le16(esl_addr, data);
	LOG_DBG("0x%04x %s_rsp", esl_c_obj_l->gatt[conn_idx].esl_device.esl_addr,
		(esl_c_write_wo_rsp ? "wo" : "with"));
	if (esl_c_write_wo_rsp) {
		err = bt_gatt_write_without_response(
			esl_c_obj_l->conn[conn_idx],
			esl_c_obj_l->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ADDR],
			(void *)data, ESL_ADDR_LEN, false);
		if (err != 0) {
			LOG_ERR("GATT write problem (err:%d)", err);
		}
	} else {
		esl_c_obj_l->gatt[conn_idx].esl_write_params.func = esl_c_write_attr;
		/* Write ESL Address characteristic */
		esl_c_obj_l->gatt[conn_idx].esl_write_params.data = (void *)data;
		esl_c_obj_l->gatt[conn_idx].esl_write_params.length = ESL_ADDR_LEN;
		esl_c_obj_l->gatt[conn_idx].esl_write_params.handle =
			esl_c_obj_l->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ADDR];
		UNSET_FLAG(write_chrc);
		err = bt_gatt_write(esl_c_obj_l->conn[conn_idx],
				    &esl_c_obj_l->gatt[conn_idx].esl_write_params);
		if (err != 0) {
			LOG_ERR("GATT write problem (err:%d)", err);
		}
		WAIT_FOR_FLAG(write_chrc);
	}
	return err;
}

static int bt_esl_c_ecp_command_send(struct bt_esl_client *esl_c, uint8_t *command,
				     enum ESL_OP_CODE op_code, uint8_t conn_idx)
{
	int err = 0;

	CHECKIF(!check_ble_connection(esl_c->conn[conn_idx])) {
		LOG_ERR("no valid connection, quit send ecp command");
		return err;
	}

	esl_c->gatt[conn_idx].ecp_write_params.handle =
		esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP];
	esl_c->gatt[conn_idx].ecp_write_params.func = esl_c_write_attr;
	esl_c->gatt[conn_idx].ecp_write_params.data = ecp_buf;
	ecp_buf[0] = op_code;
	ecp_buf[1] = LOW_BYTE(esl_c->gatt[conn_idx].esl_device.esl_addr);
	switch (op_code) {
	case OP_PING:
		LOG_DBG("OP_PING\n");
		esl_c->gatt[conn_idx].ecp_write_params.length = 2;
		break;
	case OP_UNASSOCIATE:
		LOG_DBG("OP_UNASSOCIATE\n");
		atomic_clear_bit(&esl_c->gatt[conn_idx].basic_state, ESL_SYNCHRONIZED);
		esl_c->gatt[conn_idx].ecp_write_params.length = 2;
		break;
	case OP_SERVICE_RESET:
		LOG_DBG("OP_SERVICE_RESET\n");
		esl_c->gatt[conn_idx].ecp_write_params.length = 2;
		break;
	case OP_FACTORY_RESET:
		LOG_DBG("OP_FACTORY_RESET\n");
		esl_c->gatt[conn_idx].ecp_write_params.length = 2;
		break;
	case OP_UPDATE_COMPLETE:
		LOG_DBG("OP_UPDATE_COMPLETE");
		esl_c->gatt[conn_idx].ecp_write_params.length = 2;
		break;
	case OP_READ_SENSOR_DATA:
		LOG_DBG("OP_READ_SENSOR_DATA\n");
		ecp_buf[2] = command[0];
		esl_c->gatt[conn_idx].ecp_write_params.length = 3;
		break;
	case OP_REFRESH_DISPLAY:
		LOG_DBG("OP_REFRESH_DISPLAY\n");
		ecp_buf[2] = command[0];
		esl_c->gatt[conn_idx].ecp_write_params.length = 3;
		break;
	case OP_DISPLAY_IMAGE:
		LOG_DBG("OP_DISPLAY_IMAGE\n");
		ecp_buf[2] = command[0];
		ecp_buf[3] = command[1];
		esl_c->gatt[conn_idx].ecp_write_params.length = 4;
		break;
	case OP_DISPLAY_TIMED_IMAGE:
		LOG_DBG("OP_DISPLAY_TIMED_IMAGE\n");
		memcpy((ecp_buf + ESL_OP_HEADER_LEN), command, 6);
		esl_c->gatt[conn_idx].ecp_write_params.length = 8;
		break;
	case OP_LED_CONTROL:
		LOG_DBG("OP_LED_CONTROL\n");
		memcpy((ecp_buf + ESL_OP_HEADER_LEN), command, 11);
		esl_c->gatt[conn_idx].ecp_write_params.length = 13;
		break;
	case OP_LED_TIMED_CONTROL:
		LOG_DBG("OP_TIMED_LED_CONTROL\n");
		memcpy((ecp_buf + ESL_OP_HEADER_LEN), command, 15);
		esl_c->gatt[conn_idx].ecp_write_params.length = 17;
		break;
	default:
		if ((op_code & OP_VENDOR_SPECIFIC_STATE_MASK)) {
			LOG_DBG("vendor-specific TAG");
			/** TODO: Do something about vendor-specific command
			 * e.g: Modify price of image
			 **/
		} else {
			LOG_ERR("OP CODE RFU\n");
			return -1;
		}
	}

	if (esl_c_write_wo_rsp) {
		err = bt_gatt_write_without_response(
			esl_c->conn[conn_idx],
			esl_c->gatt[conn_idx].gatt_handles.handles[HANDLE_ECP], (void *)ecp_buf,
			esl_c->gatt[conn_idx].ecp_write_params.length, false);
		if (err != 0) {
			LOG_ERR("GATT ECP write with rsp problem (err:%d)", err);
		}
	} else {
		UNSET_FLAG(write_chrc);
		LOG_HEXDUMP_DBG(ecp_buf, esl_c->gatt[conn_idx].ecp_write_params.length,
				"bt_esl_c_ecp_command_send");
		err = bt_gatt_write(esl_c->conn[conn_idx], &esl_c->gatt[conn_idx].ecp_write_params);
		WAIT_FOR_FLAG(write_chrc);
		if (err != 0) {
			LOG_ERR("GATT ECP write problem (err:%d)", err);
		}
	}

	return err;
}

int bt_esl_c_led_control(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t led_idx,
			 struct led_obj *led, bool timed, uint32_t abs_time)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	LOG_DBG("led_idx %d", led_idx);
	command[0] = led_idx;
	command[1] = led->color_brightness;
	memcpy((command + ESL_OP_HEADER_LEN), led->flash_pattern, ESL_LED_FLASH_PATTERN_LEN);

	command[ESL_OP_LED_CONTROL_BIT_OFF_PERIOD - ESL_OP_HEADER_LEN] = led->bit_off_period;
	command[ESL_OP_LED_CONTROL_BIT_ON_PERIOD - ESL_OP_HEADER_LEN] = led->bit_on_period;
	pack_repeat_type_duration((command + ESL_OP_LED_CONTROL_REPEAT_TYPE - ESL_OP_HEADER_LEN),
				  led->repeat_type, led->repeat_duration);
	LOG_DBG("repeat_type %d repeat_duration 0x%02x low %d high %d cmd %d %d", led->repeat_type,
		led->repeat_duration, LOW_BYTE(led->repeat_duration),
		HIGH_BYTE(led->repeat_duration),
		command[ESL_OP_LED_CONTROL_REPEAT_TYPE - ESL_OP_HEADER_LEN],
		command[ESL_OP_LED_CONTROL_REPEAT_DURATION - ESL_OP_HEADER_LEN]);
	LOG_DBG("cmd %d %d 11th %d %d", ESL_OP_LED_CONTROL_REPEAT_TYPE - ESL_OP_HEADER_LEN,
		ESL_OP_LED_CONTROL_REPEAT_DURATION - ESL_OP_HEADER_LEN, command[9], command[10]);
	if (timed) {
		memcpy((command + ESL_OP_LED_TIMED_ABS_TIME_IDX - ESL_OP_HEADER_LEN), &abs_time,
		       sizeof(abs_time));
		err = bt_esl_c_ecp_command_send(esl_c, command, OP_LED_TIMED_CONTROL, conn_idx);
	} else {
		err = bt_esl_c_ecp_command_send(esl_c, command, OP_LED_CONTROL, conn_idx);
	}

	return err;
}

int bt_esl_c_display_control(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t disp_idx,
			     uint8_t img_idx, bool timed, uint32_t abs_time)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	LOG_DBG("disp_idx %d", disp_idx);
	command[0] = disp_idx;
	command[1] = img_idx;

	if (timed) {
		memcpy((command + ESL_OP_DISPLAY_TIMED_ABS_TIME_IDX - ESL_OP_HEADER_LEN), &abs_time,
		       sizeof(abs_time));
		err = bt_esl_c_ecp_command_send(esl_c, command, OP_DISPLAY_TIMED_IMAGE, conn_idx);
	} else {
		err = bt_esl_c_ecp_command_send(esl_c, command, OP_DISPLAY_IMAGE, conn_idx);
	}
	return err;
}

int bt_esl_c_refresh_display(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t disp_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	LOG_DBG("disp_idx %d", disp_idx);
	command[0] = disp_idx;
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_REFRESH_DISPLAY, conn_idx);
	return err;
}

int bt_esl_c_factory_reset(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_FACTORY_RESET, conn_idx);
	bt_c_esl_post_unassociate(conn_idx);

	return err;
}

int bt_esl_c_ping_op(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_PING, conn_idx);
	return err;
}

int bt_esl_c_service_reset(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_SERVICE_RESET, conn_idx);
	return err;
}

int bt_esl_c_read_sensor(struct bt_esl_client *esl_c, uint8_t conn_idx, uint8_t sensor_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	command[0] = sensor_idx;
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_READ_SENSOR_DATA, conn_idx);
	return err;
}

int bt_esl_c_update_complete(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_UPDATE_COMPLETE, conn_idx);
	return err;
}

static void esl_c_sync_buffer_init(void)
{
	LOG_INF("size sync_buf %d total %d", sizeof(struct bt_esl_sync_buffer),
		sizeof(struct bt_esl_sync_buffer) * CONFIG_ESL_CLIENT_MAX_GROUP);
	memset(esl_c_obj_l->sync_buf, 0,
	       sizeof(struct bt_esl_sync_buffer) * CONFIG_ESL_CLIENT_MAX_GROUP);
}

/**
 * Push ESL SYNC packet format B0FF00F0411000000032FA001F
 **/
static void esl_c_sync_buffer_dummy_cmd_init(void)
{
	size_t idx;

	esl_c_sync_buffer_init();
	for (idx = 0; idx < CONFIG_ESL_CLIENT_MAX_GROUP; idx++) {
		esl_dummy_ap_ad_data(0, idx);
	}

	for (idx = 0; idx < CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT; idx++) {
		memset(subevent_bufs_data, 0,
		       CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_BUFFER_COUNT *
			       ESL_ENCRTYPTED_DATA_MAX_LEN);
		net_buf_simple_init_with_data(&subevent_bufs[idx], &subevent_bufs_data[idx],
					      ESL_ENCRTYPTED_DATA_MAX_LEN);
		LOG_DBG("subevent_bufs[%d]->data %p size %d len %d", idx,
			(void *)&subevent_bufs[idx].data, subevent_bufs[idx].size,
			subevent_bufs[idx].len);
	}
}

static void setup_pawr_adv(void)
{
	int err;
	/* Create a non-connectable non-scannable advertising set */
	struct bt_le_adv_param *adv_param;
	struct bt_le_per_adv_param param;

	if (is_pawr_init) {
		return;
	}

	/**
	 * Increase interval of extended to prevent collide with PAWR
	 * when enable extended adv for sniffer.
	 * 0x3700 = 10 * 1.76s = 10 * (128 * 11 subevent interval *1.25)
	 **/
	adv_param = BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_NAME, 0x3700, 0x3700,
				    NULL);
	adv_param->sid = ESL_AP_PA_SID;
	err = bt_le_ext_adv_create(adv_param, &pawr_cbs, &adv_pawr);
	if (err != 0) {
		LOG_ERR("Failed to create pawr advertising extended set (err %d)\n", err);
		return;
	}

	param.interval_min = CONFIG_ESL_PAWR_INTERVAL_MIN;
	param.interval_max = CONFIG_ESL_PAWR_INTERVAL_MAX;
	param.options = BT_LE_PER_ADV_OPT_NONE;
	param.num_subevents = CONFIG_ESL_CLIENT_MAX_GROUP;
	param.subevent_interval = CONFIG_ESL_SUBEVENT_INTERVAL;
	param.response_slot_delay = CONFIG_ESL_RESPONSE_SLOT_DELAY;
	param.response_slot_spacing = CONFIG_ESL_RESPONSE_SLOT_SPACING;
	param.num_response_slots = CONFIG_ESL_NUM_RESPONSE_SLOTS;

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv_pawr, &param);
	if (err != 0) {
		LOG_ERR("Failed to set periodic advertising with response parameters"
			" (err %d)\n",
			err);
	}

	is_pawr_init = true;
}

void pawr_request(struct bt_le_ext_adv *adv, const struct bt_le_per_adv_data_request *request)
{
	LOG_DBG("req evt %d %d", request->start, request->count);
	esl_ap_sync_fill_buf_work.info = *request;
	(void)k_work_submit_to_queue(&pawr_work_q, &esl_ap_sync_fill_buf_work.work);
}

static void pawr_response(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
			  struct net_buf_simple *buf)
{
	for (size_t idx = 0; idx < CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER; idx++) {
		esl_c_obj_l->sync_buf[info->subevent].rsp_buffer[idx].rsp_len = 0;
	}

	memcpy(esl_c_obj_l->sync_buf[info->subevent].rsp_buffer[info->response_slot].data,
	       buf->data, buf->len);
	esl_c_obj_l->sync_buf[info->subevent].rsp_buffer[info->response_slot].rsp_len = buf->len;
	esl_ap_resp_fill_buf_work.info = *info;

	(void)k_work_submit(&esl_ap_resp_fill_buf_work.work);
	LOG_INF("Tx status %d Tx Power %d, rssi %d cte_type %d subevent %d slot %d len %d",
		info->tx_status, info->tx_power, info->rssi, info->cte_type, info->subevent,
		info->response_slot, buf->len);
}

int esl_start_pawr(void)
{
	int err;

	LOG_DBG("%d", k_work_delayable_busy_get(&esl_ap_pawr_work.work));
	esl_ap_pawr_work.onoff = true;
	err = k_work_reschedule_for_queue(&pawr_work_q, &esl_ap_pawr_work.work, K_NO_WAIT);

	return err;
}
int esl_stop_pawr(void)
{
	int err;

	LOG_DBG("%d", k_work_delayable_busy_get(&esl_ap_pawr_work.work));
	esl_ap_pawr_work.onoff = false;
	err = k_work_reschedule_for_queue(&pawr_work_q, &esl_ap_pawr_work.work, K_NO_WAIT);

	return err;
}
void esl_ap_pawr_work_fn(struct k_work *work)
{
	int err;
	uint8_t adv_handle;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_ap_pawr_work_info *pawr_work =
		CONTAINER_OF(dwork, struct esl_ap_pawr_work_info, work);

	if (!pawr_work->onoff) {
		LOG_DBG("PAWR off");
		bt_le_ext_adv_stop(adv_pawr);
		bt_le_per_adv_stop(adv_pawr);
		return;
	}

	LOG_DBG("PAWR off");

	err = bt_hci_get_adv_handle(adv_pawr, &adv_handle);
	if (err) {
		LOG_ERR("pawr adv handle not exists");
	}

	LOG_INF("Start ESL AP Extended Advertising...");
	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv_pawr, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		LOG_ERR("Failed to start extended advertising "
			"(err %d)\n",
			err);
		return;
	}

	LOG_INF("Start ESL AP Periodic Advertising with Responses...");
	/* Start Periodic Advertising with Responses*/
	err = bt_le_per_adv_start(adv_pawr);
	if (err != 0) {
		LOG_ERR("Failed to enable periodic advertising with Responses %d", err);
	}

	bt_le_ext_adv_stop(adv_pawr);
}

/* 0 = PAST success, 1 = no need PAST, non-zero means error */
static int esl_c_past_unsynced_tag(uint8_t conn_idx)
{
	int ret;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	if (!esl_c_obj_l->gatt[conn_idx].esl_device.past_needed) {
		LOG_INF("No need Auto PAST");
		return 1;
	}

	bt_esl_c_write_esl_addr(conn_idx, esl_c_obj_l->gatt[conn_idx].esl_device.esl_addr);
	esl_c_tag_abs_timer_update(conn_idx);
	esl_c_ap_key_update(conn_idx);
	esl_c_rsp_key_update(conn_idx);
	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	ret = bt_esl_c_ecp_command_send(esl_c_obj_l, command, OP_UPDATE_COMPLETE, conn_idx);
	if (ret) {
		LOG_ERR("Send Update complete %02d (ret %d)", conn_idx, ret);
		return ret;
	}

	for (size_t i = 0; i <= AUTO_PAST_RETRY; i++) {
		LOG_INF("AUTO_PAST_RETRY %d\n", i);
		CHECKIF(check_ble_connection(esl_c_obj_l->conn[conn_idx])) {
			if (i == AUTO_PAST_RETRY) {
				LOG_ERR("Auto PAST failed");
				peer_disconnect(esl_c_obj_l->conn[conn_idx]);
				ret = -EAGAIN;
			}

			ret = esl_c_past(conn_idx);
			if (ret) {
				LOG_ERR("Send PAST %02d (ret %d)", conn_idx, ret);
			}
			/* Wait till next subevent interval */
			k_msleep((CONFIG_ESL_PAWR_INTERVAL_MIN * 4 / 3));
		} else {
			break;
		}
	}

	return ret;
}

static void esl_tag_load_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_tag_load_work_info *tag_work =
		CONTAINER_OF(dwork, struct esl_tag_load_work_info, work);

	LOG_INF("load tag begin");
	load_tag_in_storage(tag_work->ble_addr, &esl_c_obj_l->gatt[tag_work->conn_idx].esl_device);
	LOG_INF("load tag end");
}

/* 6.1.1 Configure or reconfigure an ESL procedure */
void esl_ap_config_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esl_ap_config_work_info *config_work =
		CONTAINER_OF(dwork, struct esl_ap_config_work_info, work);
	int err;
	uint8_t data[ESL_ADDR_LEN];
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	LOG_DBG("config_work conn_idx %d, 0x%04x", config_work->conn_idx,
		config_work->param.esl_addr);

	CHECKIF(!check_ble_connection(esl_c_obj_l->conn[config_work->conn_idx])) {
		LOG_ERR("no valid connection ,quit configure TAG");
		printk("#CONFIGURED: 0,0x%02x,0x%04x\n no valid connection", config_work->conn_idx,
		       config_work->param.esl_addr);
		return;
	}

	/* Write ESL Address characteristic */
	sys_put_le16(config_work->param.esl_addr, data);
	esl_c_obj_l->gatt[config_work->conn_idx].esl_device.esl_addr = config_work->param.esl_addr;
	err = bt_esl_c_write_esl_addr(config_work->conn_idx, config_work->param.esl_addr);
	if (err != 0) {
		LOG_ERR("GATT write problem (err:%d)", err);
	}

	/* Write AP Sync Key characteristic */
	esl_c_ap_key_update(config_work->conn_idx);

	/* Write ESL Response Key characteristic */
	esl_c_rsp_key_update(config_work->conn_idx);

	esl_c_tag_abs_timer_update(config_work->conn_idx);
#if defined(ESL_CHRC_MULTI)
	UNSET_FLAG(read_chrc);
	err = bt_esl_c_read_chrc_multi(esl_c_obj_l, config_work->conn_idx);
#else
	err = bt_esl_c_read_chrc(esl_c_obj_l, config_work->conn_idx);
#endif
	esl_c_obj_l->gatt[config_work->conn_idx].esl_device.past_needed = true;
	/* Save tag */
#if defined(CONFIG_BT_ESL_TAG_STORAGE)
	LOG_INF("save tag begin");
	save_tag_in_storage(&esl_c_obj_l->gatt[config_work->conn_idx].esl_device);
	LOG_INF("save tag end");
#endif /* (CONFIG_BT_ESL_TAG_STORAGE) */

	/* Update Complete */
	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	err = bt_esl_c_ecp_command_send(esl_c_obj_l, command, OP_UPDATE_COMPLETE,
					config_work->conn_idx);
	if (err != 0) {
		LOG_ERR("Senc ECP Update Complete (err:%d)", err);
	}

	printk("#CONFIGURED: 1,0x%04x,0x%04x\n", config_work->conn_idx,
	       config_work->param.esl_addr);
}

int bt_c_esl_unassociate(struct bt_esl_client *esl_c, uint8_t conn_idx)
{
	int err;
	uint8_t command[ESL_ECP_COMMAND_MAX_LEN];

	memset(command, 0, ESL_ECP_COMMAND_MAX_LEN);
	esl_c->pending_unassociated_tag = conn_idx;
	err = bt_esl_c_ecp_command_send(esl_c, command, OP_UNASSOCIATE, conn_idx);
	return err;
}

int bt_c_esl_unbond(int conn_idx)
{
	int err;

	if (conn_idx == -1) {
		err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	} else if (conn_idx < CONFIG_BT_ESL_PERIPHERAL_MAX) {
		err = bt_unpair(BT_ID_DEFAULT, &esl_c_obj_l->gatt[conn_idx].esl_device.ble_addr);
	} else {
		LOG_ERR("invalid conn_idx");
		err = -EINVAL;
	}

	return err;
}

int bt_c_esl_post_unassociate(uint8_t conn_idx)
{
	int ret;

	LOG_DBG("pending conn_idx %d", conn_idx);

	ret = bt_unpair(BT_ID_DEFAULT, &esl_c_obj_l->gatt[conn_idx].esl_device.ble_addr);
	esl_c_obj_l->pending_unassociated_tag = -1;
	LOG_DBG("tag before %d", esl_c_obj_l->current_esl_count);

	if (esl_c_obj_l->current_esl_count > 0) {
		esl_c_obj_l->current_esl_count--;
	}

	LOG_DBG("tag after %d", esl_c_obj_l->current_esl_count);
	return ret;
}

static char *sync_status_to_str(enum AP_SYNC_BUFFER_STATUS status)
{
	switch (status) {
	case SYNC_EMPTY:
		return "EMPTY";
	case SYNC_READY_TO_PUSH:
		return "SYNC_READY_TO_PUSH";
	case SYNC_PUSHED:
		return "SYNC_PUSHED";
	case SYNC_RESP_FULL:
		return "SYNC_RESP_FULL";
	default:
		return "UKNONWN";
	}
}

void esl_c_sync_buf_status(int8_t group_id)
{
	uint8_t idx = 0;
	uint8_t idx_end = CONFIG_ESL_CLIENT_MAX_GROUP;

	if (group_id != -1) {
		idx = group_id;
		idx_end = group_id + 1;
	}

	for (; idx < idx_end; idx++) {
		printk("Group %03d status %d %s len %02d\n", idx, esl_c_obj_l->sync_buf[idx].status,
		       sync_status_to_str(esl_c_obj_l->sync_buf[idx].status),
		       esl_c_obj_l->sync_buf[idx].data_len);
	}
}

int esl_c_push_sync_buf(uint8_t *data, uint8_t data_len, uint8_t group_id)
{
	int err = 0;

	if (esl_c_obj_l->sync_buf[group_id].status == SYNC_EMPTY ||
	    esl_c_obj_l->sync_buf[group_id].status == SYNC_PUSHED) {
		esl_c_obj_l->sync_buf[group_id].status = SYNC_READY_TO_PUSH;
		esl_c_obj_l->sync_buf[group_id].data_len =
			esl_compose_ad_data((esl_c_obj_l->sync_buf[group_id].data), data, data_len,
					    esl_c_obj_l->esl_randomizer, esl_c_obj_l->esl_ap_key);
		k_work_submit(&esl_ap_sync_fill_buf_work.work);
	} else {
		err = -EBUSY;
	}

	return err;
}

int esl_c_push_rsp_key(
	struct bt_esl_rsp_buffer rsp_buffer[CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER],
	uint8_t group_id)
{
	for (size_t idx = 0; idx < CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER; idx++) {
		memcpy(esl_c_obj_l->sync_buf[group_id].rsp_buffer[idx].rsp_key.key_v,
		       rsp_buffer[idx].rsp_key.key_v, sizeof(rsp_buffer[idx].rsp_key.key_v));
	}

	return 0;
}
void esl_c_dump_sync_buf(uint8_t group_id)
{
	printk("#GROUP:%03d,0x", group_id);
	print_hex(esl_c_obj_l->sync_buf[group_id].data, esl_c_obj_l->sync_buf[group_id].data_len);
	esl_c_dump_resp_buf(group_id);
}

void esl_c_dump_resp_buf(uint8_t group_id)
{
	uint8_t idx;

	for (idx = 0; idx < CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER; idx++) {
		LOG_HEXDUMP_INF(esl_c_obj_l->sync_buf[group_id].rsp_buffer[idx].rsp_key.key_v,
				EAD_KEY_MATERIAL_LEN, "RSP_KEY");
		if (esl_c_obj_l->sync_buf[group_id].rsp_buffer[idx].rsp_len != 0) {
			printk("#RESPONSESLOT:%02d,0x", idx);
			print_hex(esl_c_obj_l->sync_buf[group_id].rsp_buffer[idx].data,
				  esl_c_obj_l->sync_buf[group_id].rsp_buffer[idx].rsp_len);
			esl_c_obj_l->sync_buf[group_id].status = SYNC_EMPTY;
		}
	}
}

void esl_c_scan(bool onoff, bool oneshot)
{
	int err;

	if (onoff) {
		if (atomic_test_bit(&esl_c_obj_l->acl_state, ESL_C_CONNECTING)) {
			LOG_WRN("Connecting, can't scan");
			return;
		}

		err = bt_scan_start(BT_LE_SCAN_TYPE_PASSIVE);
		if (err != 0) {
			LOG_ERR("Scanning ESL TAG failed to start (err %d)", err);
			return;
		}

		atomic_set_bit(&esl_c_obj_l->acl_state, ESL_C_SCANNING);
		k_work_reschedule(&esl_list_tag_work.work, K_NO_WAIT);

		if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
			dk_set_led(DK_LED1, 0);
			ind_led[0].blink = true;
		}

	} else {
		err = bt_scan_stop();
		if (err != 0) {
			LOG_ERR("Stop ESL TAG scanning failed (err %d)", err);
		}

		atomic_clear_bit(&esl_c_obj_l->acl_state, ESL_C_SCANNING);

		if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
			ind_led[0].blink = false;
		}
	}

	esl_c_obj_l->scan_one_shot = oneshot;
}

int esl_c_scanned_tag_add(const bt_addr_le_t *addr)
{
	struct tag_addr_node *tag_addr = NULL, *cur = NULL, *next = NULL;
	int ret = 0;
	char str_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str_addr, sizeof(str_addr));

	/* Travel ble addr list */
	if (sys_slist_peek_head(&esl_c_obj_l->scanned_tag_addr_list) != NULL) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&esl_c_obj_l->scanned_tag_addr_list, cur, next,
						  node) {
			ret = bt_addr_le_cmp(addr, &cur->addr);
			if (ret == 0) {
				/* LOG_DBG("tag %s exists in list", str_addr); */
				return ret;
			}
		}
	}

	/* Add ble addr node if node does not exist */
	if (tag_addr == NULL) {
		tag_addr = (struct tag_addr_node *)k_malloc(sizeof(struct tag_addr_node));
		if (tag_addr == NULL) {
			return -ENOBUFS;
		}

		memset(tag_addr, 0, sizeof(struct tag_addr_node));
		sys_slist_append(&esl_c_obj_l->scanned_tag_addr_list, &tag_addr->node);
		memcpy(&tag_addr->addr, addr, sizeof(bt_addr_le_t));
		tag_addr->connected = false;
		printk("#TAG_SCANNED: 1,%s to list\n", str_addr);
		esl_c_obj_l->scanned_count++;
		ret = 1;
	}

	return ret;
}

void esl_c_list_scanned(bool dump_connected)
{
	struct tag_addr_node *cur = NULL, *next = NULL;
	size_t idx = 0;
	char str_addr[BT_ADDR_LE_STR_LEN];

	if (sys_slist_peek_head(&esl_c_obj_l->scanned_tag_addr_list) != NULL) {
		printk("#SCANNED_TAG:\n");
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&esl_c_obj_l->scanned_tag_addr_list, cur, next,
						  node) {
			if (cur && (dump_connected || !cur->connected)) {
				bt_addr_le_to_str(&cur->addr, str_addr, sizeof(str_addr));
				printk("[%u]\t %s connected %d\n", idx++, str_addr, cur->connected);
			}
		}
	}
}

void esl_c_list_connection(void)
{
	char str_addr[BT_ADDR_LE_STR_LEN];

	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_PERIPHERAL_MAX; idx++) {
		if (check_ble_connection(esl_c_obj_l->conn[idx])) {
			bt_addr_le_to_str(bt_conn_get_dst(esl_c_obj_l->conn[idx]), str_addr,
					  sizeof(str_addr));
			printk("CONN[0x%02x]:Connected %s\n", idx, str_addr);
		} else {
			printk("CONN[0x%02x]:Free\n", idx);
		}
	}
}

int esl_c_scanned_tag_remove_with_ble_addr(const bt_addr_le_t *peer_addr)
{
	struct tag_addr_node *tag_addr = NULL;
	int ret = 0;

	tag_addr = esl_c_get_tag_addr(-1, peer_addr);

	if (tag_addr == NULL) {
		ret = -ENOENT;
	} else {
		/* Remove node in list */
		tag_addr->connected = false;

		if (!sys_slist_find_and_remove(&esl_c_obj_l->scanned_tag_addr_list,
					       &tag_addr->node)) {
			ret = -EIO;
		}

		k_free(tag_addr);
	}

	return ret;
}

int esl_c_scanned_tag_remove(int tag_idx)
{
	struct tag_addr_node *tag_addr = NULL;
	bool rc = false;

	if (tag_idx == -1) {
		sys_slist_init(&esl_c_obj_l->scanned_tag_addr_list);
		esl_c_obj_l->scanned_count = 0;
		rc = true;
	} else {
		tag_addr = esl_c_get_tag_addr(tag_idx, NULL);
		if (tag_addr != NULL) {
			rc = sys_slist_find_and_remove(&esl_c_obj_l->scanned_tag_addr_list,
						       &tag_addr->node);
			k_free(tag_addr);
			esl_c_obj_l->scanned_count--;
		}
	}

	LOG_DBG("esl_c_scanned_tag_remove (rc %d)", rc);

	return rc;
}

struct tag_addr_node *esl_c_get_tag_addr(int tag_idx, const bt_addr_le_t *peer_addr)
{
	int idx = 0;
	struct tag_addr_node *cur = NULL, *next = NULL, *ret = NULL;

	if (tag_idx > esl_c_obj_l->scanned_count) {
		LOG_ERR("Looking tag_idx is out of scanned range");
		return NULL;
	}

	if (sys_slist_peek_head(&esl_c_obj_l->scanned_tag_addr_list) != NULL) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&esl_c_obj_l->scanned_tag_addr_list, cur, next,
						  node) {
			if (idx == tag_idx) {
				LOG_INF("found by index");
				ret = cur;
				break;
			} else if (bt_addr_le_cmp(peer_addr, &cur->addr) == 0) {
				LOG_INF("found by addr");
				ret = cur;
				break;
			}

			idx++;
		}
	}

	if (ret == NULL) {
		LOG_ERR("tag_addr_node not found");
	}

	return ret;
}

int esl_c_get_free_conn_idx(void)
{
	int ret = -1;

	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_PERIPHERAL_MAX; idx++) {
		if (!check_ble_connection(esl_c_obj_l->conn[idx])) {
			ret = idx;
			break;
		}
	}

	if (ret == -1) {
		LOG_ERR("No free conn\n");
	}

	return ret;
}

int esl_c_connect(const bt_addr_le_t *peer_addr, uint16_t esl_addr)
{
	int ret;
	struct bt_le_conn_param *conn_param = BT_LE_CONN_PARAM(
		(CONFIG_ESL_SUBEVENT_INTERVAL), (CONFIG_ESL_SUBEVENT_INTERVAL), 4, 400);
	struct bt_conn_le_create_param *create_param = BT_CONN_LE_CREATE_PARAM(
		BT_CONN_LE_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_INTERVAL);
	int conn_idx = esl_c_get_free_conn_idx();
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(peer_addr, addr_str, sizeof(addr_str));
	if (conn_idx == -1) {
		LOG_ERR("No free conn");
		return -ENOMEM;
	}

	LOG_INF("%s uses conn_idx 0x%02x 0x%04x %s\n", __func__, conn_idx, esl_addr, addr_str);

	if (atomic_test_bit(&esl_c_obj_l->acl_state, ESL_C_SCANNING)) {
		if (esl_c_obj_l->scan_one_shot == false) {
			atomic_set_bit(&esl_c_obj_l->acl_state, ESL_C_SCAN_PENDING);
		}

		esl_c_scan(false, esl_c_obj_l->scan_one_shot);
	}

	if (LOW_BYTE(esl_addr) == ESL_ADDR_BROADCAST) {
		ret = bt_conn_le_create(peer_addr, create_param, conn_param,
					&esl_c_obj_l->conn[conn_idx]);
		esl_c_obj_l->gatt[conn_idx].esl_device.connect_from = ACL_FROM_SCAN;
		LOG_DBG("bt_conn_le_create (ret %d)\n", ret);
		if (ret != 0) {
			LOG_ERR("Create BT LE connection %d", ret);
#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
			k_sem_give(&sem_auto_config);
#endif
		} else {
			ret = conn_idx;
		}
	} else {
		struct bt_conn_le_create_synced_param params;

		params.subevent = GROUPID_BYTE(esl_addr);
		params.peer = peer_addr;
		ret = bt_conn_le_create_synced(adv_pawr, &params, conn_param,
					       &esl_c_obj_l->conn[conn_idx]);
		esl_c_obj_l->gatt[conn_idx].esl_device.connect_from = ACL_FROM_PAWR;
		LOG_INF("bt_conn_le_create_synced conn_idx %d subevent %d (ret %d)\n", conn_idx,
			params.subevent, ret);
		if (ret != 0) {
			LOG_ERR("Create BT LE connection v2 %d", ret);
		} else {
			ret = conn_idx;
		}
	}

	return ret;
}

uint16_t conn_idx_to_esl_addr(uint8_t conn_idx)
{
	uint16_t ret = 0x8000;

	if (conn_idx < CONFIG_BT_ESL_PERIPHERAL_MAX) {
		ret = esl_c_obj_l->gatt[conn_idx].esl_device.esl_addr;
	}

	return ret;
}

int esl_addr_to_conn_idx(uint16_t esl_addr)
{
	int ret = -1;

	/* Ignore GROUP_ID RFU bit */
	BIT_UNSET(esl_addr, ESL_ADDR_RFU_BIT);

	for (uint16_t idx = 0; idx < CONFIG_BT_ESL_PERIPHERAL_MAX; idx++) {
		if (BIT_UNSET(esl_c_obj_l->gatt[idx].esl_device.esl_addr, ESL_ADDR_RFU_BIT) ==
		    esl_addr) {
			ret = idx;
			break;
		}
	}
	return ret;
}

int ble_addr_to_conn_idx(const bt_addr_le_t *addr)
{
	int ret = -1;

	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_PERIPHERAL_MAX; idx++) {
		if (bt_addr_le_cmp(addr, bt_conn_get_dst(esl_c_obj_l->conn[idx])) == 0) {
			ret = idx;
			LOG_DBG("Found conn_idx 0x%02x through ble_addr", ret);
			break;
		}
	}

	if (ret == -1) {
		ret = esl_c_get_free_conn_idx();
		LOG_DBG("Can't find conn_idx through ble_addr, use free conn_idx %d", ret);
	}

	bt_addr_le_copy(&esl_c_obj_l->gatt[ret].esl_device.ble_addr, addr);
	return ret;
}

void esl_c_ap_key_update(uint8_t conn_idx)
{
	int err;
	struct bt_esl_key_material esl_ap_key;

	esl_c_obj_l->gatt[conn_idx].esl_write_params.func = esl_c_write_attr;
	memcpy(esl_ap_key.key_v, esl_c_obj_l->esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN);
	/* Change endiness of session key before send it out */
	sys_mem_swap(esl_ap_key.key.session_key, BT_EAD_KEY_SIZE);
	/* Write AP Sync Key characteristic */
	esl_c_obj_l->gatt[conn_idx].esl_write_params.data = esl_ap_key.key_v;
	esl_c_obj_l->gatt[conn_idx].esl_write_params.length = EAD_KEY_MATERIAL_LEN;
	esl_c_obj_l->gatt[conn_idx].esl_write_params.handle =
		esl_c_obj_l->gatt[conn_idx].gatt_handles.handles[HANDLE_AP_SYNC_KEY];
	LOG_HEXDUMP_DBG(esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN, "ap_sync_key");
	UNSET_FLAG(write_chrc);

	err = bt_gatt_write(esl_c_obj_l->conn[conn_idx],
			    &esl_c_obj_l->gatt[conn_idx].esl_write_params);
	if (err != 0) {
		LOG_ERR("GATT write problem (err:%d)", err);
	}

	WAIT_FOR_FLAG(write_chrc);
}

void esl_c_rsp_key_update(uint8_t conn_idx)
{
	int err;
	uint8_t rsp_key[EAD_KEY_MATERIAL_LEN] = {0};

	if (memcmp(esl_c_obj_l->gatt[conn_idx].esl_device.esl_rsp_key.key_v, rsp_key,
		   EAD_KEY_MATERIAL_LEN) == 0) {
		err = bt_rand(esl_c_obj_l->gatt[conn_idx].esl_device.esl_rsp_key.key_v,
			      EAD_KEY_MATERIAL_LEN);
		LOG_DBG("bt_rand for esl rsp key (err %d)", err);
	}

	memcpy(rsp_key, esl_c_obj_l->gatt[conn_idx].esl_device.esl_rsp_key.key_v,
	       EAD_KEY_MATERIAL_LEN);
	/* Change endiness of session key before send it out */
	sys_mem_swap(rsp_key, BT_EAD_KEY_SIZE);

	esl_c_obj_l->gatt[conn_idx].esl_write_params.data = rsp_key;
	esl_c_obj_l->gatt[conn_idx].esl_write_params.length = EAD_KEY_MATERIAL_LEN;
	esl_c_obj_l->gatt[conn_idx].esl_write_params.handle =
		esl_c_obj_l->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_RESP_KEY];
	LOG_HEXDUMP_DBG(rsp_key, EAD_KEY_MATERIAL_LEN, "esl_rsp_key");
	UNSET_FLAG(write_chrc);
	err = bt_gatt_write(esl_c_obj_l->conn[conn_idx],
			    &esl_c_obj_l->gatt[conn_idx].esl_write_params);
	if (err != 0) {
		LOG_ERR("GATT write problem (err:%d)", err);
	}

	WAIT_FOR_FLAG(write_chrc);

#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
	/* Auto AP related Parameter */
	uint16_t esl_addr = esl_c_obj_l->gatt[conn_idx].esl_device.esl_addr;
	uint8_t esl_id_offset = LOW_BYTE(esl_addr) - CONFIG_ESL_CLIENT_DEFAULT_ESL_ID;

	if (esl_id_offset <= esl_c_tags_per_group) {
		memcpy(esl_c_obj_l->sync_buf[GROUPID_BYTE(esl_addr)]
			       .rsp_buffer[esl_id_offset]
			       .rsp_key.key_v,
		       esl_c_obj_l->gatt[conn_idx].esl_device.esl_rsp_key.key_v,
		       EAD_KEY_MATERIAL_LEN);
	}

#endif /* CONFIG_BT_ESL_AP_AUTO_MODE */
}

void esl_c_tag_abs_timer_update(uint8_t conn_idx)
{
	int err;
	uint32_t abs_time = esl_c_get_abs_time();

	esl_c_obj_l->gatt[conn_idx].esl_write_params.func = esl_c_write_attr;

	/* Write ABSOLUTE TIME characteristic */
	esl_c_obj_l->gatt[conn_idx].esl_write_params.data = &abs_time;
	esl_c_obj_l->gatt[conn_idx].esl_write_params.length = sizeof(uint32_t);
	esl_c_obj_l->gatt[conn_idx].esl_write_params.handle =
		esl_c_obj_l->gatt[conn_idx].gatt_handles.handles[HANDLE_ESL_ABS_TIME];
	UNSET_FLAG(write_chrc);
	err = bt_gatt_write(esl_c_obj_l->conn[conn_idx],
			    &esl_c_obj_l->gatt[conn_idx].esl_write_params);
	if (err != 0) {
		LOG_ERR("GATT write problem (err:%d)", err);
	}

	WAIT_FOR_FLAG(write_chrc);
}

struct bt_esl_client *esl_c_get_esl_c_obj(void)
{
	return esl_c_obj_l;
}

int bt_esl_c_reset_ap(void)
{
	int err;

	err = esl_c_setting_clear_all();
	esl_c_obj_l->current_esl_count = 0;
	/* Release all flag*/
	SET_FLAG(read_disp_chrc);
	SET_FLAG(read_img_chrc);
	SET_FLAG(read_sensor_chrc);
	SET_FLAG(read_led_chrc);
	SET_FLAG(read_chrc);
	SET_FLAG(write_chrc);
	SET_FLAG(esl_ots_select_flag);
	SET_FLAG(esl_ots_write_flag);

	bt_ots_client_unregister(0);
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_PERIPHERAL_MAX; idx++) {
		memset(&esl_c_obj_l->gatt[idx].esl_device, 0, sizeof(struct bt_esl_chrc_data));
	}

	err = bt_c_esl_unbond(-1);
#if defined(BT_ESL_TAG_STORAGE)
	err = remove_all_tags_in_storage();
#endif /* (CONFIG_BT_ESL_TAG_STORAGE) */
	return err;
}

void esl_c_gatt_flag_dump(void)
{
	printk("read_disp_chrc 0x%02x\n", (unsigned int)GET_FLAG(read_disp_chrc));
	printk("read_img_chrc 0x%02x\n", (unsigned int)GET_FLAG(read_img_chrc));
	printk("read_sensor_chrc 0x%02x\n", (unsigned int)GET_FLAG(read_sensor_chrc));
	printk("read_led_chrc 0x%02x\n", (unsigned int)GET_FLAG(read_led_chrc));
	printk("read_chrc 0x%02x\n", (unsigned int)GET_FLAG(read_chrc));
	printk("write_chrc 0x%02x\n", (unsigned int)GET_FLAG(write_chrc));
	printk("esl_ots_select_flag 0x%02x\n", (unsigned int)GET_FLAG(esl_ots_select_flag));
	printk("esl_ots_write_flag 0x%02x\n", (unsigned int)GET_FLAG(esl_ots_write_flag));
}

void esl_c_dump_chrc(const struct bt_esl_chrc_data *chrc)
{
	printk("#CHR_READ:1\n");
	printk("ESL_ADDR:0x%04x\n", chrc->esl_addr);
	printk("RSP_KEY:0x");
	print_hex((uint8_t *)chrc->esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN);
	printk("Max_image_index:%d\n", chrc->max_image_index);
	printk("DISPLAY:%d%s", chrc->display_count, (chrc->display_count > 0) ? ",0x" : "");
	print_hex((uint8_t *)chrc->displays, (chrc->display_count * sizeof(struct esl_disp_inf)));
	printk("SENSOR:%d%s", chrc->sensor_count, (chrc->sensor_count > 0) ? ",0x" : "");
	print_hex((uint8_t *)(uint8_t *)chrc->sensors,
		  (chrc->sensor_count * sizeof(struct esl_sensor_inf)));
	dump_sensor_inf(chrc->sensors, chrc->sensor_count);
	printk("LED:%d%s", chrc->led_count, (chrc->led_count > 0) ? ",0x" : "");
	print_hex((uint8_t *)chrc->led_type, chrc->led_count);
	dump_led_infs((uint8_t *)chrc->led_type, chrc->led_count);
	printk("PNPID:VID:0x%04x, PID:0x%04x\n", chrc->dis_pnp_id.pnp_vid,
	       chrc->dis_pnp_id.pnp_pid);
}

void esl_c_gatt_flag_set(uint8_t flag_idx, uint8_t set_unset)
{
	switch (flag_idx) {
	case 0:
		(set_unset == 1) ? SET_FLAG(read_disp_chrc) : UNSET_FLAG(read_disp_chrc);
		break;
	case 1:
		(set_unset == 1) ? SET_FLAG(read_img_chrc) : UNSET_FLAG(read_img_chrc);
		break;
	case 2:
		(set_unset == 1) ? SET_FLAG(read_sensor_chrc) : UNSET_FLAG(read_sensor_chrc);
		break;
	case 3:
		(set_unset == 1) ? SET_FLAG(read_led_chrc) : UNSET_FLAG(read_led_chrc);
		break;
	case 4:
		(set_unset == 1) ? SET_FLAG(read_chrc) : UNSET_FLAG(read_chrc);
		break;
	case 5:
		(set_unset == 1) ? SET_FLAG(write_chrc) : UNSET_FLAG(write_chrc);
		break;
	case 6:
		(set_unset == 1) ? SET_FLAG(esl_ots_select_flag) : UNSET_FLAG(esl_ots_select_flag);
		break;
	case 7:
		(set_unset == 1) ? SET_FLAG(esl_ots_write_flag) : UNSET_FLAG(esl_ots_write_flag);
		break;
	default:
		break;
	}
}

int esl_c_past(uint8_t conn_idx)
{
	int err;

	err = bt_le_per_adv_set_info_transfer(adv_pawr, esl_c_obj_l->conn[conn_idx],
					      BT_UUID_ESL_VAL);

	if (err != 0) {
		printk("#PAST:0,%02d,%d\n", conn_idx, err);
	} else {
		printk("#PAST:1,%02d\n", conn_idx);
	}

	return err;
}

int bt_esl_client_init(struct bt_esl_client *esl_c,
		       const struct bt_esl_client_init_param *esl_c_init)
{
	int err;
	uint8_t empty_key[EAD_KEY_MATERIAL_LEN] = {0};
	struct k_work_queue_config pawr_q_config = {
		.name = "pawr_workq",
	};

	struct k_work_queue_config ap_config_q_config = {
		.name = "ap_config_workq",
	};

	if (!esl_c || !esl_c_init) {
		return -EINVAL;
	}

	sys_slist_init(&esl_c->scanned_tag_addr_list);
	esl_c->scan_one_shot = false;
	esl_c->current_esl_count = 0;
	esl_c->pending_unassociated_tag = -1;
	esl_c->acl_state = ATOMIC_INIT(0);
	esl_ap_security_level =
		IS_ENABLED(CONFIG_BT_ESL_SECURITY_ENABLED) ? BT_SECURITY_L2 : BT_SECURITY_L1;
	esl_c_obj_l = esl_c;
	esl_c_obj_l->esl_addr_start = (CONFIG_ESL_CLIENT_DEFAULT_GROUP_ID & 0xff) << 8 |
				      (CONFIG_ESL_CLIENT_DEFAULT_ESL_ID & 0xff);
	esl_c_settings_init();

	esl_c_sync_buffer_dummy_cmd_init();
	bt_conn_cb_register(&conn_callbacks);
	bt_gatt_cb_register(&gatt_callbacks);

	esl_c_scan_init();
	if (memcmp(esl_c->esl_ap_key.key_v, empty_key, EAD_KEY_MATERIAL_LEN) == 0) {
		LOG_DBG("ap_key is not set");
		err = bt_rand(esl_c->esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN);
		LOG_INF("bt_rand ap_key (err %d)", err);
		esl_c_setting_ap_key_save();
	}

	LOG_HEXDUMP_DBG(esl_c->esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN,
			"bt_esl_client_init esl_ap_key");

	abs_time_anchor = k_uptime_get_32();
	esl_c_write_wo_rsp = false;
	esl_init_randomizer(esl_c->esl_randomizer);
	bt_otc_init();
	/* PAwR workqueue init */
	k_work_queue_start(&pawr_work_q, pawr_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(pawr_wq_stack_area), PAWR_WQ_PRIORITY,
			   &pawr_q_config);
	/* AP config workqueue init */
	k_work_queue_start(&ap_config_work_q, ap_config_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(ap_config_wq_stack_area), AP_CONFIG_PRIORITY,
			   &ap_config_q_config);
	k_work_init(&esl_ap_sync_fill_buf_work.work, esl_ap_sync_fill_buf_work_fn);
	k_work_init_delayable(&esl_ap_config_work.work, esl_ap_config_work_fn);
	k_work_init_delayable(&esl_list_tag_work.work, esl_list_tag_work_fn);
	k_work_init_delayable(&esl_ap_pawr_work.work, esl_ap_pawr_work_fn);
	k_work_init(&esl_ap_resp_fill_buf_work.work, esl_ap_resp_fill_buf_work_fn);
#if defined(CONFIG_BT_ESL_TAG_STORAGE)
	k_work_init_delayable(&esl_tag_load_work.work, esl_tag_load_work_fn);
#endif
#if defined(CONFIG_BT_ESL_AP_AUTO_MODE)
	k_work_init_delayable(&esl_ots_write_img_work.work, esl_ots_write_img_work_fn);
	k_thread_create(&esl_auto_ap_connect_thread, esl_auto_ap_connect_thread_stack,
			K_THREAD_STACK_SIZEOF(esl_auto_ap_connect_thread_stack),
			esl_auto_ap_connect_work_fn, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO + 2, K_USER, K_NO_WAIT);
#endif
	pawr_cbs.pawr_data_request = pawr_request;
	pawr_cbs.pawr_response = pawr_response;
	setup_pawr_adv();
	esl_start_pawr();

	discovery_work_init();

	esl_c_obj_l->cb = esl_c_init->cb;
	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION)) {
		(void)dk_leds_init();
		k_timer_start(&led_blink_timer, K_MSEC(BLINK_FREQ_MS / 2),
			      K_MSEC(BLINK_FREQ_MS / 2));
		dk_set_led(DK_LED1, 1);
	}

	return 0;
}
