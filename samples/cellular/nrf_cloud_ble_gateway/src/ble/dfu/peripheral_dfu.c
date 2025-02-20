/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <zephyr/logging/log.h>
#include <net/fota_download.h>
#include <net/download_client.h>
#include <zephyr/sys/crc.h>
#include <cJSON.h>

#include "nrf_cloud_fota.h"
#include "ble.h"
#include "gateway.h"
#include "ble_conn_mgr.h"
#include "peripheral_dfu.h"

LOG_MODULE_REGISTER(peripheral_dfu, CONFIG_NRF_CLOUD_FOTA_LOG_LEVEL);

#define DFU_CONTROL_POINT_UUID     "8EC90001F3154F609FB8838830DAEA50"
#define DFU_PACKET_UUID            "8EC90002F3154F609FB8838830DAEA50"
#define DFU_BUTTONLESS_UUID        "8EC90003F3154F609FB8838830DAEA50"
#define DFU_BUTTONLESS_BONDED_UUID "8EC90004F3154F609FB8838830DAEA50"
#define MAX_CHUNK_SIZE 20
#define PROGRESS_UPDATE_INTERVAL 5
#define MAX_FOTA_FILES 2

#define CALL_TO_PRINTK(fmt, ...) do {		 \
		printk(fmt "\n", ##__VA_ARGS__); \
	} while (false)

#define LOGPKINF(...)	do {					     \
				if (use_printk) {		     \
					CALL_TO_PRINTK(__VA_ARGS__); \
				} else {			     \
					LOG_INF(__VA_ARGS__);        \
				}				     \
		} while (false)

enum nrf_dfu_op_t {
	NRF_DFU_OP_PROTOCOL_VERSION = 0x00,
	NRF_DFU_OP_OBJECT_CREATE = 0x01,
	NRF_DFU_OP_RECEIPT_NOTIF_SET = 0x02,
	NRF_DFU_OP_CRC_GET = 0x03,
	NRF_DFU_OP_OBJECT_EXECUTE = 0x04,
	NRF_DFU_OP_OBJECT_SELECT = 0x06,
	NRF_DFU_OP_MTU_GET = 0x07,
	NRF_DFU_OP_OBJECT_WRITE = 0x08,
	NRF_DFU_OP_PING = 0x09,
	NRF_DFU_OP_HARDWARE_VERSION = 0x0A,
	NRF_DFU_OP_FIRMWARE_VERSION = 0x0B,
	NRF_DFU_OP_ABORT = 0x0C,
	NRF_DFU_OP_RESPONSE = 0x60,
	NRF_DFU_OP_INVALID = 0xFF
};

enum nrf_dfu_result_t {
	NRF_DFU_RES_CODE_INVALID = 0x00,
	NRF_DFU_RES_CODE_SUCCESS = 0x01,
	NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED = 0x02,
	NRF_DFU_RES_CODE_INVALID_PARAMETER = 0x03,
	NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES = 0x04,
	NRF_DFU_RES_CODE_INVALID_OBJECT = 0x05,
	NRF_DFU_RES_CODE_UNSUPPORTED_TYPE = 0x07,
	NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED = 0x08,
	NRF_DFU_RES_CODE_OPERATION_FAILED = 0x0A,
	NRF_DFU_RES_CODE_EXT_ERROR = 0x0B
};

typedef enum {
	NRF_DFU_EXT_ERROR_NO_ERROR = 0x00,
	NRF_DFU_EXT_ERROR_INVALID_ERROR_CODE = 0x01,
	NRF_DFU_EXT_ERROR_WRONG_COMMAND_FORMAT = 0x02,
	NRF_DFU_EXT_ERROR_UNKNOWN_COMMAND = 0x03,
	NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID = 0x04,
	NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE = 0x05,
	NRF_DFU_EXT_ERROR_HW_VERSION_FAILURE = 0x06,
	NRF_DFU_EXT_ERROR_SD_VERSION_FAILURE = 0x07,
	NRF_DFU_EXT_ERROR_SIGNATURE_MISSING = 0x08,
	NRF_DFU_EXT_ERROR_WRONG_HASH_TYPE = 0x09,
	NRF_DFU_EXT_ERROR_HASH_FAILED = 0x0A,
	NRF_DFU_EXT_ERROR_WRONG_SIGNATURE_TYPE = 0x0B,
	NRF_DFU_EXT_ERROR_VERIFICATION_FAILED = 0x0C,
	NRF_DFU_EXT_ERROR_INSUFFICIENT_SPACE = 0x0D,
} nrf_dfu_ext_error_code_t;

enum  nrf_dfu_firmware_type_t {
	NRF_DFU_FIRMWARE_TYPE_SOFTDEVICE = 0x00,
	NRF_DFU_FIRMWARE_TYPE_APPLICATION = 0x01,
	NRF_DFU_FIRMWARE_TYPE_BOOTLOADER = 0x02,
	NRF_DFU_FIRMWARE_TYPE_UNKNOWN = 0xFF
};

struct dfu_notify_packet {
	uint8_t magic;
	uint8_t op;
	uint8_t result;
	union {
		struct select {
			uint32_t max_size;
			uint32_t offset;
			uint32_t crc;
		} select;
		struct crc {
			uint32_t offset;
			uint32_t crc;
		} crc;
		struct hw_version {
			uint32_t part;
			uint32_t variant;
			uint32_t rom_size;
			uint32_t ram_size;
			uint32_t rom_page_size;
		} hw;
		struct fw_version {
			uint8_t type;
			uint32_t version;
			uint32_t addr;
			uint32_t len;
		} fw;
		uint8_t ext_err_code;
	};
} __packed;

