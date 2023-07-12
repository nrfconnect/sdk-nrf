/*
 * Copyright 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(azure_fota, CONFIG_AZURE_FOTA_LOG_LEVEL);

#define STATUS_CURRENT_STR	"\"currentFwVersion\":\"%s\""
#define STATUS_PENDING_STR	"\"pendingFwVersion\":\"%s\""
#define STATUS_UPDATE_STR	"\"fwUpdateStatus\":\"%s\""
#define STATUS_UPDATE_SUB_STR	"\"fwUpdateSubstatus\":\"%s\""
#define STATUS_JOB_ID		"\"jobId\":\"%s\""

#define STATUS_REPORT					\
	"{"						\
		"\"firmware\":{"			\
			STATUS_UPDATE_STR ","		\
			STATUS_CURRENT_STR		\
		"}"					\
	"}"

#define STATUS_REPORT_UPDATING					\
	"{"						\
		"\"firmware\":{"			\
			STATUS_UPDATE_STR ","		\
			STATUS_CURRENT_STR ","		\
			STATUS_PENDING_STR ","		\
			STATUS_JOB_ID			\
		"}"					\
	"}"

/* Static variables */

/* Enum to keep track of the internal state. */
static enum internal_state {
	/* Library is uninitialized, no messages can be processed. */
	STATE_UNINIT,
	/* Initialized and waiting for incoming messages to process. */
	STATE_INIT,
	/* Downloading FOTA image. */
	STATE_DOWNLOADING,
} internal_state = STATE_UNINIT;

/* Enum to keep track of the reported fota status */
static enum fota_status {
	REP_STATUS_CURRENT,
	REP_STATUS_DOWNLOADING,
	REP_STATUS_APPLYING,
	REP_STATUS_REBOOTING,
	REP_STATUS_ERROR,
	REP_STATUS_ROLLEDBACK,

	REP_STATUS_INVALID,

	REP_STATUS_COUNT,
} current_status = REP_STATUS_CURRENT;

static struct fota_object {
	char host[CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE];
	char path[CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE];
	char version[CONFIG_AZURE_FOTA_VERSION_MAX_LEN];
	char job_id[CONFIG_AZURE_FOTA_JOB_ID_MAX_LEN];
	size_t fragment_size;
} fota_object;

static azure_fota_callback_t callback;
static char *internal_state_str[] = {
	[STATE_UNINIT]		= "STATE_UNINIT",
	[STATE_INIT]		= "STATE_INIT",
	[STATE_DOWNLOADING]	= "STATE_DOWNLOADING",
};
static char *rep_status_str[REP_STATUS_COUNT] = {
	[REP_STATUS_CURRENT]		= "current",
	[REP_STATUS_DOWNLOADING]	= "downloading",
	[REP_STATUS_APPLYING]		= "applying",
	[REP_STATUS_REBOOTING]		= "rebooting",
	[REP_STATUS_ERROR]		= "error",
	[REP_STATUS_ROLLEDBACK]		= "rolledback",
};

static char reported_job_id[CONFIG_AZURE_FOTA_JOB_ID_MAX_LEN];

#if IS_ENABLED(CONFIG_AZURE_FOTA_APP_VERSION_AUTO)
static char current_version[CONFIG_AZURE_FOTA_VERSION_MAX_LEN] =
	STRINGIFY(AZURE_FOTA_APP_VERSION);
#else
BUILD_ASSERT(sizeof(CONFIG_AZURE_FOTA_APP_VERSION) > 1, "No app version given");
static char current_version[CONFIG_AZURE_FOTA_VERSION_MAX_LEN] =
	CONFIG_AZURE_FOTA_APP_VERSION;
#endif

#if IS_ENABLED(AZURE_FOTA_TLS) && !defined(CONFIG_AZURE_FOTA_SEC_TAG)
BUILD_ASSERT(0, "CONFIG_AZURE_FOTA_SEC_TAG must be defined");
#endif

