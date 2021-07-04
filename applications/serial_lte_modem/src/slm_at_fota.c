/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>

#include <zephyr.h>
#include <stdio.h>
#include <nrf_socket.h>
#include <dfu/mcuboot.h>
#include <net/tls_credentials.h>
#include <net/http_parser_url.h>
#include <net/fota_download.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_fota.h"
#include "pm_config.h"

LOG_MODULE_REGISTER(slm_fota, CONFIG_SLM_LOG_LEVEL);

/* file_uri: scheme://hostname[:port]path */
#define FILE_URI_MAX	256
#define SCHEMA_HTTP	"http"
#define SCHEMA_HTTPS	"https"
#define URI_HOST_MAX	64

#define APN_MAX		32

/* Some features need fota_download update */
#define FOTA_FUTURE_FEATURE	0

enum slm_fota_operation {
	AT_FOTA_STOP,
	AT_FOTA_START_APP,
	AT_FOTA_START_MFW,
	AT_FOTA_PAUSE_RESUME,
	AT_FOTA_ERASE_APP = 8,
	AT_FOTA_ERASE_MFW
};

static char path[SLM_MAX_URL];
static char hostname[URI_HOST_MAX];

/* global functions defined in different files */
int slm_setting_fota_init(void);
int slm_setting_fota_save(void);

/* global variable defined in different files */
extern char rsp_buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];
extern struct at_param_list at_param_list;
extern uint8_t fota_stage;
extern uint8_t fota_status;
extern int32_t fota_info;

static int do_fota_erase_app(void)
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

static int do_fota_erase_mfw(void)
{
	int fd;
	int err;
	nrf_dfu_fw_offset_t offset;
	nrf_socklen_t len = sizeof(offset);

	LOG_INF("Erasing modem scratch");

	fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_STREAM, NRF_PROTO_DFU);
	if (fd < 0) {
		LOG_ERR("nrf_socket error: %d", errno);
		return -errno;
	}
	err = nrf_setsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_BACKUP_DELETE, NULL, 0);
	if (err) {
		LOG_ERR("nrf_setsockopt error: %d", errno);
		nrf_close(fd);
		return -errno;
	}
	while (true) {
		err = nrf_getsockopt(fd, NRF_SOL_DFU, NRF_SO_DFU_OFFSET, &offset, &len);
		if (err < 0) {
			k_sleep(K_SECONDS(1));
		} else {
			LOG_INF("Erase completed");
			break;
		}
	}

	nrf_close(fd);
	return 0;
}

static int do_fota_start(int op, const char *file_uri, int sec_tag,
			 const char *apn, enum dfu_target_image_type type)
{
	int ret;
	struct http_parser_url parser = {
		/* UNINIT checker fix, assumed UF_SCHEMA existence */
		.field_set = 0
	};
	char schema[8];

	http_parser_url_init(&parser);
	ret = http_parser_parse_url(file_uri, strlen(file_uri), 0, &parser);
	if (ret) {
		LOG_ERR("Parse URL error");
		return -EINVAL;
	}
	memset(schema, 0x00, 8);
	if (parser.field_set & (1 << UF_SCHEMA)) {
		strncpy(schema, file_uri + parser.field_data[UF_SCHEMA].off,
			parser.field_data[UF_SCHEMA].len);
	} else {
		LOG_ERR("Parse schema error");
		return -EINVAL;
	}
	memset(path, 0x00, SLM_MAX_URL);
	if (parser.field_set & (1 << UF_PATH)) {
		strncpy(path, file_uri + parser.field_data[UF_PATH].off,
			parser.field_data[UF_PATH].len);
	} else {
		LOG_ERR("Parse path error");
		return -EINVAL;
	}

	memset(hostname, 0x00, URI_HOST_MAX);
	strncpy(hostname, file_uri, strlen(file_uri) - parser.field_data[UF_PATH].len);

	/* start HTTP(S) FOTA */
	if (slm_util_cmd_casecmp(schema, SCHEMA_HTTPS)) {
		if (sec_tag == INVALID_SEC_TAG) {
			LOG_ERR("Missing sec_tag");
			return -EINVAL;
		}
		ret = fota_download_start_with_image_type(hostname, path, sec_tag, apn, 0, type);
	} else if (slm_util_cmd_casecmp(schema, SCHEMA_HTTP)) {
		ret = fota_download_start_with_image_type(hostname, path, -1, apn, 0, type);
	} else {
		ret = -EINVAL;
	}
	/* Send an URC if failed to start */
	if (ret) {
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d,%d\r\n", FOTA_STAGE_DOWNLOAD,
			FOTA_STATUS_ERROR, ret);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		fota_stage = FOTA_STAGE_DOWNLOAD;
		fota_status = FOTA_STATUS_OK;
		fota_info = evt->progress;
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d,%d\r\n", fota_stage, fota_status, fota_info);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		fota_stage = FOTA_STAGE_ACTIVATE;
		fota_info = 0;
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		/* Save, in case reboot by reset */
		slm_setting_fota_save();
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
		fota_stage = FOTA_STAGE_DOWNLOAD_ERASE_PENDING;
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		fota_stage = FOTA_STAGE_DOWNLOAD_ERASED;
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		fota_status = FOTA_STATUS_ERROR;
		fota_info = evt->cause;
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d,%d\r\n", fota_stage, fota_status, fota_info);
		/* FOTA session terminated */
		slm_setting_fota_init();
		break;

	case FOTA_DOWNLOAD_EVT_CANCELLED:
		fota_status = FOTA_STATUS_CANCELLED;
		fota_info = 0;
		sprintf(rsp_buf, "\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		/* FOTA session terminated */
		slm_setting_fota_init();
		break;

	default:
		return;
	}
	rsp_send(rsp_buf, strlen(rsp_buf));
}