enum ble_dfu_buttonless_op_code_t {
	DFU_OP_RESERVED = 0x00,
	DFU_OP_ENTER_BOOTLOADER = 0x01,
	DFU_OP_SET_ADV_NAME = 0x02,
	DFU_OP_RESPONSE_CODE = 0x20
};

enum ble_dfu_buttonless_rsp_code_t {
	DFU_RSP_INVALID = 0x00,
	DFU_RSP_SUCCESS = 0x01,
	DFU_RSP_OP_CODE_NOT_SUPPORTED = 0x02,
	DFU_RSP_OPERATION_FAILED = 0x04,
	DFU_RSP_ADV_NAME_INVALID = 0x05,
	DFU_RSP_BUSY = 0x06,
	DFU_RSP_NOT_BONDED = 0x07
};

struct secure_dfu_ind_packet {
	uint8_t resp_code;
	uint8_t op_code;
	uint8_t rsp_code;
} __packed;

K_SEM_DEFINE(peripheral_dfu_active, 1, 1);
static size_t download_size;
static size_t completed_size;
static bool finish_page;
static bool test_mode;
static bool verbose;
static uint32_t max_size;
static uint32_t flash_page_size;
static uint32_t page_remaining;

/* responses from peripheral device */
static bool notify_received;
static bool normal_mode;
static uint16_t notify_length;
static uint8_t dfu_notify_data[MAX_CHUNK_SIZE];

/* normal mode BLE device address */
static char ble_norm_addr[BT_ADDR_STR_LEN];
/* normal mode secure DFU responses */
struct secure_dfu_ind_packet *secure_dfu_ind_packet_data =
	(struct secure_dfu_ind_packet *) dfu_notify_data;

/* DFU mode BLE device address */
static char ble_dfu_addr[BT_ADDR_STR_LEN];
struct ble_device_conn *dfu_conn_ptr;
/* DFU mode protocol responses */
struct dfu_notify_packet *dfu_notify_packet_data =
	(struct dfu_notify_packet *)dfu_notify_data;

static struct nrf_cloud_fota_ble_job fota_ble_job;
static struct nrf_cloud_fota_job_info fota_files[MAX_FOTA_FILES];
static int total_completed_size;
static int active_num;
static int num_fota_files;
static struct k_work_delayable fota_job_work;

static char app_version[16];
static int image_size;
static uint16_t original_crc;
static int socket_retries_left;
static bool first_fragment;
static bool init_packet;
static bool use_printk;
static struct download_client dlc;

static int send_select_command(char *ble_addr);
static int send_create_command(char *ble_addr, uint32_t size);
static int send_switch_to_dfu(char *ble_addr);
static int send_select_data(char *ble_addr);
static int send_create_data(char *ble_addr, uint32_t size);
static int send_prn(char *ble_addr, uint16_t receipt_rate);
static int send_data(char *ble_addr, const char *buf, size_t len);
static int send_request_crc(char *ble_addr);
static int send_execute(char *ble_addr);
static int send_abort(char *ble_addr);
static void on_sent(struct bt_conn *conn, uint8_t err,
		    struct bt_gatt_write_params *params);
static int send_hw_version_get(char *ble_addr);
static int send_fw_version_get(char *ble_addr, uint8_t fw_type);
static uint8_t peripheral_dfu(const char *buf, size_t len);
static int download_client_callback(const struct download_client_evt *event);
static void cancel_dfu(enum nrf_cloud_fota_error error);
static void fota_ble_callback(const struct nrf_cloud_fota_ble_job *
			      const ble_job);
static void free_job(void);
static bool start_next_job(void);
static void fota_job_next(struct k_work *work);

void peripheral_dfu_set_test_mode(bool test)
{
	test_mode = test;
}

static int notification_callback(const char *addr, const char *chrc_uuid,
				 uint8_t *data, uint16_t len)
{
	if ((strcmp(addr, ble_norm_addr) != 0) &&
	    (strcmp(addr, ble_dfu_addr) != 0)) {
		LOG_WRN("Notification received for %s (not us)", addr);
		return 0; /* not for us */
	}
	/** @TODO: implement bonded mode also */
	if (strcmp(chrc_uuid, DFU_BUTTONLESS_UUID) == 0) {
		normal_mode = true;
	} else if (strcmp(chrc_uuid, DFU_CONTROL_POINT_UUID) == 0) {
		normal_mode = false;
	} else {
		LOG_WRN("Notification received for wrong UUID:%s", chrc_uuid);
		return 0; /* not for us */
	}
	notify_length = MIN(len, sizeof(dfu_notify_data));
	memcpy(dfu_notify_data, data, notify_length);
	notify_received = true;
	LOG_DBG("%sdfu notified %u bytes", normal_mode ? "secure " : "", notify_length);
	LOG_HEXDUMP_DBG(data, notify_length, "notify packet");
	return 1;
}

static int wait_for_notification(void)
{
	int max_loops = 1000;

	while (!notify_received) {
		k_sleep(K_MSEC(10));
		if (!max_loops--) {
			return -ETIMEDOUT;
		}
	}
	return 0;
}

