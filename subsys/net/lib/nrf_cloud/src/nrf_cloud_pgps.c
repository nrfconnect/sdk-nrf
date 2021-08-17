/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <nrfx_nvmc.h>
#include <device.h>
#include <drivers/gps.h>
#include <storage/stream_flash.h>
#include <net/socket.h>
#include <nrf_socket.h>

#include <cJSON.h>
#include <cJSON_os.h>
#include <modem/modem_info.h>
#include <date_time.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <settings/settings.h>
#include <power/reboot.h>
#include <logging/log_ctrl.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_pgps, CONFIG_NRF_CLOUD_GPS_LOG_LEVEL);

#include "nrf_cloud_transport.h"
#include "nrf_cloud_pgps_schema_v1.h"
#include "nrf_cloud_pgps_utils.h"

#define FORCE_HTTP_DL			0 /* set to 1 to force HTTP instead of HTTPS */
#define PGPS_DEBUG			0 /* set to 1 for extra logging */

#define PREDICTION_PERIOD		CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD
#define REPLACEMENT_THRESHOLD		CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD
#define SEC_TAG				CONFIG_NRF_CLOUD_SEC_TAG
#define FRAGMENT_SIZE			CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_FRAGMENT_SIZE
#define PREDICTION_MIDPOINT_SHIFT_SEC	(120 * SEC_PER_MIN)
#define LOCATION_UNC_SEMIMAJOR_K	89U
#define LOCATION_UNC_SEMIMINOR_K	89U
#define LOCATION_CONFIDENCE_PERCENT	68U

#define PGPS_JSON_APPID_KEY		"appId"
#define PGPS_JSON_APPID_VAL_PGPS	"PGPS"
#define PGPS_JSON_MSG_TYPE_KEY		"messageType"
#define PGPS_JSON_MSG_TYPE_VAL_DATA	"DATA"
#define PGPS_JSON_DATA_KEY		"data"

#define PGPS_JSON_PRED_COUNT		"predictionCount"
#define PGPS_JSON_PRED_INT_MIN		"predictionIntervalMinutes"
#define PGPS_JSON_GPS_DAY		"startGpsDay"
#define PGPS_JSON_GPS_TIME		"startGpsTimeOfDaySeconds"

#if defined(CONFIG_PGPS_INCLUDE_MODEM_INFO)
#define PGPS_JSON_MCC_KEY		"mcc"
#define PGPS_JSON_MNC_KEY		"mnc"
#define PGPS_JSON_AREA_CODE_KEY		"tac"
#define PGPS_JSON_CELL_ID_KEY		"eci"
#define PGPS_JSON_PHYCID_KEY		"phycid"
#define PGPS_JSON_TYPES_KEY		"types"
#define PGPS_JSON_CELL_LOC_KEY_DOREPLY	"doReply"
#define PGPS_JSON_APPID_VAL_SINGLE_CELL	"SCELL"
#define PGPS_JSON_APPID_VAL_MULTI_CELL	"MCELL"
#define PGPS_JSON_CELL_LOC_KEY_LAT	"lat"
#define PGPS_JSON_CELL_LOC_KEY_LON	"lon"
#define PGPS_JSON_CELL_LOC_KEY_UNCERT	"uncertainty"
#endif

#define RCV_ITEM_IDX_FILE_HOST 0
#define RCV_ITEM_IDX_FILE_PATH 1

enum pgps_state {
	PGPS_NONE,
	PGPS_INITIALIZING,
	PGPS_EXPIRED,
	PGPS_REQUESTING,
	PGPS_LOADING,
	PGPS_READY,
};
static enum pgps_state state;

struct pgps_index {
	struct nrf_cloud_pgps_header header;
	int64_t start_sec;
	int64_t end_sec;
	uint32_t dl_offset;
	uint16_t pred_offset;
	uint16_t expected_count;
	uint16_t loading_count;
	uint16_t period_sec;
	uint8_t dl_pnum;
	uint8_t pnum_offset;
	uint8_t cur_pnum;
	bool partial_request;
	bool stale_server_data;
	uint32_t storage_extent;
	int store_block;

	/* array of pointers to predictions, in sorted time order */
	struct nrf_cloud_pgps_prediction *predictions[NUM_PREDICTIONS];
};

static struct pgps_index index;

static pgps_event_handler_t handler;
/* array of potentially out-of-time-order predictions */
static uint8_t *storage;
static uint32_t storage_size;
static struct stream_flash_ctx stream;
static uint8_t *write_buf;
static uint32_t flash_page_size;

static uint8_t prediction_buf[PGPS_PREDICTION_STORAGE_SIZE];

static bool json_initialized;
static bool ignore_packets;
static atomic_t pgps_need_assistance;

static int validate_stored_predictions(uint16_t *bad_day, uint32_t *bad_time);
static void log_pgps_header(const char *msg, const struct nrf_cloud_pgps_header *header);
static int consume_pgps_header(const char *buf, size_t buf_len);
static void cache_pgps_header(const struct nrf_cloud_pgps_header *header);
static int consume_pgps_data(uint8_t pnum, const char *buf, size_t buf_len);
static void prediction_work_handler(struct k_work *work);
static void prediction_timer_handler(struct k_timer *dummy);
void agps_print_enable(bool enable);
static void print_time_details(const char *info,
			       int64_t sec, uint16_t day, uint32_t time_of_day);

K_WORK_DEFINE(prediction_work, prediction_work_handler);
K_TIMER_DEFINE(prediction_timer, prediction_timer_handler, NULL);

static int determine_prediction_num(struct nrf_cloud_pgps_header *header,
				    struct nrf_cloud_pgps_prediction *p)
{
	int64_t start_sec = npgps_gps_day_time_to_sec(header->gps_day,
						      header->gps_time_of_day);
	uint32_t period_sec = header->prediction_period_min * SEC_PER_MIN;
	int64_t end_sec = start_sec + header->prediction_count * period_sec;
	int64_t pred_sec = npgps_gps_day_time_to_sec(p->time.date_day,
						     p->time.time_full_s);

	if ((start_sec <= pred_sec) && (pred_sec < end_sec)) {
		return (int)((pred_sec - start_sec) / period_sec);
	} else {
		return -EINVAL;
	}
}

static void log_pgps_header(const char *msg, const struct nrf_cloud_pgps_header *header)
{
	LOG_INF("%sSchema version:%u, type:%u, num:%u, "
		"count:%u", msg ? msg : "",
		header->schema_version & 0xFFU, header->array_type & 0xFFU,
		header->num_items, header->prediction_count);
	LOG_INF("  size:%u, period (minutes):%u, GPS day:%u, GPS time:%u",
		header->prediction_size,
		header->prediction_period_min,
		header->gps_day & 0xFFFFU, header->gps_time_of_day);
}

