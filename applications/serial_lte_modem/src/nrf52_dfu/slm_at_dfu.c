/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <pm_config.h>
#include <dfu/mcuboot.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <net/http_parser_url.h>
#include <net/download_client.h>
#include <nrf_socket.h>
#include "slm_at_host.h"
#include "slm_at_dfu.h"
#include "dfu_target_nrf52.h"
#include "dfu_host.h"
#include "slm_util.h"

LOG_MODULE_REGISTER(dfu, CONFIG_SLM_LOG_LEVEL);

/* file_uri: scheme://hostname[:port]path */
#define FILE_URI_MAX	256
#define SCHEMA_HTTP	"http"
#define SCHEMA_HTTPS	"https"
#define URI_HOST_MAX	64
#define URI_PATH_MAX	128
#define APN_MAX		32

#define FILE_BUF_LEN (CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE)

static enum slm_dfu_operation {
	DFU_INIT_PACKET_DOWNLOAD,
	DFU_FILE_IMAGE_DOWNLOAD,
	DFU_MODE_CHECK,
	DFU_INITIALIZE,
	DFU_INIT_PACKET_TRANSFER,
	DFU_FILE_IMAGE_TRANSFER
} dfu_operation;

static K_SEM_DEFINE(sem_init_packet, 0, 1);
static K_SEM_DEFINE(sem_file_image, 0, 1);

static struct k_work_delayable dfu_work;
static uint8_t dfu_buf[CONFIG_SLM_DFU_FLASH_BUF_SZ];
static uint8_t init_packet[CONFIG_SLM_DFU_INIT_PACKET_SZ];
static uint16_t init_packet_size;
static uint32_t file_image_size;

static char hostname[URI_HOST_MAX];
static char filepath[URI_PATH_MAX];
static struct download_client dlc;
static int socket_retries_left;
static bool first_fragment;

/* global functions defined in different resources */
void rsp_send(const uint8_t *str, size_t len);
void enter_dfumode(void);
void exit_dfumode(void);
int poweroff_uart(void);
int poweron_uart(void);

/* global variable defined in different resources */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];
extern struct k_work_q slm_work_q;