static int decode_secure_dfu(void)
{
	struct secure_dfu_ind_packet *p = secure_dfu_ind_packet_data;
	int err;

	if (notify_length < 3) {
		LOG_ERR("Indication too short: %u < 3", notify_length);
		return -EINVAL;
	}
	if (p->resp_code != DFU_OP_RESPONSE_CODE) {
		LOG_ERR("First byte not 0x%02X: 0x%02X", DFU_OP_RESPONSE_CODE, p->resp_code);
		return -EINVAL;
	}
	switch (p->op_code) {
	case DFU_OP_ENTER_BOOTLOADER:
		if (p->rsp_code == DFU_RSP_SUCCESS) {
			LOG_INF("Device switching to DFU mode!");
			err = 0;
		} else {
			LOG_ERR("Error %d switching to DFU mode", p->rsp_code);
			err = -EIO;
		}
		break;
	case DFU_OP_SET_ADV_NAME:
		if (p->rsp_code == DFU_RSP_SUCCESS) {
			LOG_INF("DFU adv name changed successfully");
			err = 0;
		} else {
			LOG_ERR("Error %d changing DFU adv name", p->rsp_code);
			err = -EIO;
		}
		break;
	default:
		LOG_ERR("Unexpected op indicated: 0x%02X", p->op_code);
		err = -EINVAL;
	}
	return err;
}

static int decode_dfu(void)
{
	struct dfu_notify_packet *p = dfu_notify_packet_data;
	char buf[256];

	if (notify_length < 3) {
		LOG_ERR("Notification too short: %u < 3", notify_length);
		return -EINVAL;
	}
	if (p->magic != 0x60) {
		LOG_ERR("First byte not 0x60: 0x%02X", p->magic);
		return -EINVAL;
	}
	switch (p->op) {
	case NRF_DFU_OP_OBJECT_CREATE:
		snprintf(buf, sizeof(buf), "DFU Create response: 0x%02X",
			 p->result);
		break;
	case NRF_DFU_OP_RECEIPT_NOTIF_SET:
		snprintf(buf, sizeof(buf),
			 "DFU Set Receipt Notification response: 0x%02X",
			 p->result);
		break;
	case NRF_DFU_OP_CRC_GET:
		if (notify_length < 7) {
			LOG_ERR("Notification too short: %u < 7",
				notify_length);
			return -EINVAL;
		}
		snprintf(buf, sizeof(buf), "DFU CRC response: 0x%02X, "
					   "offset: 0x%08X, crc: 0x%08X",
		       p->result, p->crc.offset, p->crc.crc);
		break;
	case NRF_DFU_OP_OBJECT_EXECUTE:
		snprintf(buf, sizeof(buf), "DFU Execute response: 0x%02X",
			 p->result);
		break;
	case NRF_DFU_OP_OBJECT_SELECT:
		snprintf(buf, sizeof(buf),
			 "DFU Select response: 0x%02X, offset: 0x%08X,"
			 " crc: 0x%08X, max_size: %u", p->result,
			 p->select.offset, p->select.crc, p->select.max_size);
		max_size = p->select.max_size;
		break;
	case NRF_DFU_OP_HARDWARE_VERSION:
		snprintf(buf, sizeof(buf),
			"DFU HW Version Get response: 0x%02X, part: 0x%08X,"
			" variant: 0x%08X, rom_size: 0x%08X, ram_size: 0x%08X,"
			" rom_page_size: 0x%08X", p->result, p->hw.part,
			p->hw.variant, p->hw.rom_size, p->hw.ram_size,
			p->hw.rom_page_size);
		flash_page_size = p->hw.rom_page_size;
		break;
	case NRF_DFU_OP_FIRMWARE_VERSION:
		snprintf(buf, sizeof(buf),
			 "DFU FW Version Get response: 0x%02X, type: 0x%02X,"
			 " version: 0x%02X, addr: 0x%02X, len: 0x%02X",
			 p->result, p->fw.type, p->fw.version, p->fw.addr,
			 p->fw.len);
		break;
	default:
		LOG_ERR("Unknown DFU notification: 0x%02X", p->op);
		return -EINVAL;
	}
	if (p->result == NRF_DFU_RES_CODE_SUCCESS) {
		if (verbose) {
			LOG_INF("%s", buf);
		} else {
			LOG_DBG("%s", buf);
		}
		return 0;
	} else if (p->result == NRF_DFU_RES_CODE_EXT_ERROR) {
		if (p->ext_err_code == NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE) {
			LOG_ERR("Cannot downgrade target firmware version!");
		} else if (p->ext_err_code == NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE) {
			LOG_ERR("Incompatible target hardware version!");
		} else if (p->ext_err_code == NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE) {
			LOG_ERR("Incompatible target SoftDevice version!");
		} else {
			LOG_ERR("Extended DFU error: 0x%02X", p->ext_err_code);
		}
		return -EFAULT;
	}
	LOG_WRN("DFU operation failed: %d", p->result);
	return -EPROTO;
}

int peripheral_dfu_init(void)
{
	int err;

	k_work_init_delayable(&fota_job_work, fota_job_next);

	err = download_client_init(&dlc, download_client_callback);
	if (err) {
		return err;
	}
	return nrf_cloud_fota_ble_set_handler(fota_ble_callback);
}

int peripheral_dfu_cleanup(void)
{
	ble_subscribe(ble_norm_addr, DFU_BUTTONLESS_UUID, 0);
	ble_conn_mgr_remove_conn(ble_dfu_addr);
	ble_conn_mgr_rem_desired(ble_dfu_addr, true);
	k_sem_give(&peripheral_dfu_active);
	return 0;
}

