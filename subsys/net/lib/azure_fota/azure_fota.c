/*
 * Copyright 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* The format of the JSON objects used for the Azure FOTA library is based on
 * the recommendations from Azure found that can be here:
 * https://docs.microsoft.com/en-us/azure/iot-hub/tutorial-firmware-update
 */

#include <stdlib.h>
#include <string.h>
#include <net/fota_download.h>
#include <net/azure_iot_hub.h>
#include <net/azure_fota.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include "azure_iot_hub_dps.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(azure_fota, CONFIG_AZURE_FOTA_LOG_LEVEL);

#define STATUS_CURRENT_STR "\"currentFwVersion\":\"%s\""
#define STATUS_PENDING_STR "\"pendingFwVersion\":\"%s\""
#define STATUS_UPDATE_STR "\"fwUpdateStatus\":\"%s\""
#define STATUS_UPDATE_SUB_STR "\"fwUpdateSubstatus\":\"%s\""

#define STATUS_REPORT					\
	"{"						\
		"\"firmware\":{"			\
			STATUS_UPDATE_STR ","		\
			STATUS_CURRENT_STR ","		\
			STATUS_PENDING_STR		\
		"}"					\
	"}"

/* Enum to keep the fota status */
enum fota_status {
	STATUS_CURRENT = 0,
	STATUS_DOWNLOADING,
	STATUS_APPLYING,
	STATUS_REBOOTING,
	STATUS_ERROR,
	STATUS_ROLLEDBACK,

	STATUS_COUNT,
};

struct fota_object {
	char host[CONFIG_AZURE_FOTA_HOSTNAME_MAX_LEN];
	char path[CONFIG_AZURE_FOTA_FILE_PATH_MAX_LEN];
	char version[CONFIG_AZURE_FOTA_VERSION_MAX_LEN];
	size_t fragment_size;
};

/* Static variables */
static azure_fota_callback_t callback;
static enum fota_status fota_state;
static struct fota_object fota_object;
static char *status_str[STATUS_COUNT] = {
	[STATUS_CURRENT]	= "current",
	[STATUS_DOWNLOADING]	= "downloading",
	[STATUS_APPLYING]	= "applying",
	[STATUS_REBOOTING]	= "rebooting",
	[STATUS_ERROR]		= "error",
	[STATUS_ROLLEDBACK]	= "rolledback",
};

#if IS_ENABLED(CONFIG_AZURE_FOTA_APP_VERSION_AUTO)
static char current_version[CONFIG_AZURE_FOTA_VERSION_MAX_LEN] =
	STRINGIFY(APP_VERSION);
#else
BUILD_ASSERT(sizeof(CONFIG_AZURE_FOTA_APP_VERSION) > 1, "No app version given");
static char current_version[CONFIG_AZURE_FOTA_VERSION_MAX_LEN] =
	CONFIG_AZURE_FOTA_APP_VERSION;
#endif

#if IS_ENABLED(AZURE_FOTA_TLS) && !defined(CONFIG_AZURE_FOTA_SEC_TAG)
BUILD_ASSERT(0, "CONFIG_AZURE_FOTA_SEC_TAG must be defined");
#endif

static char *create_report(void)
{
	size_t buf_size = sizeof(STATUS_REPORT) +
			2 * CONFIG_AZURE_FOTA_VERSION_MAX_LEN + 15;
	ssize_t len;
	char *buf = k_malloc(buf_size);

	if (buf == NULL) {
		LOG_WRN("Failed to allocate memory for FOTA report");
		return NULL;
	}

	len = snprintk(buf, buf_size, STATUS_REPORT, status_str[fota_state],
		       current_version, fota_object.version);
	if ((len == 0) || (len >= buf_size)) {
		LOG_WRN("Failed to create FOTA report");
		k_free(buf);

		return NULL;
	}

	LOG_DBG("Created FOTA report: %s", log_strdup(buf));

	return buf;
}

static void callback_with_report(struct azure_fota_event *evt)
{
	evt->report = create_report();
	if (evt->report == NULL) {
		LOG_WRN("FOTA report was not created");
	}

	callback(evt);

	if (evt->report) {
		k_free(evt->report);
	}
}

static char *get_current_version(void)
{
	return current_version;
}

