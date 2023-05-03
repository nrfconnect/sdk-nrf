/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>

#include <stdint.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <nrf_modem_delta_dfu.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/parser_url.h>
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
#define ERASE_WAIT_TIME 20
#define ERASE_POLL_TIME 2

/* Some features need fota_download update */
#define FOTA_FUTURE_FEATURE	0

enum slm_fota_operation {
	SLM_FOTA_STOP,
	SLM_FOTA_START_APP,
	SLM_FOTA_START_MFW,
	SLM_FOTA_START_BL,
	SLM_FOTA_PAUSE_RESUME,
	SLM_FOTA_APP_READ = 6,
	SLM_FOTA_MFW_READ,
	SLM_FOTA_ERASE_APP,
	SLM_FOTA_ERASE_MFW
};

static char path[SLM_MAX_URL];
static char hostname[URI_HOST_MAX];

/* global functions defined in different files */
int slm_setting_fota_init(void);
int slm_setting_fota_save(void);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern uint8_t fota_type;
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

static void do_fota_app_read(uint8_t area_id)
{
	int err;
	struct mcuboot_img_header header;

	err = boot_read_bank_header(area_id, &header, sizeof(header));
	if (err) {
		LOG_WRN("failed in area %d banker header: %d", area_id, err);
		return;
	}

	rsp_send("\r\n#XFOTA: %d,%d,%d,\"%d.%d.%d+%d\"\r\n", area_id,
		header.mcuboot_version,
		header.h.v1.image_size,
		header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor,
		header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num);
}

static int do_fota_mfw_read(void)
{
	int err;
	size_t offset = 0;
	size_t area = 0;

	err = nrf_modem_delta_dfu_area(&area);
	if (err) {
		LOG_ERR("failed in delta dfu area: %d", err);
		return err;
	}

	err = nrf_modem_delta_dfu_offset(&offset);
	if (err) {
		LOG_ERR("failed in delta dfu offset: %d", err);
		return err;
	}

	rsp_send("\r\n#XFOTA: %d,%d\r\n", area, offset);

	return 0;
}

static int do_fota_erase_mfw(void)
{
	int err;
	size_t offset = 0;
	int in_progress = false;

	err = nrf_modem_delta_dfu_offset(&offset);
	if (err) {
		if (err == NRF_MODEM_DELTA_DFU_ERASE_PENDING) {
			in_progress = true;
		} else {
			LOG_ERR("failed in delta dfu offset: %d", err);
			return err;
		}
	}

	if (offset == 0 && !in_progress) {
		/* no need of erasing */
		return 0;
	}

	if (!in_progress) {
		err = nrf_modem_delta_dfu_erase();
		if (err) {
			LOG_ERR("failed in delta dfu erase: %d", err);
			return err;
		}
	}

	int time_elapsed = 0;

	do {
		k_sleep(K_SECONDS(ERASE_POLL_TIME));
		err = nrf_modem_delta_dfu_offset(&offset);
		if (err != 0 && err != NRF_MODEM_DELTA_DFU_ERASE_PENDING) {
			LOG_ERR("failed in delta dfu offset: %d", err);
			return err;
		}
		if (err == 0 && offset == 0) {
			LOG_INF("Erase completed");
			break;
		}
		time_elapsed += ERASE_POLL_TIME;
	} while (time_elapsed < ERASE_WAIT_TIME);

	if (time_elapsed >= ERASE_WAIT_TIME) {
		LOG_WRN("Erase timeout");
		return -ETIME;
	}

	return 0;
}

static int do_fota_start(int op, const char *file_uri, int sec_tag,
			 uint8_t pdn_id, enum dfu_target_image_type type)
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
		/* Remove the leading '/' as some HTTP servers don't like it */
		strncpy(path, file_uri + parser.field_data[UF_PATH].off + 1,
			parser.field_data[UF_PATH].len - 1);
	} else {
		LOG_ERR("Parse path error");
		return -EINVAL;
	}

#if defined(CONFIG_SECURE_BOOT)
	/* When download MCUBOOT bootloader, S0 and S1 must be separated by "+"
	 * Convert "+" to " " SPACE as this is required in fota_download
	 */
	char *delimiter = strstr(path, "+");

	if (delimiter == NULL) {
		LOG_ERR("Invalid S0/S1 string");
		return -EINVAL;
	}

	*delimiter = ' ';

	/* When download MCUBOOT bootloader, S0 and S1 must be two full filepath
	 * as this is required in fota_download
	 */
	char *tmp = strrchr(path, '/');

	if (tmp != NULL) {
		char path2[SLM_MAX_URL] = { 0 };

		memcpy(path2, path, tmp - path + 1);
		tmp = delimiter + 1;
		strcat(path2, tmp);
		strcpy(tmp, path2);
	}