int peripheral_dfu_config(const char *addr, int size, const char *version,
			   uint32_t crc, bool init_pkt, bool use_prtk)
{
	struct ble_device_conn *conn;
	uint16_t handle;
	int err;

	err = ble_conn_mgr_get_conn_by_addr(addr, &conn);
	if (err) {
		LOG_ERR("Connection not found for addr %s", addr);
		return err;
	}

	err = k_sem_take(&peripheral_dfu_active, K_NO_WAIT);
	if (err) {
		LOG_ERR("Peripheral DFU already active.");
		conn->dfu_attempts = MAX_DFU_ATTEMPTS;
		conn->dfu_pending = true;
		return -EAGAIN;
	}

	dfu_conn_ptr = NULL;

	strncpy(ble_norm_addr, addr, sizeof(ble_norm_addr));

	ble_register_notify_callback(notification_callback);

	err = ble_conn_mgr_get_handle_by_uuid(&handle, DFU_BUTTONLESS_UUID, conn);
	if (err) {
		err = ble_conn_mgr_get_handle_by_uuid(&handle, DFU_CONTROL_POINT_UUID, conn);
		if (err) {
			LOG_ERR("Device does not support buttonless DFU, and is not in DFU mode");
			k_sem_give(&peripheral_dfu_active);
			return -EINVAL;
		}
		LOG_INF("Device already in DFU mode");
		strncpy(ble_dfu_addr, addr, sizeof(ble_dfu_addr));
		goto ready;
	}

	/* need to switch device to DFU mode first */
	ble_conn_mgr_calc_other_addr(addr, ble_dfu_addr, DEVICE_ADDR_LEN, false);

	err = ble_conn_mgr_get_conn_by_addr(ble_dfu_addr, &conn);
	if (err) {
		LOG_INF("Adding temporary connection to %s for DFU",
			ble_dfu_addr);
		err = ble_conn_mgr_add_conn(ble_dfu_addr);
		if (err) {
			goto failed;
		}
		LOG_INF("Connection added to ble_conn_mgr");
	} else {
		LOG_INF("Connection already exists in ble_conn_mgr");
	}
	err = ble_conn_mgr_add_desired(ble_dfu_addr, true);
	if (err) {
		goto failed;
	}

	LOG_INF("Enabling indication");
	err = ble_subscribe(ble_norm_addr, DFU_BUTTONLESS_UUID,
			    BT_GATT_CCC_INDICATE);
	if (err) {
		goto failed;
	}

	LOG_INF("Switching to DFU mode");
	err = send_switch_to_dfu(ble_norm_addr);
	if (err) {
		goto failed;
	}
	int max_loops = 300;

	err = ble_conn_mgr_get_conn_by_addr(ble_dfu_addr, &conn);
	if (err) {
		goto failed;
	}

	/* special dfu flag -- don't send info to cloud about this dfu device */
	conn->hidden = true;

	LOG_INF("Waiting for device discovery...");
	while (!conn->discovered && !conn->encode_discovered) {
		k_sleep(K_MSEC(100));
		if (!max_loops--) {
			LOG_ERR("Timeout: conn:%u, ding:%u, disc:%u, enc:%u, np:%u",
				conn->connected, conn->discovering,
				conn->discovered, conn->encode_discovered,
				conn->num_pairs);
			err = -ETIMEDOUT;
			goto failed;
		}
	}
	LOG_INF("Continuing BLE DFU");

ready:
	strncpy(app_version, version, sizeof(app_version));
	image_size = size;
	original_crc = crc;
	init_packet = init_pkt;
	use_printk = use_prtk;
	dfu_conn_ptr = conn;
	return 0;

failed:
	LOG_ERR("Error configuring update:%d", err);
	return err;
}

int peripheral_dfu_start(const char *host, const char *file, int sec_tag,
			 const char *apn, size_t fragment_size)
{
	int err = -1;
	const int sec_tag_list[1] = {
		sec_tag
	};
	struct download_client_cfg config = {
		.sec_tag_list = sec_tag_list,
		.sec_tag_count = ARRAY_SIZE(sec_tag_list),
		.pdn_id = 0,
		.frag_size_override = fragment_size,
		.set_tls_hostname = (sec_tag != -1),
	};

	if (host == NULL || file == NULL) {
		peripheral_dfu_cleanup();
		return -EINVAL;
	}

	socket_retries_left = CONFIG_FOTA_SOCKET_RETRIES;

	first_fragment = true;

	err = download_client_set_host(&dlc, host, &config);
	if (err != 0) {
		peripheral_dfu_cleanup();
		return err;
	}

	err = download_client_start(&dlc, file, 0);
	if (err != 0) {
		download_client_disconnect(&dlc);
		peripheral_dfu_cleanup();
		return err;
	}

	return 0;
}

static int download_client_callback(const struct download_client_evt *event)
{
	int err = 0;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT:
		if (first_fragment) {
			err = download_client_file_size_get(&dlc, &image_size);
			if (err) {
				LOG_ERR("Error determining file size: %d", err);
			} else {
				LOG_INF("Downloading %zd bytes", image_size);
			}
		}
		err = peripheral_dfu(event->fragment.buf,
				     event->fragment.len);
		if (err) {
			LOG_ERR("Error from peripheral_dfu: %d", err);
		}
		break;
	case DOWNLOAD_CLIENT_EVT_DONE:
		LOG_INF("Download client done");
		err = download_client_disconnect(&dlc);
		if (err) {
			LOG_ERR("Error disconnecting from download client: %d",
				err);
		}
		k_work_reschedule(&fota_job_work, K_SECONDS(1));
		break;
	case DOWNLOAD_CLIENT_EVT_ERROR: {
		/* In case of socket errors we can return 0 to retry/continue,
		 * or non-zero to stop
		 */
		if ((socket_retries_left) && ((event->error == -ENOTCONN) ||
					      (event->error == -ECONNRESET))) {
			LOG_WRN("Download socket error. %d retries left...",
				socket_retries_left);
			socket_retries_left--;
			/* Fall through and return 0 below to tell
			 * download_client to retry
			 */
		} else {
			err = download_client_disconnect(&dlc);
			if (err) {
				LOG_ERR("Error disconnecting from "
					"download client: %d", err);
			}
			LOG_ERR("Download client error");
			cancel_dfu(NRF_CLOUD_FOTA_ERROR_DOWNLOAD);
			err = -EIO;
		}
		break;
	}
	default:
		break;
	}

	return err;
}