static bool is_update(void)
{
	return strncmp(get_current_version(), fota_object.version,
		       strlen(get_current_version())) != 0;
}


static void fota_dl_handler(const struct fota_download_evt *fota_evt)
{
	struct azure_fota_event evt;

	switch (fota_evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_DBG("FOTA download completed evt received");

		fota_state = STATUS_APPLYING;
		evt.type = AZURE_FOTA_EVT_DONE;

		callback_with_report(&evt);
		break;

	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
		evt.type = AZURE_FOTA_EVT_ERASE_PENDING;
		callback(&evt);
		break;

	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		evt.type = AZURE_FOTA_EVT_ERASE_DONE;
		callback(&evt);
		break;

	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("FOTA download failed");
		fota_state = STATUS_ERROR;
		evt.type = AZURE_FOTA_EVT_ERROR;

		callback_with_report(&evt);
		break;

	case FOTA_DOWNLOAD_EVT_PROGRESS:
		/* Download progress events are not forwarded. */
		break;
	default:
		LOG_WRN("Unhandled FOTA event ID: %d", fota_evt->id);
		break;
	}
}

/* Checks the 'reported' object for current FOTA state
 * Returns true if the fota_state has been updated and should be reported to
 * the device twin. If no action is needed, false is returned.
 */
static bool parse_reported_status(const char *msg)
{
	struct cJSON *root_obj, *reported_obj, *fw_obj, *fw_status_obj,
		*fw_current_obj, *fw_pending_obj;
	bool report_needed = false;

	/* The 'reported' object has the excpected structure if it exists:
	 *	properties.reported.firmware : {
			"fwUpdateStatus": <status>,
			"currentFwVersion": <current firmware version>,
			"pendingFwVersion": <FOTA version in progress (if any)>
	 *	}
	 */

	root_obj = cJSON_Parse(msg);
	if (root_obj == NULL) {
		LOG_ERR("Could not parse message");
		return report_needed;
	}

	/* Reported object */
	reported_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "reported");
	if (reported_obj == NULL) {
		LOG_DBG("No 'reported' object found");
		goto clean_exit;
	}

	/* Firmware object */
	fw_obj = cJSON_GetObjectItemCaseSensitive(reported_obj, "firmware");
	if (fw_obj == NULL) {
		LOG_DBG("No 'firmware' object found");

		report_needed = true;

		goto clean_exit;
	}

	/* Firmware update status object (fwUpdateStatus) */
	fw_status_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							 "fwUpdateStatus");
	if (fw_status_obj == NULL) {
		LOG_DBG("No fwUpdateStatus object found");
		goto clean_exit;
	}

	LOG_DBG("Currently reported 'fwUpdateStatus' in device twin: %s",
		log_strdup(fw_status_obj->valuestring));

	/* Current firmware object (currentFwVersion) */
	fw_current_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							 "currentFwVersion");
	if (fw_current_obj == NULL) {
		LOG_DBG("No currentFwVersion object found");
		goto clean_exit;
	}

	LOG_DBG("Currently reported 'currentFwVersion' in device twin: %s",
		log_strdup(fw_current_obj->valuestring));

	/* Pending firmware object (pendingFwVersion) */
	fw_pending_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							 "pendingFwVersion");
	if (fw_pending_obj == NULL) {
		LOG_DBG("No pendingFwVersion object found");
		goto clean_exit;
	}

	LOG_DBG("Currently reported 'pendingFwVersion' in device twin: %s",
		strlen(fw_pending_obj->valuestring) > 0 ?
		       log_strdup(fw_pending_obj->valuestring) :
		       "(none)");

	/* "current" status indicates no FOTA was in progress, check if the
	 * reported version matches the actual version of running firmware.
	 */
	if (strcmp("current", fw_status_obj->valuestring) == 0) {
		if (strcmp(get_current_version(),
			   fw_current_obj->valuestring) == 0) {
			LOG_DBG("Currently reported version is correct");
			fota_state = STATUS_CURRENT;
			report_needed = false;
		} else {
			/* Reported current version should be updated. */
			LOG_DBG("Currently reported version must be updated");
			fota_state = STATUS_CURRENT;
			report_needed = true;
		}
	} else if (strcmp("applying", fw_status_obj->valuestring) == 0) {
		report_needed = true;

		/* "applying" indicates firmware was downloaded and would be
		 * used after reboot. Check if running version matches the
		 * pending version.
		 */
		if (strcmp(get_current_version(),
			   fw_pending_obj->valuestring) == 0) {
			LOG_DBG("FOTA firmware was successfully applied");
			fota_state = STATUS_CURRENT;
		} else if (strcmp(get_current_version(),
				  fw_current_obj->valuestring) == 0) {
			LOG_WRN("FOTA image was not applied, rolled back");
			fota_state = STATUS_ROLLEDBACK;
		} else {
			/* Current version matches neither of the reported
			 * current or pending versions, which indicate that
			 * the firmware has been changed by other means.
			 */
			fota_state = STATUS_CURRENT;
		}
	} else {
		/* In all other cases, the current firmware should be reported
		 * back, and the status is "current".
		 */
		fota_state = STATUS_CURRENT;
		report_needed = true;
	}