#endif

	memset(hostname, 0x00, URI_HOST_MAX);
	strncpy(hostname, file_uri, strlen(file_uri) - parser.field_data[UF_PATH].len);

	/* start HTTP(S) FOTA */
	if (slm_util_cmd_casecmp(schema, SCHEMA_HTTPS)) {
		if (sec_tag == INVALID_SEC_TAG) {
			LOG_ERR("Missing sec_tag");
			return -EINVAL;
		}
		ret = fota_download_start_with_image_type(hostname, path, sec_tag, pdn_id, 0, type);
	} else if (slm_util_cmd_casecmp(schema, SCHEMA_HTTP)) {
		ret = fota_download_start_with_image_type(hostname, path, -1, pdn_id, 0, type);
	} else {
		ret = -EINVAL;
	}
	/* Send an URC if failed to start */
	if (ret) {
		rsp_send("\r\n#XFOTA: %d,%d,%d\r\n", FOTA_STAGE_DOWNLOAD,
			FOTA_STATUS_ERROR, ret);
	}

	fota_type = type;

	return ret;
}

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		fota_stage = FOTA_STAGE_DOWNLOAD;
		fota_status = FOTA_STATUS_OK;
		fota_info = evt->progress;
		rsp_send("\r\n#XFOTA: %d,%d,%d\r\n", fota_stage, fota_status, fota_info);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		fota_stage = FOTA_STAGE_ACTIVATE;
		fota_info = 0;
		rsp_send("\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		/* Save, in case reboot by reset */
		slm_setting_fota_save();
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
		fota_stage = FOTA_STAGE_DOWNLOAD_ERASE_PENDING;
		rsp_send("\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		fota_stage = FOTA_STAGE_DOWNLOAD_ERASED;
		rsp_send("\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		fota_status = FOTA_STATUS_ERROR;
		fota_info = evt->cause;
		rsp_send("\r\n#XFOTA: %d,%d,%d\r\n", fota_stage, fota_status, fota_info);
		/* FOTA session terminated */
		slm_setting_fota_init();
		break;

	case FOTA_DOWNLOAD_EVT_CANCELLED:
		fota_status = FOTA_STATUS_CANCELLED;
		fota_info = 0;
		rsp_send("\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		/* FOTA session terminated */
		slm_setting_fota_init();
		break;

	default:
		return;
	}
}

/**@brief handle AT#XFOTA commands
 *  AT#XFOTA=<op>,<file_uri>[,<sec_tag>[,<pdn_id>]]
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
		if (op == SLM_FOTA_STOP) {
			err = fota_download_cancel();
#if defined(CONFIG_SECURE_BOOT)
		} else if (op == SLM_FOTA_START_APP || op == SLM_FOTA_START_MFW ||
			   op == SLM_FOTA_START_BL) {
#else
		} else if (op == SLM_FOTA_START_APP || op == SLM_FOTA_START_MFW) {
#endif
			char uri[FILE_URI_MAX];
			uint16_t pdn_id;
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
			if (op == SLM_FOTA_START_APP || op == SLM_FOTA_START_BL) {
				type = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
			} else {
				type = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
			}
			if (at_params_valid_count_get(&at_param_list) > 4) {
				at_params_unsigned_short_get(&at_param_list, 4, &pdn_id);
				err = do_fota_start(op, uri, sec_tag, pdn_id, type);
			} else {
				err = do_fota_start(op, uri, sec_tag, 0, type);
			}
#if defined(CONFIG_SECURE_BOOT)
			if (op == SLM_FOTA_START_BL) {
				bool s0_active;

				/* Override fota_type with SLM specified type */
				fota_type = SLM_DFU_TARGET_IMAGE_TYPE_BL1;
				(void)fota_download_s0_active_get(&s0_active);
				LOG_INF("orig s0_active %d", s0_active);
			}
#endif
#if FOTA_FUTURE_FEATURE
		} else if (op == SLM_FOTA_PAUSE_RESUME) {
			if (paused) {
				fota_download_resume();
				paused = false;
			} else {
				fota_download_pause();
				paused = true;
			}
			err = 0;
#endif
		} else if (op == SLM_FOTA_APP_READ) {
			do_fota_app_read(PM_MCUBOOT_PRIMARY_ID);
			do_fota_app_read(PM_MCUBOOT_SECONDARY_ID);
			err = 0;
		} else if (op == SLM_FOTA_MFW_READ) {
			err = do_fota_mfw_read();
		} else if (op == SLM_FOTA_ERASE_APP) {
			err = do_fota_erase_app();
		} else if (op == SLM_FOTA_ERASE_MFW) {
			err = do_fota_erase_mfw();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
#if defined(CONFIG_SECURE_BOOT)
		rsp_send("\r\n#XFOTA: (%d,%d,%d,%d,%d,%d,%d,%d),<file_uri>,<sec_tag>,<apn>\r\n",
			SLM_FOTA_STOP, SLM_FOTA_START_APP, SLM_FOTA_START_MFW, SLM_FOTA_START_BL,
			SLM_FOTA_APP_READ, SLM_FOTA_MFW_READ,
			SLM_FOTA_ERASE_APP, SLM_FOTA_ERASE_MFW);
#else
		rsp_send("\r\n#XFOTA: (%d,%d,%d,%d,%d,%d,%d),<file_uri>,<sec_tag>,<apn>\r\n",
			SLM_FOTA_STOP, SLM_FOTA_START_APP, SLM_FOTA_START_MFW,
			SLM_FOTA_APP_READ, SLM_FOTA_MFW_READ,
			SLM_FOTA_ERASE_APP, SLM_FOTA_ERASE_MFW);
#endif
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
			rsp_send("\r\n#XFOTA: %d,%d\r\n", fota_stage, fota_status);
		} else {
			rsp_send("\r\n#XFOTA: %d,%d,%d\r\n", fota_stage, fota_status,
				fota_info);
		}
	}
	/* FOTA session completed */
	slm_setting_fota_init();
	slm_setting_fota_save();
}