static bool validate_pgps_header(const struct nrf_cloud_pgps_header *header)
{
	log_pgps_header("Checking P-GPS header: ", header);
	if ((header->schema_version != NRF_CLOUD_PGPS_BIN_SCHEMA_VERSION) ||
	    (header->array_type != NRF_CLOUD_PGPS_PREDICTION_HEADER) ||
	    (header->num_items != 1) ||
	    (header->prediction_period_min != PREDICTION_PERIOD) ||
	    (header->prediction_count <= 0) ||
	    (header->prediction_count > NUM_PREDICTIONS)) {
		if ((((uint8_t)header->schema_version) == 0xff) &&
		    (((uint8_t)header->array_type) == 0xff)) {
			LOG_WRN("Flash is erased.");
		} else {
			LOG_WRN("One or more fields are wrong");
		}
		return false;
	}
	return true;
}

static int validate_prediction(const struct nrf_cloud_pgps_prediction *p,
			       uint16_t gps_day,
			       uint32_t gps_time_of_day,
			       uint16_t period_min,
			       bool exact,
			       bool margin)
{
	int err = 0;

	/* validate that this prediction was actually updated and matches */
	if ((p->schema_version != NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION) ||
	    (p->time_type != NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK) ||
	    (p->time_count != 1)) {
		LOG_ERR("invalid prediction header");
		err = -EINVAL;
	} else if (exact && (p->time.date_day != gps_day)) {
		LOG_ERR("prediction day:%u, expected:%u",
			p->time.date_day, gps_day);
		err = -EINVAL;
	} else if (exact && (p->time.time_full_s != gps_time_of_day)) {
		LOG_ERR("prediction time:%u, expected:%u",
			p->time.time_full_s, gps_time_of_day);
		err = -EINVAL;
	}

	int64_t gps_sec = npgps_gps_day_time_to_sec(gps_day,
						    gps_time_of_day);
	int64_t pred_sec = npgps_gps_day_time_to_sec(p->time.date_day,
						     p->time.time_full_s);
	int64_t end_sec = pred_sec + period_min * SEC_PER_MIN;

	if (margin) {
		end_sec += PGPS_MARGIN_SEC;
	}

	if ((gps_sec < pred_sec) || (gps_sec > end_sec)) {
		LOG_ERR("prediction does not contain desired time; "
			"start:%lld, cur:%lld, end:%lld",
			pred_sec, gps_sec, end_sec);
		err = -EINVAL;
	}

	if ((p->ephemeris_type != NRF_CLOUD_AGPS_EPHEMERIDES) ||
	    (p->ephemeris_count != NRF_CLOUD_PGPS_NUM_SV)) {
		LOG_ERR("ephemeris header bad:%u, %u",
			p->ephemeris_type, p->ephemeris_count);
		err = -EINVAL;
	}

	if (exact && !err) {
		uint32_t expected_sentinel;
		uint32_t stored_sentinel;

		expected_sentinel = npgps_gps_day_time_to_sec(gps_day,
							      gps_time_of_day);
		stored_sentinel = p->sentinel;
		if (expected_sentinel != stored_sentinel) {
			LOG_ERR("prediction at:%p has stored_sentinel:0x%08X, "
				"expected:0x%08X", p, stored_sentinel,
				expected_sentinel);
			err = -EINVAL;
		}
	}
#if IS_ENABLED(CONFIG_NRF_CLOUD_LOG_LEVEL_DBG)
	if (!err) {
		print_time_details("prediction:", pred_sec, p->time.date_day, p->time.time_full_s);
	}
#endif
	return err;
}

static int validate_stored_predictions(uint16_t *first_bad_day,
				       uint32_t *first_bad_time)
{
	int err;
	int i;
	uint16_t count = index.header.prediction_count;
	uint16_t period_min = index.header.prediction_period_min;
	uint16_t gps_day = index.header.gps_day;
	uint32_t gps_time_of_day = index.header.gps_time_of_day;
	uint8_t *p = storage;
	struct nrf_cloud_pgps_prediction *pred;
	int64_t start_gps_sec = index.start_sec;
	int64_t gps_sec;
	int pnum;

	/* reset catalog of predictions */
	for (pnum = 0; pnum < count; pnum++) {
		index.predictions[pnum] = NULL;
	}

	npgps_reset_block_pool();

	/* build catalog of predictions by block */
	for (i = 0; i < count; i++) {
		pred = (struct nrf_cloud_pgps_prediction *)p;

		pnum = determine_prediction_num(&index.header, pred);
		if (pnum < 0) {
			LOG_ERR("prediction idx:%u, ofs:%p, out of expected time range;"
				" day:%u, time:%u", i, p, pred->time.date_day,
				pred->time.time_full_s);
		} else if (index.predictions[pnum] == NULL) {
			index.predictions[pnum] = pred;
			LOG_DBG("Prediction num:%u stored at idx:%d", pnum, i);
		} else {
			LOG_WRN("Prediction num:%u stored more than once!", pnum);
		}
		p += PGPS_PREDICTION_STORAGE_SIZE;
	}

	/* validate predictions in time order, independent of storage order */
	i = -1;
	for (pnum = 0; pnum < count; pnum++) {
		/* calculate expected time signature */
		gps_sec = start_gps_sec + pnum * period_min * SEC_PER_MIN;
		npgps_gps_sec_to_day_time(gps_sec, &gps_day, &gps_time_of_day);

		pred = index.predictions[pnum];
		if (pred == NULL) {
			LOG_WRN("Prediction num:%u missing", pnum);
			/* request partial data; download interrupted? */
			*first_bad_day = gps_day;
			*first_bad_time = gps_time_of_day;
			break;
		}

		err = validate_prediction(pred, gps_day, gps_time_of_day,
					  period_min, true, false);
		if (err) {
			LOG_ERR("Prediction num:%u, gps_day:%u, "
				"gps_time_of_day:%u is bad:%d; loc:%p",
				pnum, gps_day, gps_time_of_day, err, pred);
			/* request partial data; download interrupted? */
			*first_bad_day = gps_day;
			*first_bad_time = gps_time_of_day;
			break;
		}

		i = npgps_pointer_to_block((uint8_t *)pred);
		LOG_DBG("Prediction num:%u, loc:%p, blk:%d", pnum, pred, i);
		__ASSERT(i != -1, "unexpected pointer value %p", pred);
		npgps_mark_block_used(i, true);
	}

	/* find first free block in flash, if any, after chronologicaly
	 * last good prediction, if any; this is where any new downloads
	 * should begin, to maintain a circularly arranged flash
	 */
	if (i != -1) {
		LOG_DBG("finding first free after %d", i);
		i = npgps_find_first_free(i);
		if (i != -1) {
			LOG_DBG("first free:%d", i);
		} else {
			LOG_DBG("no free blocks");
		}
	}

	npgps_print_blocks();
	return pnum;
}

static void get_prediction_day_time(int pnum, int64_t *gps_sec, uint16_t *gps_day,
				    uint32_t *gps_time_of_day)
{
	int64_t psec = index.start_sec + (uint32_t)pnum * index.period_sec;

	if (gps_sec) {
		*gps_sec = psec;
	}
	if (gps_day || gps_time_of_day) {
		npgps_gps_sec_to_day_time(psec, gps_day, gps_time_of_day);
	}
}