clean_exit:
	cJSON_Delete(root_obj);

	return report_needed;
}

/* @brief Extracts firmware image information from device twin JSON document.
 *
 * @retval 0 if successfully parsed, hostname and file_path populated.
 * @retval -ENOMSG Could not parse incoming buffer as JSON.
 * @retval -ENOMEM Could not find object, or insufficient memory to allocate it.
 */
static int extract_fw_details(const char *msg)
{
	int err = -ENOMEM;
	struct cJSON *root_obj, *desired_obj, *fw_obj, *fw_version_obj,
		*fw_location_obj, *fw_hostname_obj, *fw_path_obj,
		*fw_fragment_obj;

	/* The properties.desired.firmware object has the excpected structure:
	 *	properties.desired.firmware : {
	 *		fwVersion : <version>,
	 *		fwLocation : {
	 *			host : <hostname>,
	 *			path : <file path>
	 *		},
	 *		fwFragmentSize : <value>
	 *	}
	 */

	root_obj = cJSON_Parse(msg);
	if (root_obj == NULL) {
		LOG_ERR("Could not parse message as JSON");
		return -ENOMSG;
	}

	/* Desired object */
	desired_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "desired");
	if (desired_obj == NULL) {
		/* The received object can be the 'desired' object only, or
		 * the whole device twin properties object. If there is no
		 * 'desired' object to be found, assume it's the root object.
		 */
		LOG_DBG("No 'desired' object found, assuming it's root object");
		desired_obj = root_obj;
	}

	/* Firmware update information object */
	fw_obj = cJSON_GetObjectItemCaseSensitive(desired_obj, "firmware");
	if (fw_obj == NULL) {
		LOG_DBG("No 'firmware' object found");
		err = -ENOMSG;
		goto clean_exit;
	}

	/* Firmware version */
	fw_version_obj = cJSON_GetObjectItemCaseSensitive(fw_obj, "fwVersion");
	if (fw_version_obj == NULL) {
		LOG_DBG("No fwVersion object found");
		goto clean_exit;
	}

	if (cJSON_IsString(fw_version_obj)) {
		if (strlen(fw_version_obj->valuestring) >=
		    sizeof(fota_object.version)) {
			LOG_ERR("Version string longer than buffer");
			err = -ENOMEM;
			goto clean_exit;
		}

		memcpy(fota_object.version, fw_version_obj->valuestring,
		       sizeof(fota_object.version));

		LOG_DBG("Incoming firmware version: %s",
			log_strdup(fota_object.version));
	} else if (cJSON_IsNumber(fw_version_obj)) {
		ssize_t len = snprintk(fota_object.version,
				       sizeof(fota_object.version),
				       "%d", fw_version_obj->valueint);

		if ((len < 0) || (len >= sizeof(fota_object.version))) {
			LOG_ERR("Version number too long for buffer");
			err = -ENOMEM;
		}
	} else {
		LOG_DBG("Invalid fwVersion format received");
		goto clean_exit;
	}

	/* Firmware image location */
	fw_location_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							   "fwLocation");
	if (fw_location_obj == NULL) {
		LOG_DBG("No fwLocation object found");
		goto clean_exit;
	}

	/* Firmware image hostname */
	fw_hostname_obj = cJSON_GetObjectItemCaseSensitive(fw_location_obj,
							   "host");
	if (fw_hostname_obj == NULL) {
		LOG_DBG("No hostname object found");
		goto clean_exit;
	}

	if (strlen(fw_hostname_obj->valuestring) >=
	    sizeof(fota_object.host)) {
		LOG_ERR("Received hostname larger (%d) than buffer (%d)",
			strlen(fw_hostname_obj->valuestring),
			sizeof(fota_object.host));
		goto clean_exit;
	}

	strncpy(fota_object.host, fw_hostname_obj->valuestring,
		sizeof(fota_object.host));

	/* Firmware image file path */
	fw_path_obj = cJSON_GetObjectItemCaseSensitive(fw_location_obj, "path");
	if (fw_path_obj == NULL) {
		LOG_DBG("No path object found");
		goto clean_exit;
	}

	if (strlen(fw_path_obj->valuestring) >= sizeof(fota_object.path)) {
		LOG_ERR("Received file path larger (%d) than buffer (%d)",
			strlen(fw_path_obj->valuestring),
			sizeof(fota_object.path));
		goto clean_exit;
	}

	strncpy(fota_object.path, fw_path_obj->valuestring,
		sizeof(fota_object.path));

	/* Firmware fragment size */
	fw_fragment_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							"fwFragmentSize");
	if (fw_fragment_obj == NULL) {
		LOG_DBG("Failed to parse 'fwFragmentSize' object");
		goto clean_exit;
	}

	fota_object.fragment_size = fw_fragment_obj->valueint;

	err = 0;