static int start_ble_job(struct nrf_cloud_fota_ble_job *const ble_job)
{
	__ASSERT_NO_MSG(ble_job != NULL);
	int ret;
	enum nrf_cloud_fota_status status;

	ret = peripheral_dfu_start(ble_job->info.host, ble_job->info.path,
				  CONFIG_NRF_CLOUD_SEC_TAG, NULL,
				  CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE);
	if (ret) {
		LOG_ERR("Failed to start FOTA download: %d", ret);
		status = NRF_CLOUD_FOTA_FAILED;
	} else {
		LOG_INF("Downloading update");
		status = NRF_CLOUD_FOTA_IN_PROGRESS;
		if (total_completed_size) {
			/* not the first file, so no progress needed here */
			return ret;
		}
	}
	(void)nrf_cloud_fota_ble_job_update(ble_job, status);

	return ret;
}

static void fota_ble_callback(const struct nrf_cloud_fota_ble_job *
			      const ble_job)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bool init_pkt;
	char *ver = "n/a";
	uint32_t crc = 0;
	int sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;
	char *apn = NULL;
	size_t frag = CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE;
	int err;

	bt_addr_to_str(&ble_job->ble_id, addr, sizeof(addr));

	if (!ble_conn_mgr_is_addr_connected(addr)) {
		/* TODO: add ability to queue? */
		LOG_WRN("Device not connected; ignoring job");
		return;
	}

	init_pkt = strstr(ble_job->info.path, "dat") != NULL;

	err = peripheral_dfu_config(addr, ble_job->info.file_size, ver, crc,
				    init_pkt, false);
	if (err == -EAGAIN) {
		/* already busy; ask for the job when done with current */
		return;
	} else if (err) {
		/* could not configure, so don't start job */
		cancel_dfu(NRF_CLOUD_FOTA_ERROR_APPLY_FAIL);
		return;
	}

	memset(fota_files, 0, sizeof(fota_files));
	num_fota_files = 0;

	int i;
	char *end;
	char *path = ble_job->info.path;
	bool done = false;

	/* separate out various file paths to download */
	for (i = 0; i < MAX_FOTA_FILES; i++) {
		end = strchr(path, ' ');
		if (!end) {
			end = &path[strlen(path)];
			done = true;
		}
		fota_files[i].path = k_calloc(1 + end - path, 1);
		if (!fota_files[i].path) {
			LOG_ERR("Out of memory");
			return;
		}
		memcpy(fota_files[i].path, path, end - path);
		path = end + 1;
		num_fota_files++;
		if (done) {
			break;
		}
	}

	/* fix up order of files based on type */
	for (i = 0; i < MAX_FOTA_FILES; i++) {
		if (strstr(fota_files[i].path, ".dat")) {
			if (i) { /* .dat is not first; make it so */
				char *tmp = fota_files[0].path;

				fota_files[0].path = fota_files[i].path;
				fota_files[i].path = tmp;
			}
			break;
		}
	}

	active_num = 0;
	total_completed_size = 0;
	fota_ble_job.info.path = fota_files[active_num].path;

	/* job structure will disappear after this callback, so make a copy */
	fota_ble_job.ble_id = ble_job->ble_id;
	fota_ble_job.info.type = ble_job->info.type;
	fota_ble_job.info.id = strdup(ble_job->info.id);
	fota_ble_job.info.host = strdup(ble_job->info.host);
	/* total size of all files */
	fota_ble_job.info.file_size = ble_job->info.file_size;
	fota_ble_job.error = NRF_CLOUD_FOTA_ERROR_NONE;

	LOG_INF("Starting BLE DFU to addr:%s, from host:%s, size:%d, "
		"init:%d, ver:%s, crc:%u, sec_tag:%d, apn:%s, frag_size:%zd",
		addr, fota_ble_job.info.host,
		fota_ble_job.info.file_size,
		init_packet, ver, crc, sec_tag,
		apn ? apn : "<n/a>", frag);
	LOG_INF("Num files:%d", num_fota_files);
	for (i = 0; i < MAX_FOTA_FILES; i++) {
		if (!fota_files[i].path) {
			break;
		}
		LOG_INF("File:%d path:%s", i + 1, fota_files[i].path);
	}

	(void)start_ble_job(&fota_ble_job);
}

static void fota_job_next(struct k_work *work)
{
	if (!start_next_job()) {
		ble_conn_mgr_force_dfu_rediscover(ble_norm_addr);
		if (strcmp(ble_norm_addr, ble_dfu_addr) != 0) {
			ble_conn_mgr_force_dfu_rediscover(ble_dfu_addr);
		}
		ble_register_notify_callback(NULL);
		LOGPKINF("DFU complete");
		free_job();
		peripheral_dfu_cleanup();
		ble_conn_mgr_check_pending();
	}
}

