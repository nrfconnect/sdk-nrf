/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Ensure 'strnlen' is available even with -std=c99. */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <nrfx.h>

#include "fota_download_util.h"

#if defined(CONFIG_DFU_TARGET_MODEM_DELTA)
#include "fota_download_delta_modem.h"
#endif

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
#include "fota_download_full_modem.h"
#endif

#if defined(CONFIG_DFU_TARGET_MCUBOOT)
#include "fota_download_mcuboot.h"
#endif

#if defined(CONFIG_DFU_TARGET_SMP)
#include "fota_download_smp.h"
#endif

LOG_MODULE_REGISTER(fota_download_util, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

/**
 * @brief FOTA download url data.
 */
struct fota_download_client_url_data {
	/** Host name */
	const char *host;
	/** Host name length */
	size_t host_len;
	/** File name */
	const char *file;
	/** File name length */
	size_t file_len;
	/** HTTPs or CoAPs */
	bool sec_tag_needed;
};

static void start_fota_download(struct k_work *work);
static K_WORK_DEFINE(download_work, start_fota_download);
static fota_download_callback_t fota_client_callback;
static char fota_path[CONFIG_FOTA_DOWNLOAD_FILE_NAME_LENGTH];
static char fota_host[CONFIG_FOTA_DOWNLOAD_HOST_NAME_LENGTH];
static int fota_sec_tag = -1;
static bool download_active;
static enum dfu_target_image_type active_dfu_type;

int fota_download_parse_dual_resource_locator(char *const file, bool s0_active,
					     const char **selected_path)
{
	if (file == NULL || selected_path == NULL) {
		LOG_ERR("Got NULL pointer");
		return -EINVAL;
	}

	if (!nrfx_is_in_ram(file)) {
		LOG_ERR("'file' pointer is not located in RAM");
		return -EFAULT;
	}

	/* Ensure that 'file' is null-terminated. */
	if (strnlen(file, CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH) ==
	    CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH) {
		LOG_ERR("Input is not null terminated");
		return -ENOTSUP;
	}

	/* We have verified that there is a null-terminator, so this is safe */
	char *delimiter = strstr(file, " ");

	if (delimiter == NULL) {
		/* Could not find delimiter in input */
		*selected_path = NULL;
		return 0;
	}

	/* Insert null-terminator to split the dual filepath into two separate filepaths */
	*delimiter = '\0';
	const char *s0_path = file;
	const char *s1_path = delimiter + 1;

	*selected_path = s0_active ? s1_path : s0_path;
	return 0;
}

static void fota_download_callback(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	/* These two cases return immediately */
	case FOTA_DOWNLOAD_EVT_PROGRESS:
	default:
		break;

	/* Following cases mark end of FOTA download */
	case FOTA_DOWNLOAD_EVT_CANCELLED:
	case FOTA_DOWNLOAD_EVT_ERROR:
	case FOTA_DOWNLOAD_EVT_FINISHED:
		download_active = false;
		break;
	}
	if (fota_client_callback) {
		fota_client_callback(evt);
	}
}

static int fota_download_client_url_parse(const char *uri,
					  struct fota_download_client_url_data *parsed_uri)
{
	int len;
	char *e, *s;

	len = strlen(uri);

	/* Find the end of protocol marker https:// or coap:// */
	s = strstr(uri, "://");

	if (!s) {
		LOG_ERR("Host not found");
		return -EINVAL;
	}
	s += strlen("://");

	/* Find the end of host name, which is start of path */
	e = strchr(s, '/');

	if (!e) {
		LOG_ERR("Path not found");
		return -EINVAL;
	}

	parsed_uri->host = uri;
	parsed_uri->host_len = e - uri;
	parsed_uri->file = e + 1;
	parsed_uri->file_len = strlen(uri) - parsed_uri->host_len;
	parsed_uri->sec_tag_needed =
		strncmp(uri, "https://", 8) == 0 || strncmp(uri, "coaps://", 8) == 0;

	return 0;
}

static int download_url_parse(const char *uri, int sec_tag)
{
	int ret;
	struct fota_download_client_url_data parsed_uri;

	LOG_INF("Download url %s", uri);

	ret = fota_download_client_url_parse(uri, &parsed_uri);
	if (ret) {
		return ret;
	}

	if (parsed_uri.host_len >= sizeof(fota_host)) {
		LOG_ERR("Host name too big %d", parsed_uri.host_len);
		return -ENOMEM;
	} else if (parsed_uri.file_len >= sizeof(fota_path)) {
		LOG_ERR("File name too big %d", parsed_uri.host_len);
		return -ENOMEM;
	}

	if (parsed_uri.sec_tag_needed) {
		if (sec_tag == -1) {
			LOG_ERR("FOTA SMP sec tag not configured");
			return -EINVAL;
		}
		fota_sec_tag = sec_tag;
	} else {
		fota_sec_tag = -1;
	}

	strncpy(fota_host, parsed_uri.host, parsed_uri.host_len);
	fota_host[parsed_uri.host_len] = 0;

	strncpy(fota_path, parsed_uri.file, parsed_uri.file_len);
	fota_path[parsed_uri.file_len] = 0;

	return 0;
}