static void discard_oldest_predictions(int num)
{
	int i;
	int pnum;
	int block;
	int last = MIN(num, index.header.prediction_count);

	/* ensure 'last' oldest predictions are free; we can already
	 * have some free, if a previous attempt to replace expired
	 * predictions failed (e.g., due to lack of LTE connection)
	 */
	LOG_INF("discarding %d", last);

	for (pnum = 0; pnum < last; pnum++) {
		block = npgps_pointer_to_block((uint8_t *)index.predictions[pnum]);
		__ASSERT((block != -1), "unexpected ptr:%p for Prediction num:%d",
			 index.predictions[pnum], pnum);
		npgps_free_block(block);
	}

	/* move predictions we are keeping to the start */
	for (i = last; i < index.header.prediction_count; i++) {
		pnum = i - last;
		index.predictions[pnum] = index.predictions[i];
	}

	/* set prediction pointers for 'last' in the newly empty
	 * entries to NULL
	 */
	for (pnum = index.header.prediction_count - last; pnum <
	      index.header.prediction_count; pnum++) {
		index.predictions[pnum] = NULL;
	}
	npgps_print_blocks();

	/* update index and header for new first stored prediction */
	get_prediction_day_time(last, &index.start_sec,
				&index.header.gps_day,
				&index.header.gps_time_of_day);
	LOG_DBG("updated index to gps_sec:%lld, day:%u, time:%u",
		index.start_sec, index.header.gps_day,
		index.header.gps_time_of_day);
}

int nrf_cloud_pgps_notify_prediction(void)
{
	/* when current prediction is ready, callback handler with
	 * PGPS_EVT_AVAILABLE and pointer to prediction
	 */
	int err;
	int pnum;
	struct nrf_cloud_pgps_prediction *prediction;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}
	LOG_DBG("num_predictions:%d, replacement threshold:%d",
		NUM_PREDICTIONS, REPLACEMENT_THRESHOLD);

	LOG_INF("Searching for prediction");
	err = nrf_cloud_pgps_find_prediction(&prediction);
	if (err == -ELOADING) {
		pgps_need_assistance = true;
		err = 0;
	} else if (err < 0) {
		if (!pgps_need_assistance) {
			pgps_need_assistance = true;
			err = nrf_cloud_pgps_request_all();
			if (err) {
				LOG_ERR("Error while requesting pgps set: %d", err);
				pgps_need_assistance = false; /* try again next time */
			}
		} else {
			err = 0;
		}
	} else if ((err >= 0) && (err < NUM_PREDICTIONS)) {
		pnum = err;
		err = 0;
		LOG_INF("Found P-GPS prediction %d", pnum);
		pgps_need_assistance = false;
		/* the application should now send data to modem with
		 * nrf_cloud_pgps_inject()
		 */
		if (handler) {
			handler(PGPS_EVT_AVAILABLE, prediction);
		}
	}
	return err;
}

static void prediction_work_handler(struct k_work *work)
{
	struct nrf_cloud_pgps_prediction *p;
	int ret;

	LOG_INF("prediction is expiring; finding next");
	ret = nrf_cloud_pgps_find_prediction(&p);
	if (ret >= 0) {
		LOG_DBG("found prediction %d; injecting to modem", ret);
		ret = nrf_cloud_pgps_inject(p, NULL, NULL);
		if (ret) {
			LOG_ERR("Error injecting prediction:%d", ret);
		} else {
			LOG_INF("Next prediction injected successfully.");
		}
	}
}

static void prediction_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&prediction_work);
}

static void start_expiration_timer(int pnum, int64_t cur_gps_sec)
{
	int64_t start_sec;
	int64_t end_sec;
	int64_t delta;

	if (k_timer_remaining_get(&prediction_timer) > 0) {
		return; /* timer already set */
	}
	get_prediction_day_time(pnum, &start_sec, NULL, NULL);
	end_sec = index.header.prediction_period_min * SEC_PER_MIN + start_sec;
	delta = (end_sec - cur_gps_sec) + 1;
	/* add 1 second to ensure we don't time out just slightly before
	 * the data actually expires (e.g. < 1 second)
	 */
	if (delta > 0) {
		k_timer_start(&prediction_timer, K_SECONDS(delta), K_NO_WAIT);
		LOG_INF("injecting next prediction in %lld seconds", delta);
	} else {
		LOG_ERR("cannot start prediction expiration timer; delta = %lld", delta);
	}
}

static void print_time_details(const char *info,
			       int64_t sec, uint16_t day, uint32_t time_of_day)
{
	uint32_t tow = (day % DAYS_PER_WEEK) * SEC_PER_DAY + time_of_day;

	LOG_INF("%s GPS sec:%llu, day:%u, time of day:%u, week:%lu, "
		"day of week:%lu, time of week:%u, toe:%u",
		info ? info : "", sec, day, time_of_day,
		day / DAYS_PER_WEEK, day % DAYS_PER_WEEK, tow, tow / 16);
}

int nrf_cloud_pgps_find_prediction(struct nrf_cloud_pgps_prediction **prediction)
{
	int64_t cur_gps_sec;
	int64_t offset_sec;
	int64_t start_sec = index.start_sec;
	int64_t end_sec = index.end_sec;
	uint16_t cur_gps_day;
	uint32_t cur_gps_time_of_day;
	uint16_t start_day = index.header.gps_day;
	uint32_t start_time = index.header.gps_time_of_day;
	uint16_t period_min = index.header.prediction_period_min;
	uint16_t count = index.header.prediction_count;
	int err;
	int pnum;
	bool margin = false;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}
	if (prediction == NULL) {
		return -EINVAL;
	}
	*prediction = NULL;

	if (index.stale_server_data) {
		LOG_ERR("server error: expired data");
		index.cur_pnum = 0xff;
		state = PGPS_EXPIRED;
		pgps_need_assistance = false; /* make sure we request it */
		return -ENODATA;
	}

	err = npgps_get_shifted_time(&cur_gps_sec, &cur_gps_day,
				     &cur_gps_time_of_day, PREDICTION_MIDPOINT_SHIFT_SEC);
	if (err < 0) {
		LOG_INF("Unknown current time");
		cur_gps_sec = 0;
	}

	print_time_details("Looking for prediction for:",
			   cur_gps_sec, cur_gps_day, cur_gps_time_of_day);

	if ((start_day == 0) && (start_time == 0)) {
		if (nrf_cloud_pgps_loading()) {
			LOG_WRN("Predictions not loaded yet");
			return -ELOADING;
		}
		LOG_WRN("No data stored");
		return -ENODATA;
	}

	offset_sec = cur_gps_sec - start_sec;

	print_time_details("First stored prediction:", start_sec, start_day, start_time);
	LOG_INF("current offset into prediction set, sec:%lld", offset_sec);

	if (offset_sec < 0) {
		/* current time must be unknown or very inaccurate; it
		 * falls before the first valid prediction; default to
		 * first entry and day and time for it
		 */
		LOG_WRN("cannot find prediction; real time not known");
		return -ETIMEUNKNOWN;
	} else if (cur_gps_sec > end_sec) {
		if ((cur_gps_sec - end_sec) > PGPS_MARGIN_SEC) {
			LOG_WRN("data expired!");
			return -ETIMEDOUT;
		}
		margin = true;
		/* data is expired; use most recent entry */
		pnum = count - 1;
	} else {
		pnum = offset_sec / (SEC_PER_MIN * period_min);
		if (pnum >= index.header.prediction_count) {
			LOG_WRN("prediction num:%d -- too large", pnum);
			pnum = index.header.prediction_count - 1;
		}
	}

	LOG_INF("Selected prediction num:%d", pnum);
	index.cur_pnum = pnum;
	*prediction = index.predictions[pnum];
	if (*prediction) {
		err = validate_prediction(*prediction,
					  cur_gps_day, cur_gps_time_of_day,
					  period_min, false, margin);
		if (!err) {
			start_expiration_timer(pnum, cur_gps_sec);
			return pnum;
		}
		return err;
	}
	if (nrf_cloud_pgps_loading()) {
		LOG_WRN("Prediction num:%u not loaded yet", pnum);
		return -ELOADING;
	}
	LOG_ERR("Prediction num:%u not available; state:%d", pnum, state);
	return -EINVAL;
}