static bool start_next_job(void)
{
	/* select next job */
	active_num++;
	if (active_num >= num_fota_files) {
		LOG_INF("No more files.  Done.");
		return false;
	}
	fota_ble_job.info.path = fota_files[active_num].path;
	fota_ble_job.error = NRF_CLOUD_FOTA_ERROR_NONE;

	if (!ble_conn_mgr_is_addr_connected(ble_dfu_addr)) {
		/* TODO: add ability to queue? */
		LOG_ERR("Device not connected; cancelling remainder of job");
		cancel_dfu(NRF_CLOUD_FOTA_ERROR_APPLY_FAIL);
		return true;
	}

	/* update globals */
	init_packet = strstr(fota_ble_job.info.path, ".dat") != NULL;

	LOG_INF("Starting second BLE DFU to addr:%s, from host:%s, "
		"path:%s, size:%d, init:%d",
		ble_dfu_addr, fota_ble_job.info.host,
		fota_ble_job.info.path, fota_ble_job.info.file_size,
		init_packet);

	int err = start_ble_job(&fota_ble_job);

	return (err == 0);
}

static void free_job(void)
{
	if (fota_ble_job.info.id) {
		free(fota_ble_job.info.id);
		fota_ble_job.info.id = NULL;
		if (fota_ble_job.info.host) {
			free(fota_ble_job.info.host);
			fota_ble_job.info.host = NULL;
		}
		memset(&fota_ble_job, 0, sizeof(fota_ble_job));
	}

	for (int i = 0; i < MAX_FOTA_FILES; i++) {
		if (!fota_files[i].path) {
			break;
		}
		k_free(fota_files[i].path);
		fota_files[i].path = NULL;
		fota_files[i].file_size = 0;
	}
}

static void cancel_dfu(enum nrf_cloud_fota_error error)
{
	ble_register_notify_callback(NULL);
	if (fota_ble_job.info.id) {
		enum nrf_cloud_fota_status status;
		int err;

		if (ble_conn_mgr_is_addr_connected(ble_dfu_addr)) {
			LOG_INF("Sending DFU Abort...");
			send_abort(ble_dfu_addr);
		}

		/* TODO: adjust error according to actual failure */
		fota_ble_job.error = error;
		status = NRF_CLOUD_FOTA_FAILED;

		err = nrf_cloud_fota_ble_job_update(&fota_ble_job, status);
		if (err) {
			LOG_ERR("Error updating job: %d", err);
		}
		free_job();
		LOGPKINF("Update cancelled.");
	}
	peripheral_dfu_cleanup();
	ble_conn_mgr_check_pending();
}