clean_exit:
	cJSON_Delete(root_obj);

	return err;
}

/* Public functions */
int azure_fota_init(azure_fota_callback_t evt_handler)
{
	int err;

	if (evt_handler == NULL) {
		return -EINVAL;
	}

	callback = evt_handler;

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		LOG_ERR("fota_download_init error %d", err);
		return err;
	}

	cJSON_Init();

	fota_state = STATUS_CURRENT;

	LOG_INF("Current firmware version: %s",
		log_strdup(get_current_version()));

	return 0;
}

int azure_fota_msg_process(const char *const buf, size_t size)
{
	int err;
	struct azure_fota_event evt = {
		.type = AZURE_FOTA_EVT_START
	};

	/* Check last reported FOTA status */
	if (parse_reported_status(buf)) {
		evt.type = AZURE_FOTA_EVT_REPORT;

		callback_with_report(&evt);

		evt.type = AZURE_FOTA_EVT_START;
	}

	err = extract_fw_details(buf);
	if (err == -ENOMSG) {
		LOG_DBG("No 'firmware' object found, ignoring message");
		return 0;
	} else if (err < 0) {
		LOG_DBG("Firmware details not found, FOTA will not start");
		return 0;
	}

	if (fota_state == STATUS_DOWNLOADING) {
		LOG_INF("Firmware download is already ongoing");
		return 0;
	}

	/* Check version to see if the firmware image should be downloaded */
	if (!is_update()) {
		LOG_DBG("Image is not an update, skipping download");
		LOG_DBG("Current version: %s",
			log_strdup(get_current_version()));
		LOG_DBG("FOTA version: %s", log_strdup(fota_object.version));

		fota_state = STATUS_CURRENT;

		return 0;
	}

	fota_state = STATUS_DOWNLOADING;

	LOG_INF("Attempting to download firmware (version '%s') from %s/%s",
		log_strdup(fota_object.version),
		log_strdup(fota_object.host),
		log_strdup(fota_object.path));

	err = fota_download_start(fota_object.host, fota_object.path,
				  CONFIG_AZURE_FOTA_SEC_TAG,
				  NULL, fota_object.fragment_size);
	if (err) {
		LOG_ERR("Error (%d) when trying to start firmware download",
			err);

		fota_state = STATUS_ERROR;
		evt.type = AZURE_FOTA_EVT_ERROR;

		callback_with_report(&evt);
		memset(&fota_object, 0, sizeof(fota_object));

		return err;
	}

	/* Callback with AZURE_FOTA_EVT_START */
	evt.version = fota_object.version;

	callback_with_report(&evt);

	return 1;
}