static cJSON *json_create_req_obj(const char *const app_id,
				   const char *const msg_type)
{
	__ASSERT_NO_MSG(app_id != NULL);
	__ASSERT_NO_MSG(msg_type != NULL);

	if (!json_initialized) {
		cJSON_Init();
		json_initialized = true;
	}

	cJSON *resp_obj = cJSON_CreateObject();

	if (!cJSON_AddStringToObject(resp_obj,
				     PGPS_JSON_APPID_KEY,
				     app_id) ||
	    !cJSON_AddStringToObject(resp_obj,
				     PGPS_JSON_MSG_TYPE_KEY,
				     msg_type)) {
		cJSON_Delete(resp_obj);
		resp_obj = NULL;
	}

	return resp_obj;
}

static int json_send_to_cloud(cJSON *const pgps_request)
{
	__ASSERT_NO_MSG(pgps_request != NULL);

	char *msg_string;
	int err;

	msg_string = cJSON_PrintUnformatted(pgps_request);
	if (!msg_string) {
		LOG_ERR("Could not allocate memory for P-GPS request message");
		return -ENOMEM;
	}

	LOG_DBG("Created P-GPS request: %s", log_strdup(msg_string));

	struct nct_dc_data msg = {
		.data.ptr = msg_string,
		.data.len = strlen(msg_string)
	};

	err = nct_dc_send(&msg);
	if (err) {
		/* @TODO: if device is offline, we need to defer this to later */
		LOG_ERR("Failed to send P-GPS request, error:%d", err);
	} else {
		LOG_DBG("P-GPS request sent");
	}

	k_free(msg_string);

	return err;
}

bool nrf_cloud_pgps_loading(void)
{
	return ((state == PGPS_REQUESTING) || (state == PGPS_LOADING));
}

int nrf_cloud_pgps_request(const struct gps_pgps_request *request)
{
	int err = 0;
	cJSON *data_obj;
	cJSON *pgps_req_obj;
	cJSON *ret;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}

	if (nrf_cloud_pgps_loading()) {
		return 0;
	}
	ignore_packets = false;

	LOG_INF("Requesting %u predictions...", request->prediction_count);

	/* Create request JSON containing a data object */
	pgps_req_obj = json_create_req_obj(PGPS_JSON_APPID_VAL_PGPS,
					   PGPS_JSON_MSG_TYPE_VAL_DATA);
	data_obj = cJSON_AddObjectToObject(pgps_req_obj, PGPS_JSON_DATA_KEY);

	if (!pgps_req_obj || !data_obj) {
		err = -ENOMEM;
		goto cleanup;
	}

#if defined(CONFIG_PGPS_INCLUDE_MODEM_INFO)
	/* Add modem info and P-GPS types to the data object */
	err = json_add_modem_info(data_obj);
	if (err) {
		LOG_ERR("Failed to add modem info to P-GPS request:%d", err);
		goto cleanup;
	}
#endif
	if (request->prediction_count < index.header.prediction_count) {
		index.partial_request = true;
		index.pnum_offset = index.header.prediction_count -
				    request->prediction_count;
	} else {
		index.partial_request = false;
		index.pnum_offset = 0;
	}
	index.expected_count = request->prediction_count;

	ret = cJSON_AddNumberToObject(data_obj, PGPS_JSON_PRED_COUNT,
				      request->prediction_count);
	if (ret == NULL) {
		LOG_ERR("Failed to add pred count to P-GPS request:%d", err);
		err = -ENOMEM;
		goto cleanup;
	}
	ret = cJSON_AddNumberToObject(data_obj, PGPS_JSON_PRED_INT_MIN,
				      request->prediction_period_min);
	if (ret == NULL) {
		LOG_ERR("Failed to add pred int min to P-GPS request:%d", err);
		err = -ENOMEM;
		goto cleanup;
	}
	ret = cJSON_AddNumberToObject(data_obj, PGPS_JSON_GPS_DAY,
				      request->gps_day);
	if (ret == NULL) {
		LOG_ERR("Failed to add gps day to P-GPS request:%d", err);
		err = -ENOMEM;
		goto cleanup;
	}
	ret = cJSON_AddNumberToObject(data_obj, PGPS_JSON_GPS_TIME,
				      request->gps_time_of_day);
	if (ret == NULL) {
		LOG_ERR("Failed to add gps time to P-GPS request:%d", err);
		err = -ENOMEM;
		goto cleanup;
	}

	err = json_send_to_cloud(pgps_req_obj);
	if (!err) {
		state = PGPS_REQUESTING;
	}

cleanup:
	cJSON_Delete(pgps_req_obj);

	return err;
}

int nrf_cloud_pgps_request_all(void)
{
	uint16_t gps_day;
	uint32_t gps_time_of_day;
	int err;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}

	if (nrf_cloud_pgps_loading()) {
		return 0;
	}

	npgps_reset_block_pool();

	index.stale_server_data = false;
	err = npgps_get_time(NULL, &gps_day, &gps_time_of_day);
	if (err) {
		/* unknown time; ask server for newest data */
		gps_day = 0;
		gps_time_of_day = 0;
	}

	struct gps_pgps_request request = {
		.prediction_count = NUM_PREDICTIONS,
		.prediction_period_min = PREDICTION_PERIOD,
		.gps_day = gps_day,
		.gps_time_of_day = gps_time_of_day
	};

	return nrf_cloud_pgps_request(&request);
}