static uint8_t peripheral_dfu(const char *buf, size_t len)
{
	static size_t prev_percent;
	static size_t prev_update_percent;
	static uint32_t prev_crc;
	size_t percent = 0;
	int err = 0;

	if (first_fragment) {
		LOG_INF("Len:%zd, first:%u, size:%d, init:%u", len,
		       first_fragment, image_size, init_packet);
	}
	download_size = image_size;

	if (test_mode) {
		size_t percent = 0;

		if (first_fragment) {
			completed_size = 0;
			first_fragment = false;
		}
		completed_size += len;

		if (download_size) {
			percent = (100 * completed_size) / download_size;
		}

		LOG_INF("Progress: %zd%%; %zd of %zd",
		       percent, completed_size, download_size);

		if (percent >= 100) {
			LOG_INF("DFU complete");
		}
		return 0;
	}

	if (first_fragment) {
		first_fragment = false;
		LOGPKINF("BLE DFU starting to %s...", ble_dfu_addr);

		if (fota_ble_job.info.id) {
			fota_ble_job.dl_progress = (100 * total_completed_size) /
						   fota_ble_job.info.file_size;
			err = nrf_cloud_fota_ble_job_update(&fota_ble_job,
						    NRF_CLOUD_FOTA_DOWNLOADING);
			if (err) {
				LOG_ERR("Error updating job: %d", err);
				free_job();
				peripheral_dfu_cleanup();
				return err;
			}
		}
		prev_percent = 0;
		prev_update_percent = 0;
		completed_size = 0;
		prev_crc = 0;
		page_remaining = 0;
		finish_page = false;

		if (init_packet) {
			verbose = true;
			flash_page_size = 0;

			LOG_INF("Loading Init Packet and "
			       "turning on notifications...");
			err = ble_subscribe(ble_dfu_addr, DFU_CONTROL_POINT_UUID,
					    BT_GATT_CCC_NOTIFY);
			if (err) {
				goto cleanup;
			}

			LOG_INF("Setting DFU PRN to 0...");
			err = send_prn(ble_dfu_addr, 0);
			if (err) {
				goto cleanup;
			}

			LOG_INF("Querying hardware version...");
			(void)send_hw_version_get(ble_dfu_addr);

			LOG_INF("Querying firmware version (APP)...");
			(void)send_fw_version_get(ble_dfu_addr,
					     NRF_DFU_FIRMWARE_TYPE_APPLICATION);

			LOG_INF("Sending DFU select command...");
			err = send_select_command(ble_dfu_addr);
			if (err) {
				goto cleanup;
			}
			/* TODO: check offset and CRC; do no transfer
			 * and skip execute if offset != init length or
			 * CRC mismatch -- however, to do this, we need the
			 * cloud to send us the expected CRC for the file
			 */
			if (dfu_notify_packet_data->select.offset != image_size) {
				LOG_INF("Image size mismatched, so continue; "
				       "offset:%u, size:%u",
				       dfu_notify_packet_data->select.offset,
				       image_size);
			} else {
				LOG_INF("Image size matches device!");
				if (dfu_notify_packet_data->select.crc != original_crc) {
					LOG_INF("CRC mismatched, so continue; "
					       "dev crc:0x%08X, this crc:"
					       "0x%08X",
					       dfu_notify_packet_data->select.crc,
					       original_crc);
				} else {
					LOG_INF("CRC matches device!");
				}
			}

			LOG_INF("Sending DFU create command...");
			err = send_create_command(ble_dfu_addr, image_size);
			/* This will need to be the size of the entire init
			 * file. If http chunks it smaller this won't work
			 */
			if (err) {
				goto cleanup;
			}
		} else {
			verbose = false;
			finish_page = false;

			LOG_INF("Loading Firmware");
			LOG_INF("Setting DFU PRN to 0...");
			err = send_prn(ble_dfu_addr, 0);
			if (err) {
				goto cleanup;
			}

			LOG_INF("Sending DFU select data...");
			err = send_select_data(ble_dfu_addr);
			if (err) {
				goto cleanup;
			}
			if (dfu_notify_packet_data->select.offset != image_size) {
				LOG_INF("Image size mismatched, so continue; "
				       "offset:%u, size:%u",
				       dfu_notify_packet_data->select.offset,
				       image_size);
			} else {
				LOG_INF("Image size matches device!");
				if (dfu_notify_packet_data->select.crc != original_crc) {
					LOG_INF("CRC mismatched, so continue; "
					       "dev crc:0x%08X, this crc:"
					       "0x%08X",
					       dfu_notify_packet_data->select.crc,
					       original_crc);
				} else {
					LOG_INF("CRC matches device!");
					/* TODO: we could skip the entire file,
					 * and just send an indication to the
					 * cloud that the job is complete
					 */
				}
			}
		}
	}

	/* output data while handing page boundaries */
	while (len) {
		if (!finish_page) {
			uint32_t file_remaining = download_size - completed_size;

			page_remaining = MIN(max_size, file_remaining);

			LOG_DBG("Page remaining %u, max_size %u, len %u",
			       page_remaining, max_size, len);
			if (!init_packet) {
				err = send_create_data(ble_dfu_addr, page_remaining);
				if (err) {
					goto cleanup;
				}
			}
			if (len < page_remaining) {
				LOG_DBG("Need to transfer %u in page",
				       page_remaining);
				finish_page = true;
			} else {
				if (!completed_size) {
					LOG_DBG("No need to finish first page "
					       "next time");
				}
			}
		}

		size_t page_len = MIN(len, page_remaining);

		LOG_DBG("Sending DFU data len %u...", page_len);
		err = send_data(ble_dfu_addr, buf, page_len);
		if (err) {
			goto cleanup;
		}

		prev_crc = crc32_ieee_update(prev_crc, buf, page_len);

		completed_size += page_len;
		total_completed_size += page_len;
		len -= page_len;
		buf += page_len;
		LOG_DBG("Completed size: %u", completed_size);

		page_remaining -= page_len;
		if ((page_remaining == 0) ||
		    (completed_size == download_size)) {
			finish_page = false;
			LOG_DBG("Page is complete");
		} else {
			LOG_DBG("Page remaining: %u bytes", page_remaining);
		}

		if (!finish_page) {
			/* give hardware time to finish; omitting this step
			 * results in an error on the execute request and/or
			 * CRC mismatch; sometimes we are too fast so we
			 * need to retry, because the transfer was still
			 * going on
			 */
			for (int i = 1; i <= 5; i++) {
				k_sleep(K_MSEC(100));

				LOG_DBG("Sending DFU request CRC %d...", i);
				err = send_request_crc(ble_dfu_addr);
				if (err) {
					break;
				}
				if (dfu_notify_packet_data->crc.offset !=
				    completed_size) {
					err = -EIO;
				} else if (dfu_notify_packet_data->crc.crc !=
					 prev_crc) {
					err = -EBADMSG;
				} else {
					LOG_INF("CRC and length match.");
					err = 0;
					break;
				}
			}
			if (err == -EIO) {
				LOG_ERR("Transfer offset wrong; received: %u,"
					" expected: %u",
					dfu_notify_packet_data->crc.offset,
					completed_size);
			}
			if (err == -EBADMSG) {
				LOG_ERR("CRC wrong; received: 0x%08X, "
					"expected: 0x%08X",
					dfu_notify_packet_data->crc.crc,
					prev_crc);
			}
			if (err) {
				goto cleanup;
			}

			LOG_DBG("Sending DFU execute...");
			err = send_execute(ble_dfu_addr);
			if (err) {
				goto cleanup;
			}
		}
	}

	if (download_size) {
		percent = (100 * total_completed_size) / fota_ble_job.info.file_size;
	}

	if (percent != prev_percent) {
		LOGPKINF("Progress: %zd%%, %zd bytes of %zd", percent,
		       completed_size, download_size);
		prev_percent = percent;

		if (fota_ble_job.info.id &&
		    (((percent - prev_update_percent) >=
		      PROGRESS_UPDATE_INTERVAL) ||
		     (percent >= 100))) {
			enum nrf_cloud_fota_status status;

			LOG_INF("Sending job update at %zd%% percent",
			       percent);
			fota_ble_job.error = NRF_CLOUD_FOTA_ERROR_NONE;
			if (percent < 100) {
				status = NRF_CLOUD_FOTA_DOWNLOADING;
				fota_ble_job.dl_progress = percent;
			} else {
				status = NRF_CLOUD_FOTA_SUCCEEDED;
				fota_ble_job.dl_progress = 100;
				if (dfu_conn_ptr) {
					dfu_conn_ptr->dfu_pending = false;
				}
			}
			err = nrf_cloud_fota_ble_job_update(&fota_ble_job,
							    status);
			if (err) {
				LOG_ERR("Error updating job: %d", err);
				goto cleanup;
			}
			prev_update_percent = percent;
		}
	}

	return err;

cleanup:
	cancel_dfu(NRF_CLOUD_FOTA_ERROR_APPLY_FAIL);
	return err;
}

