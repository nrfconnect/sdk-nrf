/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <pm_config.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/crc.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/parser_url.h>
#include <net/download_client.h>
#include <nrf_socket.h>
#include "slm_at_host.h"
#include "slm_at_dfu.h"
#include "dfu_target_nrf52.h"
#include "dfu_host.h"
#include "slm_util.h"

LOG_MODULE_REGISTER(dfu, CONFIG_SLM_LOG_LEVEL);

/* file_uri: scheme://hostname[:port]path */
#define SCHEMA_HTTP	"http"
#define SCHEMA_HTTPS	"https"
#define URI_HOST_MAX	64
#define PERCENTAGE_STEP 5

enum slm_dfu_operation {
	SLM_DFU_CANCEL,
	SLM_DFU_DOWNLOAD,
	SLM_DFU_ERASE = 8
};

static enum slm_dfu_steps {
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
	DFU_INIT_PACKET_DOWNLOAD,
	DFU_FILE_IMAGE_DOWNLOAD,
	DFU_MODE_CHECK,
	DFU_INITIALIZE,
	DFU_INIT_PACKET_TRANSFER,
#else
	DFU_FILE_IMAGE_DOWNLOAD,
	DFU_INITIALIZE,
#endif
	DFU_FILE_IMAGE_TRANSFER,
	DFU_DONE
} dfu_step;

static struct k_work_delayable dfu_work;
static uint8_t dfu_buf[CONFIG_SLM_DFU_FLASH_BUF_SZ] __aligned(4);
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
static uint8_t init_packet[CONFIG_SLM_DFU_INIT_PACKET_SZ];
static uint16_t init_packet_size;
static K_SEM_DEFINE(sem_init_packet, 0, 1);
#endif
static uint32_t file_image_size;
static uint32_t file_download_size;
static uint32_t crc_32;

static char hostname[URI_HOST_MAX];
static char filepath[SLM_MAX_URL];
static struct download_client dlc;
static int socket_retries_left;
static bool first_fragment;
static uint16_t mtu;
static uint16_t pause;


/* global functions defined in different resources */
void enter_dfumode(void);
void exit_dfumode(void);

/* global variable defined in different resources */
extern struct at_param_list at_param_list;
extern struct k_work_q slm_work_q;

static void dfu_wk(struct k_work *work)
{
	int ret = 0;
	uint32_t file_image = PM_MCUBOOT_SECONDARY_ADDRESS;

	ARG_UNUSED(work);

	enter_dfumode();

#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
	dfu_step = DFU_MODE_CHECK;
	if (!dfu_host_bl_mode_check()) {
		ret = -ENOEXEC;
		LOG_ERR("bootloader mode check fails");
		rsp_send("\r\n#XDFURUN: %d,-1\r\n", dfu_step);
		goto done;
	}
#endif

	dfu_step = DFU_INITIALIZE;
	ret = dfu_host_setup(mtu, pause);
	if (ret) {
		LOG_ERR("#XDFU: DFU initialize fails");
		rsp_send("\r\n#XDFURUN: %d,-1\r\n", dfu_step);
		goto done;
	}

#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
	dfu_step = DFU_INIT_PACKET_TRANSFER;
	ret = dfu_host_send_ip(init_packet, init_packet_size);
	if (ret) {
		LOG_ERR("DFU transfer init packet fails: %d\r\n", ret);
		rsp_send("\r\n#XDFURUN: %d,%d\r\n", dfu_step, ret);
		goto done;
	}
#endif

	dfu_step = DFU_FILE_IMAGE_TRANSFER;
	ret = dfu_host_send_fw((const uint8_t *)file_image, file_image_size);
	if (ret) {
		LOG_ERR("\r\n#XDFU: DFU transfer file image fails: %d\r\n", ret);
		rsp_send("\r\n#XDFURUN: %d,%d\r\n", dfu_step, ret);
		goto done;
	}

	dfu_step = DFU_DONE;
	LOG_INF("DFU done successfully");

done:
	exit_dfumode();
}

static int do_dfu_erase(void)
{
	int ret;

	LOG_INF("Erasing mcuboot secondary");

	ret = boot_erase_img_bank(PM_MCUBOOT_SECONDARY_ID);
	if (ret) {
		LOG_ERR("boot_erase_img_bank error: %d", ret);
	} else {
		LOG_INF("Erase completed");
	}

	return ret;
}