int nrf_cloud_pgps_preemptive_updates(void)
{
	/* keep unexpired, read newer subsequent to last */
	struct gps_pgps_request request;
	int current = index.cur_pnum;
	int n = NUM_PREDICTIONS - REPLACEMENT_THRESHOLD;
	uint16_t period_min = index.header.prediction_period_min;
	uint16_t gps_day = 0;
	uint32_t gps_time_of_day = 0;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}

	if (current == 0xff) {
		return nrf_cloud_pgps_request_all();
	}
	if ((current + npgps_num_free()) < n) {
		LOG_DBG("Updates not needed yet; current:%d, free:%d, n:%d",
			current, npgps_num_free(), n);
		return 0;
	}

	if (handler) {
		handler(PGPS_EVT_LOADING, NULL);
	}

	LOG_INF("Replacing %d oldest predictions; %d already free",
		current, npgps_num_free());
	if (current >= n) {
		discard_oldest_predictions(current);
	}
	npgps_gps_sec_to_day_time(index.end_sec, &gps_day, &gps_time_of_day);

	request.gps_day = gps_day;
	request.gps_time_of_day = gps_time_of_day;
	request.prediction_count = npgps_num_free();
	request.prediction_period_min = period_min;
	return nrf_cloud_pgps_request(&request);
}

int nrf_cloud_pgps_inject(struct nrf_cloud_pgps_prediction *p,
			  const struct gps_agps_request *request,
			  const int *socket)
{
	int err;
	int ret = 0;
	struct gps_agps_request remainder;
	struct gps_agps_request processed;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}

	if (request != NULL) {
		memcpy(&remainder, request, sizeof(remainder));
	} else {
		/* no assistance request provided from modem; assume just ephemerides */
		memset(&remainder, 0, sizeof(remainder));
		remainder.sv_mask_ephe = 0xFFFFFFFFU;
	}
	nrf_cloud_agps_processed(&processed);
	LOG_DBG("A-GPS has processed emask:0x%08X amask:0x%08X utc:%u "
		"klo:%u neq:%u tow:%u pos:%u int:%u",
		processed.sv_mask_ephe, processed.sv_mask_alm,
		processed.utc, processed.klobuchar,
		processed.nequick, processed.system_time_tow,
		processed.position, processed.integrity);

	/* we will get more accurate data from AGPS for these */
	if (processed.position && remainder.position) {
		LOG_DBG("A-GPS already received position; skipping");
		remainder.position = 0;
	}
	if (processed.system_time_tow && remainder.system_time_tow) {
		LOG_DBG("A-GPS already received time; skipping");
		remainder.system_time_tow = 0;
	}

	if (remainder.system_time_tow) {
		struct pgps_sys_time sys_time;
		uint16_t day;
		uint32_t sec;

		sys_time.schema_version = NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION;
		sys_time.type = NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK;
		sys_time.count = 1;
		sys_time.time.time_frac_ms = 0;
		sys_time.time.sv_mask = 0;

		err = npgps_get_time(NULL, &day, &sec);
		if (!err) {
			sys_time.time.date_day = day;
			sys_time.time.time_full_s = sec;
			LOG_INF("GPS unit needs time assistance. Injecting day:%u, time:%u",
				day, sec);

			/* send time */
			err = nrf_cloud_agps_process((const char *)&sys_time,
						     sizeof(sys_time) -
						     sizeof(sys_time.time.sv_tow),
						     socket);
			if (err) {
				LOG_ERR("Error injecting P-GPS sys_time (%u, %u): %d",
					sys_time.time.date_day, sys_time.time.time_full_s,
					err);
				ret = err;
			}
		} else {
			LOG_WRN("Current time not known; cannot provide time assistance");
		}
		/* it is ok to ignore this err in the function return below */
	} else {
		LOG_INF("GPS unit does not need time assistance.");
	}

	const struct gps_location *saved_location = npgps_get_saved_location();

	if (remainder.position && saved_location->gps_sec) {
		struct pgps_location location;

		location.schema_version = NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION;
		location.type = NRF_CLOUD_AGPS_LOCATION;
		location.count = 1;
		location.location.latitude = saved_location->latitude;
		location.location.longitude = saved_location->longitude;
		location.location.altitude = 0;
		location.location.unc_semimajor = LOCATION_UNC_SEMIMAJOR_K;
		location.location.unc_semiminor = LOCATION_UNC_SEMIMINOR_K;
		location.location.orientation_major = 0;
		location.location.unc_altitude = 0xFF; /* tell modem it is invalid */
		location.location.confidence = LOCATION_CONFIDENCE_PERCENT;

		LOG_INF("GPS unit needs position. Injecting lat:%d, lng:%d",
			saved_location->latitude,
			saved_location->longitude);
		/* send location */

		err = nrf_cloud_agps_process((const char *)&location, sizeof(location),
					     socket);
		if (err) {
			LOG_ERR("Error injecting P-GPS location (%d, %d): %d",
				location.location.latitude, location.location.longitude,
				err);
			ret = err;
		}
	} else if (remainder.position) {
		LOG_WRN("GPS unit needs location, but it is unknown!");
	} else {
		LOG_INF("GPS unit does not need location assistance.");
	}

	if (remainder.sv_mask_ephe) {
		LOG_INF("GPS unit needs ephemerides. Injecting %u.", p->ephemeris_count);

		/* send ephemerii */
		err = nrf_cloud_agps_process((const char *)&p->schema_version,
					     sizeof(p->schema_version) +
					     sizeof(p->ephemeris_type) +
					     sizeof(p->ephemeris_count) +
					     sizeof(p->ephemerii), socket);
		if (err) {
			LOG_ERR("Error injecting ephermerii:%d", err);
			ret = err;
		}
	} else {
		LOG_INF("GPS unit does not need ephemerides.");
	}
	return ret; /* just return last non-zero error, if any */
}

#if VERIFY_FLASH
static int flash_callback(uint8_t *buf, size_t len, size_t offset)
{
	size_t erased_start = 0;
	bool erased_found = false;
	int err;

	struct nrf_cloud_pgps_prediction *pred = (struct nrf_cloud_pgps_prediction *)buf;

	if ((pred->time_type == 0xFFU) || (pred->sentinel == 0xFFFFFFFFU)) {
		erased_found = true;
	}
	if (erased_found) {
		LOG_ERR("Erased block at offset:0x%zX len:%zu contains FFs "
			"starting at offset:0x%zX", offset, len, erased_start);
		LOG_HEXDUMP_ERR(buf, len, "block dump");
		err = -ENODATA;
	} else {
		LOG_INF("Block at offset:0x%zX len %zu: written", offset, len);
		err = 0;
	}
	return err;
}
#else
void *flash_callback;
#endif

static int open_storage(uint32_t offset, bool preserve)
{
	int err;
	const struct device *flash_dev;
	uint32_t block_offset = 0;
	const char *name = DT_PROP(DT_NODELABEL(flash_controller), label);

	LOG_DBG("storage name:%s", name);
	flash_dev = device_get_binding(name);
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get device:'%s'", name);
		return -EFAULT;
	}

	block_offset = offset % flash_page_size;
#if PGPS_DEBUG
	LOG_DBG("flash_page_size:%u, block_offset:%u, offset:%u, preserve:%d",
		flash_page_size, block_offset, offset, preserve);