/**@brief handle AT#XFOTA commands
 *  AT#XFOTA=<op>,<file_uri>[,<sec_tag>[,<apn>]]
 *  AT#XFOTA? TEST command not supported
 *  AT#XFOTA=?
 */
int handle_at_fota(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
#if FOTA_FUTURE_FEATURE
	static bool paused;
#endif

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == AT_FOTA_STOP) {
			err = fota_download_cancel();
		} else if (op == AT_FOTA_START_APP || op == AT_FOTA_START_MFW) {
			char uri[FILE_URI_MAX];
			char apn[APN_MAX];
			int size = FILE_URI_MAX;
			sec_tag_t sec_tag = INVALID_SEC_TAG;
			enum dfu_target_image_type type;

			err = util_string_get(&at_param_list, 2, uri, &size);
			if (err) {
				return err;
			}
			if (at_params_valid_count_get(&at_param_list) > 3) {
				at_params_unsigned_int_get(&at_param_list, 3, &sec_tag);
			}
			if (op == AT_FOTA_START_APP) {
				type = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
			} else {
				type = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
			}
			if (at_params_valid_count_get(&at_param_list) > 4) {
				size = APN_MAX;
				err = util_string_get(&at_param_list, 4, apn, &size);
				if (err) {
					return err;
				}
				err = do_fota_start(op, uri, sec_tag, apn, type);
			} else {
				err = do_fota_start(op, uri, sec_tag, NULL, type);
			}
#if FOTA_FUTURE_FEATURE
		} else if (op == AT_FOTA_PAUSE_RESUME) {
			if (paused) {
				fota_download_resume();
				paused = false;
			} else {
				fota_download_pause();
				paused = true;
			}
			err = 0;
#endif
		} else if (op == AT_FOTA_ERASE_APP) {
			err = do_fota_erase_app();
		} else if (op == AT_FOTA_ERASE_MFW) {
			err = do_fota_erase_mfw();
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XFOTA: (%d,%d,%d,%d,%d),<file_uri>,<sec_tag>,<apn>\r\n",
			AT_FOTA_STOP, AT_FOTA_START_APP, AT_FOTA_START_MFW,
			AT_FOTA_ERASE_APP, AT_FOTA_ERASE_MFW);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize FOTA AT commands handler
 */
int slm_at_fota_init(void)
{
	return fota_download_init(fota_dl_handler);
}

/**@brief API to uninitialize FOTA AT commands handler
 */
int slm_at_fota_uninit(void)
{
	return 0;
}

/**@brief API to handle post-FOTA processing
 */
void slm_fota_post_process(void)
{
	LOG_DBG("FOTA result %d,%d,%d", fota_stage, fota_status, fota_info);
	if (fota_stage != FOTA_STAGE_INIT) {
		/* report final result of last fota */
		if (fota_status == FOTA_STATUS_OK) {
			sprintf(rsp_buf, "\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		} else {
			sprintf(rsp_buf, "\r\n#XFOTA: %d,%d,%d\r\n", fota_stage, fota_status,
				fota_info);
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
	/* FOTA session completed */
	slm_setting_fota_init();
	slm_setting_fota_save();
}