static int download_client_callback(const struct download_client_evt *event)
{
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
				rsp_send("\r\n#XDFUGET: %d,%d\r\n", dfu_step, err);
				return err;
			}
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
			/* Init Packet is small and should be received in first fragment */
			if (dfu_step == DFU_INIT_PACKET_DOWNLOAD) {
				init_packet_size = event->fragment.len;
				memcpy(init_packet, event->fragment.buf, init_packet_size);
				first_fragment = true;
				return 0;
			}
#endif
			/* File image download */
			err = dfu_target_nrf52_init(file_image_size);
			if ((err < 0) && (err != -EBUSY)) {
				LOG_ERR("dfu_target_init error %d", err);
				(void)download_client_disconnect(&dlc);
				first_fragment = true;
				rsp_send("\r\n#XDFUGET: %d,%d\r\n", dfu_step, err);
				return err;
			}
			first_fragment = false;
			file_download_size = event->fragment.len;
			crc_32 = crc32_ieee(event->fragment.buf, event->fragment.len);
		} else {
			file_download_size += event->fragment.len;
			crc_32 = crc32_ieee_update(crc_32, event->fragment.buf,
						   event->fragment.len);
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
			rsp_send("\r\n#XDFUGET: %d,%d\r\n", dfu_step, err);
			return err;
		}

		if (!first_fragment) {
			uint32_t percentage = 0;

			percentage = (file_download_size * 100) / file_image_size;
			if ((percentage % PERCENTAGE_STEP) == 0) {
				rsp_send("\r\n#XDFUGET: %d,%d\r\n", dfu_step, percentage);
			}
		}
		break;
	}

	case DOWNLOAD_CLIENT_EVT_DONE:
		LOG_DBG("EVT_DONE");
		if (dfu_step == DFU_FILE_IMAGE_DOWNLOAD) {
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
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
		if (dfu_step == DFU_INIT_PACKET_DOWNLOAD) {
			LOG_INF("Init Packet downloaded");
			k_sem_give(&sem_init_packet);
		} else {
			LOG_INF("File Image downloaded");
		}
#else
		LOG_INF("File Image downloaded");
#endif
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
			if (dfu_step == DFU_FILE_IMAGE_DOWNLOAD) {
				err = dfu_target_nrf52_done(false);
				if (err == -EACCES) {
					LOG_DBG("No DFU target was initialized");
				} else if (err != 0) {
					LOG_ERR("Unable to deinitialze resources");
				}
			}
			first_fragment = true;
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
			if (dfu_step == DFU_INIT_PACKET_DOWNLOAD) {
				k_sem_give(&sem_init_packet);
			}
#endif
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
			size_t fragment_size)
{
	/* We need a static file buffer since the download client structure
	 * only keeps a pointer to the file buffer. This is problematic when
	 * a download needs to be restarted for some reason (e.g. if
	 * continuing a download operation from an offset).
	 */
	static char file_buf[CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE];
	const char *file_buf_ptr = file_buf;
	int err = -1;

	struct download_client_cfg config = {
		.sec_tag = sec_tag,
		.frag_size_override = fragment_size,
		.set_tls_hostname = (sec_tag != -1),
	};

	if (host == NULL || file == NULL) {
		return -EINVAL;
	}

	socket_retries_left = CONFIG_SLM_DFU_SOCKET_RETRIES;
	strncpy(file_buf, file, sizeof(file_buf));
	err = download_client_set_host(&dlc, host, &config);
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

static int do_dfu_download(const char *host, const char *path, int sec_tag)
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
	memset(filepath, 0x00, SLM_MAX_URL);
	strcpy(filepath, path);
	memset(hostname, 0x00, URI_HOST_MAX);
	strcpy(hostname, host);

	/* start HTTP(S) FOTA */
	if (slm_util_cmd_casecmp(schema, SCHEMA_HTTPS)) {
		if (sec_tag == INVALID_SEC_TAG) {
			LOG_ERR("Missing sec_tag");
			return -EINVAL;
		}
		ret = dfu_download_start(hostname, filepath, sec_tag, 0);
	} else if (slm_util_cmd_casecmp(schema, SCHEMA_HTTP)) {
		ret = dfu_download_start(hostname, filepath, -1, 0);
	} else {
		ret = -EINVAL;
	}
	if (ret) {
		LOG_ERR("Failed to start download: %d", ret);
	}

	return ret;
}

/**@brief handle AT#XDFUGET commands
 *  AT#XDFUGET=<op>[,<host>,<image_1>[,<image_2>[,<sec_tag>]]]
 *  AT#XDFUGET? READ command not supported
 *  AT#XDFUGET=?
 */