static int send_data(char *ble_addr, const char *buf, size_t len)
{
	/* "chunk" The data */
	int idx = 0;
	int err = 0;

	while (idx < len) {
		uint8_t size = MIN(MAX_CHUNK_SIZE, (len - idx));

		LOG_DBG("Sending write without response: %d, %d", size, idx);
		err = gatt_write_without_response(ble_addr, DFU_PACKET_UUID,
						  (uint8_t *)&buf[idx], size);
		if (err) {
			LOG_ERR("Error writing chunk at %d size %u: %d",
				idx, size, err);
			break;
		}
		idx += size;
	}
	return err;
}

/* name should be a string constant so log_strdup not needed */
static int do_cmd(char *ble_addr, bool normal_mode, uint8_t *buf, uint16_t len,
		  char *name, bool verbose)
{
	int err;
	const char *uuid;

	if (normal_mode) {
		uuid = DFU_BUTTONLESS_UUID;
	} else {
		uuid = DFU_CONTROL_POINT_UUID;
	}
	notify_received = false;

	err = gatt_write(ble_addr, uuid, buf, len, on_sent);
	if (err) {
		LOG_ERR("Error writing %s: %d", name, err);
		return err;
	}
	err = wait_for_notification();
	if (err) {
		LOG_ERR("Timeout waiting for notification from %s: %d",
			name, err);
		return err;
	}
	err = normal_mode ? decode_secure_dfu() : decode_dfu();
	if (err && verbose) {
		LOG_ERR("Notification decode error from %s: %d",
			name, err);
	}
	return err;
}

static int send_switch_to_dfu(char *ble_addr)
{
	char smol_buf[1];

	smol_buf[0] = DFU_OP_ENTER_BOOTLOADER;

	return do_cmd(ble_addr, true, smol_buf, sizeof(smol_buf), "DFU", true);
}


/* set number of writes between CRC reports; 0 disables */
static int send_prn(char *ble_addr, uint16_t receipt_rate)
{
	char smol_buf[3];

	smol_buf[0] = NRF_DFU_OP_RECEIPT_NOTIF_SET;
	smol_buf[1] = receipt_rate & 0xff;
	smol_buf[2] = (receipt_rate >> 8) & 0xff;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "PRN", true);
}

static int send_select_command(char *ble_addr)
{
	char smol_buf[2];

	smol_buf[0] = NRF_DFU_OP_OBJECT_SELECT;
	smol_buf[1] = 0x01;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Select Cmd", true);
}

static int send_select_data(char *ble_addr)
{
	char smol_buf[2];

	smol_buf[0] = NRF_DFU_OP_OBJECT_SELECT;
	smol_buf[1] = 0x02;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Select Data", true);
}

static int send_create_command(char *ble_addr, uint32_t size)
{
	uint8_t smol_buf[6];

	smol_buf[0] = NRF_DFU_OP_OBJECT_CREATE;
	smol_buf[1] = 0x01;
	smol_buf[2] = size & 0xFF;
	smol_buf[3] = (size >> 8) & 0xFF;
	smol_buf[4] = (size >> 16) & 0xFF;
	smol_buf[5] = (size >> 24) & 0xFF;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Create Cmd", true);
}

static int send_create_data(char *ble_addr, uint32_t size)
{
	uint8_t smol_buf[6];

	LOG_DBG("Size is %d", size);

	smol_buf[0] = NRF_DFU_OP_OBJECT_CREATE;
	smol_buf[1] = 0x02;
	smol_buf[2] = size & 0xFF;
	smol_buf[3] = (size >> 8) & 0xFF;
	smol_buf[4] = (size >> 16) & 0xFF;
	smol_buf[5] = (size >> 24) & 0xFF;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Create Data", true);
}

static void on_sent(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_write_params *params)
{
	const void *data;
	uint16_t length;

	/* TODO: Make a copy of volatile data that is passed to the
	 * callback.  Check err value at least in the wait function.
	 */
	data = params->data;
	length = params->length;

	LOG_DBG("Resp err:0x%02X, from write:", err);
	LOG_HEXDUMP_DBG(data, length, "sent");
}

static int send_request_crc(char *ble_addr)
{
	char smol_buf[1];

	/* Requesting 32 bit offset followed by 32 bit CRC */
	smol_buf[0] = NRF_DFU_OP_CRC_GET;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "CRC Request", true);
}

static int send_execute(char *ble_addr)
{
	char smol_buf[1];

	LOG_DBG("Sending Execute");

	smol_buf[0] = NRF_DFU_OP_OBJECT_EXECUTE;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Execute", true);
}

static int send_hw_version_get(char *ble_addr)
{
	char smol_buf[1];

	LOG_INF("Sending HW Version Get");

	smol_buf[0] = NRF_DFU_OP_HARDWARE_VERSION;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Get HW Ver", false);
}

static int send_fw_version_get(char *ble_addr, uint8_t fw_type)
{
	char smol_buf[2];

	LOG_INF("Sending FW Version Get");

	smol_buf[0] = NRF_DFU_OP_FIRMWARE_VERSION;
	smol_buf[1] = fw_type;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Get FW Ver", false);
}

static int send_abort(char *ble_addr)
{
	char smol_buf[1];

	smol_buf[0] = 0x0C;

	return do_cmd(ble_addr, false, smol_buf,
		      sizeof(smol_buf), "Abort", true);
}