#endif
	offset -= block_offset;

	err = stream_flash_init(&stream, flash_dev,
				write_buf, flash_page_size,
				((uint32_t)storage) + offset,
				storage_size - offset, flash_callback);
	if (err) {
		LOG_ERR("Failed to init flash stream for offset %u: %d",
			offset, err);
		state = PGPS_NONE;
		return err;
	}

	if (preserve && (block_offset != 0) && (block_offset < flash_page_size)) {
		uint8_t *p = storage + offset;

#if PGPS_DEBUG
		LOG_DBG("preserving %u bytes", block_offset);
		LOG_HEXDUMP_DBG(p, MIN(32, block_offset), "start of preserved data");
#endif
		err = stream_flash_buffered_write(&stream, p, block_offset, false);
		if (err) {
			LOG_ERR("Error writing back %u original bytes", block_offset);
		}
	}
	return err;
}

static int store_prediction(uint8_t *p, size_t len, uint32_t sentinel, bool last)
{
	static bool first = true;
	static uint8_t pad[PGPS_PREDICTION_PAD];
	int err;
	uint8_t schema = NRF_CLOUD_AGPS_BIN_SCHEMA_VERSION;
	size_t schema_offset = ((size_t) &((struct nrf_cloud_pgps_prediction *)0)->schema_version);

	if (first) {
		memset(pad, 0xff, PGPS_PREDICTION_PAD);
		first = false;
	}

	err = stream_flash_buffered_write(&stream, p, schema_offset, false);
	if (err) {
		LOG_ERR("Error writing pgps prediction:%d", err);
		return err;
	}
	p += schema_offset;
	len -= schema_offset;
	err = stream_flash_buffered_write(&stream, &schema, sizeof(schema), false);
	if (err) {
		LOG_ERR("Error writing schema:%d", err);
		return err;
	}
	err = stream_flash_buffered_write(&stream, p, len, false);
	if (err) {
		LOG_ERR("Error writing pgps prediction:%d", err);
		return err;
	}
	err = stream_flash_buffered_write(&stream, (uint8_t *)&sentinel,
					  sizeof(sentinel), false);
	if (err) {
		LOG_ERR("Error writing sentinel:%d", err);
	}
	err = stream_flash_buffered_write(&stream, pad, PGPS_PREDICTION_PAD, last);
	if (err) {
		LOG_ERR("Error writing sentinel:%d", err);
	}
	return err;
}

static int flush_storage(void)
{
	return stream_flash_buffered_write(&stream, NULL, 0, true);
}

static int process_buffer(uint8_t *buf, size_t len)
{
	int err;
	size_t need;
	int64_t gps_sec;

	if (index.dl_offset == 0) {
		struct nrf_cloud_pgps_header *header;

		if (len < sizeof(*header)) {
			return -EINVAL; /* need full header, for now */
		}
		LOG_DBG("Consuming P-GPS header len:%zd", len);
		err = consume_pgps_header(buf, len);
		if (err) {
			return err;
		}
		LOG_INF("Storing P-GPS header");
		header = (struct nrf_cloud_pgps_header *)buf;
		cache_pgps_header(header);

		err = npgps_get_shifted_time(&gps_sec, NULL, NULL,
					     PREDICTION_MIDPOINT_SHIFT_SEC);
		if (!err) {
			if ((index.start_sec <= gps_sec) &&
			    (gps_sec <= index.end_sec)) {
				LOG_INF("Received data covers good timeframe");
			} else {
				if (index.start_sec > gps_sec) {
					LOG_ERR("Received data is not within required "
						"timeframe!  Start of predictions is "
						"in the future by %lld seconds",
						index.start_sec - gps_sec);
				} else {
					LOG_ERR("Received data is not within required "
						"timeframe!  End of predictions is "
						"in the past by %lld seconds",
						gps_sec - index.end_sec);
				}
				index.stale_server_data = true;
				memset(header, 0, sizeof(*header));
				cache_pgps_header(header);
				return -EINVAL;
			}
		} else {
			LOG_WRN("Current time unknown; assume data's timeframe is valid");
		}
		log_pgps_header("pgps_header: ", header);
		npgps_save_header(header);

		len -= sizeof(*header);
		buf += sizeof(*header);
		index.dl_offset += sizeof(*header);
		index.dl_pnum = index.pnum_offset;
		index.pred_offset = 0;
	}

	need = MIN((PGPS_PREDICTION_DL_SIZE - index.pred_offset), len);
	memcpy(&prediction_buf[index.pred_offset], buf, need);
	LOG_DBG("need:%zd bytes; pred_offset:%u, fragment len:%zd, dl_ofs:%zd",
		need, index.pred_offset, len, index.dl_offset);
	len -= need;
	buf += need;
	index.pred_offset += need;
	index.dl_offset += need;

	if (index.pred_offset == PGPS_PREDICTION_DL_SIZE) {
		LOG_DBG("consuming data prediction num:%u, remainder:%zd",
			index.dl_pnum, len);
		err = consume_pgps_data(index.dl_pnum, prediction_buf,
					PGPS_PREDICTION_DL_SIZE);
		if (err) {
			return err;
		}
		if (len) { /* keep extra data for next time */
			memcpy(prediction_buf, buf, len);
		}
		index.pred_offset = len;
		index.dl_pnum++;
	}
	index.dl_offset += len;
	return 0;
}

static int consume_pgps_header(const char *buf, size_t buf_len)
{
	struct nrf_cloud_pgps_header *header = (struct nrf_cloud_pgps_header *)buf;

	if (!validate_pgps_header(header)) {
		state = PGPS_NONE;
		return -EINVAL;
	}

	if (index.partial_request) {
		LOG_INF("Partial request; starting at prediction num:%u", index.pnum_offset);

		/* fix up header with full set info */
		header->prediction_count = index.header.prediction_count;
		header->gps_day = index.header.gps_day;
		header->gps_time_of_day = index.header.gps_time_of_day;
	}

	return 0;
}

static void cache_pgps_header(const struct nrf_cloud_pgps_header *header)
{
	memcpy(&index.header, header, sizeof(*header));

	index.start_sec = npgps_gps_day_time_to_sec(index.header.gps_day,
						    index.header.gps_time_of_day);
	index.period_sec = index.header.prediction_period_min * SEC_PER_MIN;
	index.end_sec = index.start_sec +
			index.period_sec * index.header.prediction_count;
}