int handle_at_dfu_get(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	char host[URI_HOST_MAX] = { 0 };
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
	char ip_file[SLM_MAX_URL] = { 0 };
#endif
	char fw_file[SLM_MAX_URL] = { 0 };
	int size;
	sec_tag_t sec_tag = INVALID_SEC_TAG;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == SLM_DFU_CANCEL) {
			if (dlc.fd == -1) {
				LOG_WRN("Invalid state");
				return -EAGAIN;
			}
			err = download_client_disconnect(&dlc);
			if (err) {
				LOG_ERR("Failed to disconnect: %d", err);
				return err;
			}
			err = dfu_target_done(false);
			if (err && err != -EACCES) {
				LOG_ERR("Failed to clean up: %d", err);
			} else {
				first_fragment = true;
			}
		} else if (op == SLM_DFU_DOWNLOAD) {
			size = URI_HOST_MAX;
			err = util_string_get(&at_param_list, 2, host, &size);
			if (err) {
				return err;
			}
			if (strlen(host) == 0) {
				return -EINVAL;
			}
#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
			size = SLM_MAX_URL;
			err = util_string_get(&at_param_list, 3, ip_file, &size);
			if (err) {
				return err;
			}
			size = SLM_MAX_URL;
			err = util_string_get(&at_param_list, 4, fw_file, &size);
			if (err) {
				return err;
			}
			if (strlen(ip_file) == 0 || strlen(fw_file) == 0) {
				return -EINVAL;
			}
#else
			size = SLM_MAX_URL;
			err = util_string_get(&at_param_list, 3, fw_file, &size);
			if (err) {
				return err;
			}
			if (strlen(fw_file) == 0) {
				return -EINVAL;
			}
#endif
			if (at_params_valid_count_get(&at_param_list) > 5) {
				(void)at_params_unsigned_int_get(&at_param_list, 5, &sec_tag);
			}

#if defined(CONFIG_SLM_NRF52_DFU_LEGACY)
			/* Download DFU init data files */
			dfu_step = DFU_INIT_PACKET_DOWNLOAD;
			err = do_dfu_download(host, ip_file, sec_tag);
			if (err) {
				rsp_send("\r\n#XDFUGET: %d,%d\r\n", dfu_step, err);
				return err;
			}
			k_sem_take(&sem_init_packet, K_FOREVER);
#endif
			/* Download DFU image files */
			dfu_step = DFU_FILE_IMAGE_DOWNLOAD;
			file_image_size = 0;
			file_download_size = 0;
			crc_32 = 0;
			err = do_dfu_download(host, fw_file, sec_tag);
		} else if (op == SLM_DFU_ERASE) {
			err = do_dfu_erase();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XDFUGET=(%d,%d,%d),<host>,<image_1>,<image_2>,<sec_tag>\r\n",
			SLM_DFU_CANCEL, SLM_DFU_DOWNLOAD, SLM_DFU_ERASE);
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

/**@brief handle AT#XDFUSIZE commands
 *  AT#XDFUSIZE
 *  AT#XDFUSIZE? READ command not supported
 *  AT#XDFUSIZE=? TEST command not supported
 */
int handle_at_dfu_size(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		rsp_send("\r\n#XDFUSIZE: %d,%d,%d\r\n", file_image_size, file_download_size,
			 crc_32);
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

/**@brief handle AT#XDFURUN commands
 *  AT#XDFURUN=<start_delay>[,<mtu>[,<pause>]]
 *  AT#XDFURUN? READ command not supported
 *  AT#XDFURUN=?
 */
int handle_at_dfu_run(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t delay;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &delay);
		if (err < 0) {
			return err;
		}
#if !defined(CONFIG_SLM_NRF52_DFU_LEGACY)
		err = at_params_unsigned_short_get(&at_param_list, 2, &mtu);
		if (err < 0) {
			return err;
		}
		if (mtu > SLM_NRF52_BLK_SIZE || mtu % 256 != 0) {
			return -EINVAL;
		}
		err = at_params_unsigned_short_get(&at_param_list, 3, &pause);
		if (err < 0) {
			return err;
		}
		if (pause == 0) {
			return -EINVAL;
		}
#endif
		if (delay > 0) {
			k_work_schedule(&dfu_work, K_SECONDS(delay));
		} else {
			k_work_schedule(&dfu_work, K_NO_WAIT);
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XDFURUN=<start_delay>,<mtu>,<pause>\r\n");
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