static void rep_status_set(enum fota_status new_status)
{
	current_status = new_status;
}

static void state_set(enum internal_state new_state)
{
	if (internal_state == new_state) {
		return;
	}

	LOG_DBG("State transition: %s --> %s",
		internal_state_str[internal_state],
		internal_state_str[new_state]);

	internal_state = new_state;
}

static enum internal_state state_get(void)
{
	return internal_state;
}

static char *reported_status_str_get(void)
{
	return rep_status_str[current_status];
}

static enum fota_status reported_status_get(const char *status)
{
	for (size_t i = 0; i < ARRAY_SIZE(rep_status_str); i++) {
		if (strcmp(rep_status_str[i], status) == 0) {
			return i;
		}
	}

	return REP_STATUS_INVALID;
}

/* Create update report. Buffer must be freed by the user. */
static char *create_report(void)
{
	size_t buf_size = sizeof(STATUS_REPORT_UPDATING) +
			2 * CONFIG_AZURE_FOTA_VERSION_MAX_LEN +
			CONFIG_AZURE_FOTA_JOB_ID_MAX_LEN + 15;
	ssize_t len;
	char *buf = k_malloc(buf_size);

	if (buf == NULL) {
		LOG_WRN("Failed to allocate memory for FOTA report");
		return NULL;
	}

	if (strlen(fota_object.job_id) > 0) {
		len = snprintk(buf, buf_size, STATUS_REPORT_UPDATING,
			       rep_status_str[current_status],
			       current_version, fota_object.version,
			       fota_object.job_id);
	} else {
		len = snprintk(buf, buf_size, STATUS_REPORT,
			       rep_status_str[current_status],
			       current_version);
	}

	if ((len <= 0) || (len >= buf_size)) {
		LOG_WRN("Failed to create FOTA report");
		k_free(buf);

		return NULL;
	}

	LOG_DBG("Created FOTA report: %s", buf);

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
		       MAX(strlen(get_current_version()),
			   strlen(fota_object.version))) != 0;
}

static bool is_new_job(void)
{
	/* If no jobId is present, we consider the desired job as a new job.
	 * This is to support solutions built with previous version of Azure
	 * FOTA that don't have the concept of job IDs.
	 */
	if (strlen(reported_job_id) == 0) {
		return true;
	}

	return strncmp(reported_job_id, fota_object.job_id,
		       strlen(reported_job_id)) != 0;
}