static size_t get_next_pgps_element(struct nrf_cloud_apgs_element *element,
				    const char *buf)
{
	static uint16_t elements_left_to_process;
	static enum nrf_cloud_agps_type element_type;
	size_t len = 0;

	/* Check if there are more elements left in the array to process.
	 * The element type is only given once before the array, and not for
	 * each element.
	 */
	if (elements_left_to_process == 0) {
		element->type = (enum nrf_cloud_agps_type)
				buf[NRF_CLOUD_AGPS_BIN_TYPE_OFFSET];
		element_type = element->type;
		elements_left_to_process =
			*(uint16_t *)&buf[NRF_CLOUD_AGPS_BIN_COUNT_OFFSET] - 1;
		len += NRF_CLOUD_AGPS_BIN_TYPE_SIZE +
			NRF_CLOUD_AGPS_BIN_COUNT_SIZE;
	} else {
		element->type = element_type;
		elements_left_to_process -= 1;
	}

	switch (element->type) {
	case NRF_CLOUD_AGPS_EPHEMERIDES:
		element->ephemeris = (struct nrf_cloud_agps_ephemeris *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_ephemeris);
		break;
	case NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK:
		element->time_and_tow =
			(struct nrf_cloud_agps_system_time *)(buf + len);
		len += sizeof(struct nrf_cloud_agps_system_time) -
			sizeof(element->time_and_tow->sv_tow) + 4;
		break;
	default:
		LOG_DBG("Unhandled P-GPS data type:%d", element->type);
		return 0;
	}

#if PGPS_DEBUG
	LOG_DBG("  size:%zd, count:%u", len, elements_left_to_process);
#endif
	return len;
}

static int consume_pgps_data(uint8_t pnum, const char *buf, size_t buf_len)
{
	struct nrf_cloud_apgs_element element = {};
	uint8_t *prediction_ptr = (uint8_t *)buf;
	uint8_t *element_ptr = prediction_ptr;
	struct agps_header *elem = (struct agps_header *)element_ptr;
	size_t parsed_len = 0;
	int64_t gps_sec;
	bool finished = false;

	gps_sec = 0;

	LOG_DBG("Parsing prediction num:%u, idx:%u, type:%u, count:%u, buf len:%u",
		pnum, index.loading_count, elem->type, elem->count, buf_len);

	while (parsed_len < buf_len) {
		bool empty;
		size_t element_size = get_next_pgps_element(&element, element_ptr);

		if (element_size == 0) {
			LOG_INF("  End of element");
			break;
		}
		switch (element.type) {
		case NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK:
			gps_sec = npgps_gps_day_time_to_sec(element.time_and_tow->date_day,
							    element.time_and_tow->time_full_s);
			break;
		case NRF_CLOUD_AGPS_EPHEMERIDES:
			/* check for all zeros except first byte (sv_id) */
			empty = true;
			for (int i = 1; i < sizeof(struct nrf_cloud_agps_ephemeris); i++) {
				if (element_ptr[i] != 0) {
					empty = false;
					break;
				}
			}
			if (empty) {
				LOG_INF("Marking ephemeris:%u as empty",
					element.ephemeris->sv_id);
				element.ephemeris->health = NRF_CLOUD_PGPS_EMPTY_EPHEM_HEALTH;
			}
			break;
		default: /* just store */
			break;
		}

		parsed_len += element_size;
		element_ptr += element_size;
	}

	if (parsed_len == buf_len) {
		LOG_DBG("Parsing finished");

		if (index.predictions[pnum]) {
			LOG_WRN("Received duplicate MQTT packet; ignoring");
		} else if (gps_sec == 0) {
			LOG_ERR("Prediction did not include GPS day and time of day; ignoring");
			LOG_HEXDUMP_DBG(prediction_ptr, buf_len, "bad data");
		} else {
			int err;

			LOG_INF("Storing prediction num:%u idx:%u for gps sec:%lld",
				pnum, index.loading_count, gps_sec);

			index.loading_count++;
			finished = (index.loading_count == index.expected_count);
			store_prediction(prediction_ptr, buf_len, (uint32_t)gps_sec,
					 finished || (index.storage_extent == 1));
			index.predictions[pnum] = npgps_block_to_pointer(index.store_block);

			if (pgps_need_assistance &&
			    (finished || (index.loading_count > 1))) {
				nrf_cloud_pgps_notify_prediction();
			}

			if (!finished) {
				if (handler) {
					handler(PGPS_EVT_LOADING, NULL);
				}
			} else {
				LOG_INF("All P-GPS data received. Done.");
				state = PGPS_READY;
				if (handler) {
					handler(PGPS_EVT_READY, NULL);
				}
				npgps_print_blocks();
				return 0;
			}

			index.store_block = npgps_alloc_block();
			if (index.store_block == NO_BLOCK) {
				LOG_ERR("No more free blocks!");
				return -ENOMEM;
			}
			index.storage_extent--;
			if (index.storage_extent == 0) {
				index.storage_extent = npgps_get_block_extent(index.store_block);
				LOG_INF("Moving to new flash region:%d, len:%d",
					index.store_block, index.storage_extent);
				err = flush_storage();
				if (err) {
					LOG_ERR("Error flushing storage:%d", err);
					return err;
				}
				err = open_storage(npgps_block_to_offset(index.store_block),
						   false);
				if (err) {
					LOG_ERR("Error opening storage again:%d", err);
					return err;
				}
			}
		}
	} else {
		LOG_ERR("Parsing incomplete; aborting.");
		state = PGPS_NONE;
		return -EINVAL;
	}

	return 0;
}

static int get_string_from_array(const cJSON *const array, const int index,
				  char **string_out)
{
	__ASSERT_NO_MSG(string_out != NULL);

	cJSON *item = cJSON_GetArrayItem(array, index);

	if (!cJSON_IsString(item)) {
		return -EINVAL;
	}

	*string_out = item->valuestring;

	return 0;
}

static int parse_dl_info(char *host, size_t host_len,
			 char *path, size_t path_len,
			 const char *payload_in, size_t len)
{
	if (!host || !path || !payload_in) {
		return -EINVAL;
	}

	char *temp;
	char *host_ptr = NULL;
	char *path_ptr = NULL;
	int err = -ENOMSG;
	cJSON *array = cJSON_Parse(payload_in);

	if (!array || !cJSON_IsArray(array)) {
		LOG_ERR("Invalid JSON array");
		err = -EINVAL;
		goto cleanup;
	}

	temp = cJSON_PrintUnformatted(array);
	if (temp) {
		LOG_DBG("JSON array: %s", log_strdup(temp));
		cJSON_FreeString(temp);
	}

	if (get_string_from_array(array, RCV_ITEM_IDX_FILE_HOST, &host_ptr) ||
	    get_string_from_array(array, RCV_ITEM_IDX_FILE_PATH, &path_ptr)) {
		LOG_ERR("Error parsing info");
		goto cleanup;
	}

	if (host_ptr && host_len) {
		strncpy(host, host_ptr, host_len - 1);
		host[host_len - 1] = '\0';
		LOG_DBG("host: %s", log_strdup(host));
	}
	if (path_ptr && path_len) {
		strncpy(path, path_ptr, path_len - 1);
		path[path_len - 1] = '\0';
		LOG_DBG("path: %s", log_strdup(path));
	}
	err = 0;

cleanup:
	if (array) {
		cJSON_Delete(array);
	}
	return err;
}