static void start_fota_download(struct k_work *work)
{
	int ret;

	ret = fota_download_start_with_image_type(fota_host, fota_path, fota_sec_tag, 0, 0,
						  active_dfu_type);
	if (ret) {
		struct fota_download_evt evt;

		LOG_ERR("fota_download_start() failed, return code %d", ret);
		evt.id = FOTA_DOWNLOAD_EVT_CANCELLED;
		fota_download_callback(&evt);
	}
}

int fota_download_util_stream_init(void)
{
	int ret = 0;

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	ret = fota_download_full_modem_stream_params_init();
	if (ret) {
		LOG_ERR("Full modem stream init fail %d", ret);
		return ret;
	}
#endif

#if defined(CONFIG_DFU_TARGET_MCUBOOT)
	ret = fota_download_mcuboot_target_init();
	if (ret) {
		LOG_ERR("MCUBoot stream init fail %d", ret);
		return ret;
	}
#endif
	return ret;
}

int fota_download_util_download_start(const char *download_uri,
				     enum dfu_target_image_type dfu_target_type, int sec_tag,
				     fota_download_callback_t client_callback)
{
	int ret;

	if (download_active) {
		return -EBUSY;
	}

	if (download_url_parse(download_uri, sec_tag)) {
		return -EINVAL;
	}

	LOG_INF("Download Path %s host %s", fota_path, fota_host);

	active_dfu_type = dfu_target_type;

	/* Register Callback */
	ret = fota_download_util_client_init(
		fota_download_callback,
		(active_dfu_type == DFU_TARGET_IMAGE_TYPE_SMP ? true : false));

	if (ret != 0) {
		LOG_ERR("fota_download_init() returned %d", ret);
		return -EBUSY;
	}

	fota_client_callback = client_callback;
	download_active = true;

	/* Trigger download start */
	k_work_submit(&download_work);

	return 0;
}

int fota_download_util_download_cancel(void)
{
	if (!download_active) {
		return -EACCES;
	}
	download_active = false;

	return fota_download_cancel();
}

static void dfu_target_cb(enum dfu_target_evt_id evt)
{
	ARG_UNUSED(evt);
}

int fota_download_util_dfu_target_init(enum dfu_target_image_type dfu_target_type)
{
	int ret = 0;

	switch (dfu_target_type) {
#if defined(CONFIG_DFU_TARGET_MCUBOOT)
	case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
		ret = fota_download_mcuboot_target_init();
		break;
#endif

#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
		ret = fota_download_full_modem_pre_init();
		break;
#endif

#if defined(CONFIG_DFU_TARGET_MODEM_DELTA)
	case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		break;
#endif
	case DFU_TARGET_IMAGE_TYPE_SMP:
		break;
	default:
		LOG_ERR("Unsupported Image type %d", dfu_target_type);
		ret = -EACCES;
		break;
	}
	return ret;

}

static int fota_dfu_target_pre_init(enum dfu_target_image_type dfu_target_type)
{
	int ret = 0;

	ret = fota_download_util_dfu_target_init(dfu_target_type);
	if (ret) {
		return ret;
	}
	/* Init DFU target ready for usage */
	return dfu_target_init(dfu_target_type, 0, 0, dfu_target_cb);
}

static int firmware_update_slot_num_get(enum dfu_target_image_type dfu_image_type)
{
	int slot_num;

	switch (dfu_image_type) {
	case DFU_TARGET_IMAGE_TYPE_SMP:
		slot_num = 1;
		break;
	default:
		slot_num = 0;
		break;
	}

	return slot_num;
}

int fota_download_util_image_schedule(enum dfu_target_image_type dfu_target_type)
{
	int ret, slot_num;

	if (IS_ENABLED(CONFIG_FOTA_CLIENT_AUTOSCHEDULE_UPDATE)) {
		/* target is already sheduled */
		return 0;
	}

	ret = fota_dfu_target_pre_init(dfu_target_type);
	if (ret) {
		return ret;
	}
	slot_num = firmware_update_slot_num_get(dfu_target_type);

	return dfu_target_schedule_update(slot_num);
}

int fota_download_util_image_reset(enum dfu_target_image_type dfu_target_type)
{
	int ret;

	ret = fota_dfu_target_pre_init(dfu_target_type);
	if (ret) {
		return ret;
	}
	return dfu_target_reset();
}

int fota_download_util_apply_update(enum dfu_target_image_type dfu_target_type)
{
	int ret;

	switch (dfu_target_type) {
#if defined(CONFIG_DFU_TARGET_MCUBOOT)
	case DFU_TARGET_IMAGE_TYPE_MCUBOOT:
		ret = 0;
		break;
#endif
#if defined(CONFIG_DFU_TARGET_FULL_MODEM)
	case DFU_TARGET_IMAGE_TYPE_FULL_MODEM:
		ret = fota_download_full_modem_apply_update();
		break;
#endif
#if defined(CONFIG_DFU_TARGET_MODEM_DELTA)
	case DFU_TARGET_IMAGE_TYPE_MODEM_DELTA:
		ret = fota_download_apply_delta_modem_update();
		break;
#endif

#if defined(CONFIG_DFU_TARGET_SMP)
	case DFU_TARGET_IMAGE_TYPE_SMP:
		ret = fota_download_smp_update_apply();
		break;
#endif
	default:
		ret = -1;
		break;
	}

	return ret;
}