static void fota_dl_handler(const struct fota_download_evt *fota_evt)
{
	struct azure_fota_event evt;

	switch (fota_evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_DBG("FOTA download completed evt received");

		state_set(STATE_INIT);
		rep_status_set(REP_STATUS_APPLYING);
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
		state_set(STATE_INIT);
		rep_status_set(REP_STATUS_ERROR);
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

/* Process the reported status and determine if a new status must be reported
 * back.
 *
 * Returns true if report is needed, otherwise false.
 */
static bool reported_status_process(const char *status,
				    const char *current_fw,
				    const char *pending_fw)
{
	bool report_needed = false;

	switch (reported_status_get(status)) {
	case REP_STATUS_CURRENT:
		/* "current" status indicates no FOTA was in progress, check if the
		 * reported version matches the actual version of running firmware.
		 */
		if (strcmp(get_current_version(), current_fw) == 0) {
			LOG_DBG("Currently reported version is correct");
			rep_status_set(REP_STATUS_CURRENT);
			report_needed = false;
		} else {
			/* Reported current version should be updated. */
			LOG_DBG("Currently reported version must be updated");
			rep_status_set(REP_STATUS_CURRENT);
			report_needed = true;
		}

		break;
	case REP_STATUS_APPLYING:
		/* "applying" indicates firmware was downloaded and would be
		 * used after reboot. Check if running version matches the
		 * pending version.
		 */
		report_needed = true;

		if (strcmp(get_current_version(), pending_fw) == 0) {
			LOG_DBG("FOTA firmware was successfully applied");
			rep_status_set(REP_STATUS_CURRENT);
		} else if (strcmp(get_current_version(), current_fw) == 0) {
			LOG_WRN("FOTA image was not applied, rolled back");
			rep_status_set(REP_STATUS_ROLLEDBACK);
		} else {
			/* Current version matches neither of the reported
			 * current or pending versions, which indicate that
			 * the firmware has been changed by other means.
			 */
			rep_status_set(REP_STATUS_CURRENT);
		}

		break;
	case REP_STATUS_DOWNLOADING:
		if (state_get() == STATE_DOWNLOADING) {
			LOG_DBG("Status \"downloading\" was reported, no update needed");
			/* Download is in progress, no update needed */
			break;
		}

		/* A download was in progress and must have failed for this
		 * situation to occur. Therefore, the error must be reported.
		 */
		report_needed = true;

		rep_status_set(REP_STATUS_ERROR);

		break;
	case REP_STATUS_ERROR:
		LOG_DBG("Status \"error\" was reported, no update will be sent");
		rep_status_set(REP_STATUS_ERROR);
		break;
	default:
		/* In all other cases, the current firmware should be reported
		 * back, and the status is "current".
		 */
		report_needed = true;

		rep_status_set(REP_STATUS_CURRENT);

		break;
	}

	return report_needed;
}

/* Checks the 'reported' object for current FOTA state
 * Returns true if the current_status has been updated and should be reported to
 * the device twin. If no action is needed, false is returned.
 */
static bool parse_reported_status(const char *msg)
{
	struct cJSON *root_obj, *reported_obj, *fw_obj, *fw_status_obj,
		*fw_current_obj, *fw_pending_obj, *job_id_obj;
	bool report_needed = false;

	/* The 'reported' object has the expected structure if it exists:
	 *	properties.reported.firmware : {
	 *		"fwUpdateStatus": <status>,
	 *		"currentFwVersion": <current firmware version>,
	 *		"pendingFwVersion": <FOTA version in progress (if any)>,
	 *		"jobId": <FOTA update ID, formatted according to
	 *			 RFC4122. Expected to be 36 characters, of
	 *			 which 32 are hexadecimal characters, and 4
	 *			 dashes.>
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

	/* Firmware update job ID. If jobId is not present, it's not considered
	 * a failure, as we want to support previous versions of Azure FOTA
	 * that don't use the jobId.
	 */
	job_id_obj = cJSON_GetObjectItemCaseSensitive(fw_obj, "jobId");
	if (job_id_obj == NULL) {
		LOG_WRN("No job ID found");
	} else {
		if (strlen(job_id_obj->valuestring) >=
		    sizeof(reported_job_id)) {
			LOG_ERR("Received job ID larger (%d) than buffer (%d)",
				strlen(job_id_obj->valuestring),
				sizeof(reported_job_id));
			goto clean_exit;
		}

		strncpy(reported_job_id, job_id_obj->valuestring,
			sizeof(reported_job_id));
	}

	/* Firmware update status object (fwUpdateStatus) */
	fw_status_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							 "fwUpdateStatus");
	if (fw_status_obj == NULL) {
		LOG_DBG("No fwUpdateStatus object found");
		goto clean_exit;
	}

	LOG_DBG("Currently reported 'fwUpdateStatus' in device twin: %s",
		fw_status_obj->valuestring);

	/* Current firmware object (currentFwVersion) */
	fw_current_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							 "currentFwVersion");
	if (fw_current_obj == NULL) {
		LOG_DBG("No currentFwVersion object found");
		goto clean_exit;
	}

	LOG_DBG("Currently reported 'currentFwVersion' in device twin: %s",
		fw_current_obj->valuestring);

	/* Pending firmware object (pendingFwVersion) */
	fw_pending_obj = cJSON_GetObjectItemCaseSensitive(fw_obj,
							 "pendingFwVersion");
	if (fw_pending_obj == NULL) {
		LOG_DBG("No pendingFwVersion object found");
		report_needed = true;
		goto clean_exit;
	}

	LOG_DBG("Currently reported 'pendingFwVersion' in device twin: %s",
		strlen(fw_pending_obj->valuestring) > 0 ?
		       fw_pending_obj->valuestring :
		       "(none)");

	report_needed = reported_status_process(fw_status_obj->valuestring,
						fw_current_obj->valuestring,
						fw_pending_obj->valuestring);
	if (report_needed) {
		LOG_DBG("Reported status was \"%s\", reporting back \"%s\"",
			fw_status_obj->valuestring,
			reported_status_str_get());
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
		*fw_fragment_obj, *job_id_obj;

	/* The properties.desired.firmware object has the expected structure:
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

		LOG_DBG("Incoming firmware version: %s", fota_object.version);
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

	/* Firmware update job ID */
	job_id_obj = cJSON_GetObjectItemCaseSensitive(fw_obj, "jobId");
	if (job_id_obj == NULL) {
		LOG_WRN("No job ID found");
	} else {
		if (strlen(job_id_obj->valuestring) >=
		    sizeof(fota_object.job_id)) {
			LOG_ERR("Received job ID larger (%d) than buffer (%d)",
				strlen(job_id_obj->valuestring),
				sizeof(fota_object.job_id));
			goto clean_exit;
		}

		strncpy(fota_object.job_id, job_id_obj->valuestring,
			sizeof(fota_object.job_id));

		LOG_DBG("Job ID: %s", job_id_obj->valuestring);
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

	state_set(STATE_INIT);

	LOG_INF("Current firmware version: %s", get_current_version());

	return 0;
}

int azure_fota_msg_process(const char *const buf, size_t size)
{
	int err;
	struct azure_fota_event evt = {
		.type = AZURE_FOTA_EVT_START
	};

	if (state_get() == STATE_UNINIT) {
		LOG_ERR("Azure FOTA is not initialized");
		return -EINVAL;
	}

	/* Ensure that FOTA download is not started multiple times, which
	 * would corrupt the downloaded image.
	 */
	if (state_get() == STATE_DOWNLOADING) {
		LOG_INF("Firmware download is ongoing, message ignored");
		return 0;
	}

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

	if (!is_new_job()) {
		LOG_WRN("Update job (ID: %s) was already attempted, aborting",
			reported_job_id);
		rep_status_set(REP_STATUS_CURRENT);

		return 0;
	}

	/* Check version to see if the firmware image should be downloaded */
	if (!is_update()) {
		LOG_DBG("Image is not an update, skipping download");
		LOG_DBG("Current version: %s",
			get_current_version());
		LOG_DBG("FOTA version: %s", fota_object.version);

		rep_status_set(REP_STATUS_CURRENT);

		return 0;
	}

	state_set(STATE_DOWNLOADING);
	rep_status_set(REP_STATUS_DOWNLOADING);

	LOG_INF("Attempting to download firmware (version '%s') from %s/%s",
		fota_object.version,
		fota_object.host,
		fota_object.path);

	err = fota_download_start(fota_object.host, fota_object.path,
				  CONFIG_AZURE_FOTA_SEC_TAG,
				  0, fota_object.fragment_size);
	if (err) {
		LOG_ERR("Error (%d) when trying to start firmware download",
			err);

		state_set(STATE_INIT);
		rep_status_set(REP_STATUS_ERROR);
		evt.type = AZURE_FOTA_EVT_ERROR;

		callback_with_report(&evt);
		memset(&fota_object, 0, sizeof(fota_object));

		return err;
	}

	if (strlen(fota_object.job_id) > 0) {
		strcpy(reported_job_id, fota_object.job_id);
	}

	/* Callback with AZURE_FOTA_EVT_START */
	evt.version = fota_object.version;

	callback_with_report(&evt);

	return 1;
}