/* handle incoming MQTT packets */
int nrf_cloud_pgps_process(const char *buf, size_t buf_len)
{
	static char host[CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE];
	static char path[CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE];
	static uint8_t prev_pnum;
	uint8_t pnum;
	int err;

	if (state == PGPS_NONE) {
		LOG_ERR("P-GPS subsystem is not initialized.");
		return -EINVAL;
	}
	LOG_HEXDUMP_DBG(buf, buf_len, "MQTT packet");
	if (!buf_len) {
		LOG_ERR("Zero length packet received");
		state = PGPS_NONE;
		return -EINVAL;
	}

	if (ignore_packets) {
		LOG_ERR("IGNORING PACKETS");
		return -EINVAL;
	}

	if (state < PGPS_LOADING) {
		state = PGPS_LOADING;
		prev_pnum = 0xff;
		/* until we get the header (which may or may not come first,
		 * preinitialize the header with expected values
		 */
		if (!index.partial_request) {
			index.header.prediction_count = NUM_PREDICTIONS;
			index.header.prediction_period_min = PREDICTION_PERIOD;
			index.period_sec =
				index.header.prediction_period_min * SEC_PER_MIN;
			memset(index.predictions, 0, sizeof(index.predictions));
		} else {
			for (pnum = index.pnum_offset;
			     pnum < index.expected_count + index.pnum_offset; pnum++) {
				index.predictions[pnum] = NULL;
			}
		}
		index.loading_count = 0;
		index.store_block = npgps_alloc_block();
		if (index.store_block == NO_BLOCK) {
			LOG_ERR("No free flash space!");
			return -ENOMEM;
		}
		index.storage_extent = npgps_get_block_extent(index.store_block);
		LOG_INF("opening storage at block:%d, len:%d", index.store_block,
			index.storage_extent);
		err = open_storage(npgps_block_to_offset(index.store_block),
				   index.partial_request);
		if (err) {
			state = PGPS_NONE;
			return err;
		}
	}

	err = parse_dl_info(host, sizeof(host), path, sizeof(path), buf, buf_len);
	if (err) {
		return err;
	}
	index.dl_offset = 0;
	ignore_packets = true;
	int sec_tag = SEC_TAG;

	if (FORCE_HTTP_DL && (strncmp(host, "https", 5) == 0)) {
		int i;
		int len = strlen(host);

		for (i = 4; i < len; i++) {
			host[i] = host[i + 1];
		}
		sec_tag = -1;
	}

	return npgps_download_start(host, path, sec_tag, NULL, FRAGMENT_SIZE);
}

int nrf_cloud_pgps_init(struct nrf_cloud_pgps_init_param *param)
{
	int err = 0;

	__ASSERT(((REPLACEMENT_THRESHOLD & 1) == 0),
		 "REPLACEMENT_THRESHOLD must be even");
	__ASSERT(param != NULL, "param must be provided");
	__ASSERT(param->storage_base != 0, "storage must be provided");
	__ASSERT((param->storage_size >= (NUM_BLOCKS * BLOCK_SIZE)),
		 "insufficient storage provided; need at least %u bytes",
		 (NUM_BLOCKS * BLOCK_SIZE));

	handler = param->event_handler;
	if (handler) {
		handler(PGPS_EVT_INIT, NULL);
	}

	flash_page_size = nrfx_nvmc_flash_page_size_get();
	if (!flash_page_size) {
		flash_page_size = 4096;
	}

	if (nrf_cloud_pgps_loading()) {
		return 0;
	}

	state = PGPS_NONE;

	if (!write_buf) {
		write_buf = k_malloc(flash_page_size);
		if (!write_buf) {
			return -ENOMEM;
		}
#if PGPS_DEBUG
		agps_print_enable(true);

		LOG_DBG("agps_system_time size:%u, "
			"agps_ephemeris size:%u, "
			"prediction struct size:%u",
			sizeof(struct nrf_cloud_pgps_system_time),
			sizeof(struct nrf_cloud_agps_ephemeris),
			sizeof(struct nrf_cloud_pgps_prediction));
#endif
	}

	storage = (uint8_t *)param->storage_base;
	storage_size = param->storage_size;
	(void)ngps_block_pool_init(param->storage_base, NUM_PREDICTIONS);

	memset(&index, 0, sizeof(index));
	(void)npgps_settings_init();

	err = npgps_download_init(process_buffer);
	if (err) {
		LOG_ERR("Error initializing download client:%d", err);
		return err;
	}

	state = PGPS_INITIALIZING;

	uint16_t num_valid = 0;
	uint16_t count = 0;
	uint16_t period_min  = 0;
	uint16_t gps_day = 0;
	uint32_t gps_time_of_day = 0;
	const struct nrf_cloud_pgps_header *saved_header;

	saved_header = npgps_get_saved_header();
	if (validate_pgps_header(saved_header)) {
		cache_pgps_header(saved_header);

		count = index.header.prediction_count;
		period_min = index.header.prediction_period_min;
		gps_day = index.header.gps_day;
		gps_time_of_day = index.header.gps_time_of_day;

		/* check for all predictions up to date;
		 * if missing some, get from server
		 */
		LOG_INF("Checking stored P-GPS data; count:%u, period_min:%u",
			count, period_min);
		num_valid = validate_stored_predictions(&gps_day, &gps_time_of_day);
	}

	struct nrf_cloud_pgps_prediction *test_prediction;
	int pnum = -1;

	LOG_DBG("num_valid:%u, count:%u", num_valid, count);
	if (num_valid) {
		LOG_INF("Checking if P-GPS data is expired...");
		err = nrf_cloud_pgps_find_prediction(&test_prediction);
		if (err == -ETIMEDOUT) {
			LOG_WRN("Predictions expired. Requesting predictions...");
			num_valid = 0;
		} else if (err >= 0) {
			LOG_INF("Found valid prediction, day:%u, time:%u",
				test_prediction->time.date_day,
				test_prediction->time.time_full_s);
			pnum = err;
		}
	}

	if (!num_valid) {
		/* read a full set of predictions */
		if (handler) {
			handler(PGPS_EVT_UNAVAILABLE, NULL);
		}
		err = nrf_cloud_pgps_request_all();
	} else if (num_valid < count) {
		/* read missing predictions at end */
		struct gps_pgps_request request;

		if (handler) {
			handler(PGPS_EVT_LOADING, NULL);
		}
		LOG_INF("Incomplete P-GPS data; "
			"requesting %u predictions...", count - num_valid);
		get_prediction_day_time(num_valid, NULL, &gps_day, &gps_time_of_day);
		request.gps_day = gps_day;
		request.gps_time_of_day = gps_time_of_day;
		request.prediction_count = count - num_valid;
		request.prediction_period_min = period_min;
		err = nrf_cloud_pgps_request(&request);
	} else if ((count - (pnum + 1)) < REPLACEMENT_THRESHOLD) {
		/* replace expired predictions with newer */
		err = nrf_cloud_pgps_preemptive_updates();
	} else {
		state = PGPS_READY;
		LOG_INF("P-GPS data is up to date.");
		if (handler) {
			handler(PGPS_EVT_READY, NULL);
		}
		err = 0;
	}
	return err;
}