static void dfu_wk(struct k_work *work)
{
	int ret = 0;
	uint32_t file_image = PM_MCUBOOT_SECONDARY_ADDRESS;

	ARG_UNUSED(work);

	enter_dfumode();
	rsp_buf[0] = '\0';

	dfu_operation = DFU_MODE_CHECK;
	if (!dfu_host_bl_mode_check()) {
		ret = -ENOEXEC;
		LOG_ERR("bootloader mode check fails");
		sprintf(rsp_buf, "\r\n#XDFU: %d,-1\r\n", dfu_operation);
		goto done;
	}

	dfu_operation = DFU_INITIALIZE;
	ret = dfu_host_setup();
	if (ret) {
		LOG_ERR("#XDFU: DFU initialize fails");
		sprintf(rsp_buf, "\r\n#XDFU: %d,-1\r\n", dfu_operation);
		goto done;
	}

	dfu_operation = DFU_INIT_PACKET_TRANSFER;
	ret = dfu_host_send_ip(init_packet, init_packet_size);
	if (ret) {
		LOG_ERR("DFU transfer init packet fails: %d\r\n", ret);
		sprintf(rsp_buf, "\r\n#XDFU: %d,%d\r\n", dfu_operation, ret);
		goto done;
	}

	dfu_operation = DFU_FILE_IMAGE_TRANSFER;
	ret = dfu_host_send_fw((const uint8_t *)file_image, file_image_size);
	if (ret) {
		LOG_ERR("\r\n#XDFU: DFU transfer file image fails: %d\r\n", ret);
		sprintf(rsp_buf, "\r\n#XDFU: %d,%d\r\n", dfu_operation, ret);
		goto done;
	}

	LOG_INF("DFU done successfully");

done:
	exit_dfumode();
	if (ret) {
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

static int download_client_callback(const struct download_client_evt *event)
{
	static size_t download_size;
	int err;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT: {
		LOG_DBG("EVT_FRAGMENT");
		if (first_fragment) {
			err = download_client_file_size_get(&dlc, &file_image_size);
			if (err != 0) {
				LOG_ERR("download_client_file_size_get err: %d", err);
				return err;
			}
			/* Init Packet is small and should be received in first fragment */
			if (dfu_operation == DFU_INIT_PACKET_DOWNLOAD) {
				init_packet_size = event->fragment.len;
				memcpy(init_packet, event->fragment.buf, init_packet_size);
				first_fragment = true;
				return 0;
			}
			/* File image download */
			err = dfu_target_nrf52_init(file_image_size);
			if ((err < 0) && (err != -EBUSY)) {
				LOG_ERR("dfu_target_init error %d", err);
				(void)download_client_disconnect(&dlc);
				first_fragment = true;
				return err;
			}
			first_fragment = false;
			download_size = event->fragment.len;
		} else {
			download_size += event->fragment.len;
		}

		err = dfu_target_nrf52_write(event->fragment.buf, event->fragment.len);
		if (err != 0) {
			LOG_ERR("dfu_target_write error %d", err);
			int res = dfu_target_nrf52_done(false);

			if (res != 0) {
				LOG_ERR("Unable to free DFU target resources");
			}
			first_fragment = true;
			(void)download_client_disconnect(&dlc);
			return err;
		}

		break;
	}

	case DOWNLOAD_CLIENT_EVT_DONE:
		LOG_DBG("EVT_DONE");
		if (dfu_operation == DFU_FILE_IMAGE_DOWNLOAD) {
			err = dfu_target_nrf52_done(true);
			if (err != 0) {
				LOG_ERR("dfu_target_done error: %d", err);
				return err;
			}
		}

		err = download_client_disconnect(&dlc);
		if (err != 0) {
			LOG_ERR("download_client_disconnect error: %d", err);
			return err;
		}
		first_fragment = true;
		if (dfu_operation == DFU_INIT_PACKET_DOWNLOAD) {
			LOG_INF("Init Packet downloaded");
			k_sem_give(&sem_init_packet);
		} else {
			LOG_INF("File Image downloaded");
			k_sem_give(&sem_file_image);
		}
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR:
		LOG_INF("EVT_ERROR");
		/* In case of socket errors we can return 0 to retry/continue,
		 * or non-zero to stop
		 */
		if ((socket_retries_left) && ((event->error == -ENOTCONN) ||
					      (event->error == -ECONNRESET))) {
			LOG_WRN("Download socket error. %d retries left...", socket_retries_left);
			socket_retries_left--;
			/* Fall through and return 0 below to tell download_client to retry */
		} else {
			download_client_disconnect(&dlc);
			LOG_ERR("Download client error");
			if (dfu_operation == DFU_FILE_IMAGE_DOWNLOAD) {
				err = dfu_target_nrf52_done(false);
				if (err == -EACCES) {
					LOG_DBG("No DFU target was initialized");
				} else if (err != 0) {
					LOG_ERR("Unable to deinitialze resources");
				}
			}
			first_fragment = true;
			/* Return non-zero to tell download_client to stop */
			return event->error;
		}

	default:
		break;
	}

	return 0;
}

static int dfu_download_init(void)
{
	int err;

	err = download_client_init(&dlc, download_client_callback);
	if (err != 0) {
		LOG_ERR("download_client_init error: %d", err);
		return err;
	}

	first_fragment = true;
	return 0;
}

static  int dfu_download_start(const char *host, const char *file, int sec_tag,
			const char *apn, size_t fragment_size)
{
	/* We need a static file buffer since the download client structure
	 * only keeps a pointer to the file buffer. This is problematic when
	 * a download needs to be restarted for some reason (e.g. if
	 * continuing a download operation from an offset).
	 */
	static char file_buf[FILE_BUF_LEN];
	const char *file_buf_ptr = file_buf;
	int err = -1;

	struct download_client_cfg config = {
		.sec_tag = sec_tag,
		.apn = apn,
		.frag_size_override = fragment_size,
		.set_tls_hostname = (sec_tag != -1),
	};

	if (host == NULL || file == NULL) {
		return -EINVAL;
	}

	socket_retries_left = CONFIG_SLM_DFU_SOCKET_RETRIES;
	strncpy(file_buf, file, sizeof(file_buf));
	err = download_client_connect(&dlc, host, &config);
	if (err != 0) {
		return err;
	}

	err = download_client_start(&dlc, file_buf_ptr, 0);
	if (err != 0) {
		download_client_disconnect(&dlc);
		return err;
	}

	return 0;
}

static int do_dfu_download(const char *host, const char *path, int sec_tag, const char *apn)
{
	int ret;
	struct http_parser_url parser = {
		/* UNINIT checker fix, assumed UF_SCHEMA existence */
		.field_set = 0
	};
	char schema[8];

	http_parser_url_init(&parser);
	ret = http_parser_parse_url(host, strlen(host), 0, &parser);
	if (ret) {
		LOG_ERR("Parse URL error");
		return -EINVAL;
	}
	memset(schema, 0x00, 8);
	if (parser.field_set & (1 << UF_SCHEMA)) {
		strncpy(schema, host + parser.field_data[UF_SCHEMA].off,
			parser.field_data[UF_SCHEMA].len);
	} else {
		LOG_ERR("Parse schema error");
		return -EINVAL;
	}
	memset(filepath, 0x00, URI_PATH_MAX);
	strcpy(filepath, path);
	memset(hostname, 0x00, URI_HOST_MAX);
	strcpy(hostname, host);

	/* start HTTP(S) FOTA */
	if (slm_util_cmd_casecmp(schema, SCHEMA_HTTPS)) {
		if (sec_tag == INVALID_SEC_TAG) {
			LOG_ERR("Missing sec_tag");
			return -EINVAL;
		}
		ret = dfu_download_start(hostname, filepath, sec_tag, apn, 0);
	} else if (slm_util_cmd_casecmp(schema, SCHEMA_HTTP)) {
		ret = dfu_download_start(hostname, filepath, -1, apn, 0);
	} else {
		ret = -EINVAL;
	}
	if (ret) {
		LOG_ERR("Failed to start download: %d", ret);
	}

	return ret;
}

/**@brief handle AT#XDFUDOWNLOAD commands
 *  AT#XDFUDOWNLOAD=<host>,<init_packet>,<file_image>[,<sec_tag>[,<apn>]]
 *  AT#XDFUDOWNLOAD? READ command not supported
 *  AT#XDFUDOWNLOAD=?
 */
int handle_at_dfu_download(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char host[URI_HOST_MAX];
	char ip_file[FILE_URI_MAX];
	char fw_file[FILE_URI_MAX];
	char apn_str[APN_MAX];
	char *apn_ptr = NULL;
	int size;
	sec_tag_t sec_tag = INVALID_SEC_TAG;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		size = URI_HOST_MAX;
		err = util_string_get(&at_param_list, 1, host, &size);
		if (err) {
			return err;
		}
		size = FILE_URI_MAX;
		err = util_string_get(&at_param_list, 2, ip_file, &size);
		if (err) {
			return err;
		}
		size = FILE_URI_MAX;
		err = util_string_get(&at_param_list, 3, fw_file, &size);
		if (err) {
			return err;
		}
		if (at_params_valid_count_get(&at_param_list) > 4) {
			at_params_unsigned_int_get(&at_param_list, 4, &sec_tag);
		}
		if (at_params_valid_count_get(&at_param_list) > 5) {
			size = APN_MAX;
			err = util_string_get(&at_param_list, 5, apn_str, &size);
			if (err) {
				return err;
			}
			apn_ptr = apn_str;
		}

		/* Download DFU files */
		dfu_operation = DFU_INIT_PACKET_DOWNLOAD;
		err = do_dfu_download(host, ip_file, sec_tag, apn_ptr);
		if (err) {
			sprintf(rsp_buf, "\r\n#XDFU: %d,%d\r\n", dfu_operation, err);
			rsp_send(rsp_buf, strlen(rsp_buf));
			return err;
		}
		k_sem_take(&sem_init_packet, K_FOREVER);

		dfu_operation = DFU_FILE_IMAGE_DOWNLOAD;
		err = do_dfu_download(host, fw_file, sec_tag, apn_ptr);
		if (err) {
			sprintf(rsp_buf, "\r\n#XDFU: %d,%d\r\n", dfu_operation, err);
			rsp_send(rsp_buf, strlen(rsp_buf));
			return err;
		}
		k_sem_take(&sem_file_image, K_FOREVER);
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf,
			"\r\n#XDFUDOWNLOAD=<host>,<init_packet>,<file_image>,<sec_tag>,<apn>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

/**@brief handle AT#XDFUSTART commands
 *  AT#XDFUSTART=<delay>
 *  AT#XDFUSTART? READ command not supported
 *  AT#XDFUSTART=?
 */
int handle_at_dfu_start(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t delay = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &delay);
		if (err < 0) {
			return err;
		}
		if (delay > 0) {
			k_work_schedule(&dfu_work, K_SECONDS(delay));
		} else {
			k_work_schedule(&dfu_work, K_NO_WAIT);
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XDFUSTART=<delay>");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

int slm_at_dfu_init(void)
{
	k_work_init_delayable(&dfu_work, dfu_wk);
	(void)dfu_target_nrf52_set_buf(dfu_buf, sizeof(dfu_buf));
	(void)dfu_download_init();

	return 0;
}

int slm_at_dfu_uninit(void)
{
	return 0;
}
