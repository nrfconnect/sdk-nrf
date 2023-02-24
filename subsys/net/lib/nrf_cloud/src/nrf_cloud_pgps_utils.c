/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>

#include <net/nrf_cloud_pgps.h>
#include <zephyr/settings/settings.h>
#include <net/download_client.h>
#include <date_time.h>

#include "nrf_cloud_transport.h"
#include "nrf_cloud_pgps_schema_v1.h"
#include "nrf_cloud_pgps_utils.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(nrf_cloud_pgps, CONFIG_NRF_CLOUD_GPS_LOG_LEVEL);

#define SOCKET_RETRIES				CONFIG_NRF_CLOUD_PGPS_SOCKET_RETRIES
#define SETTINGS_NAME				"nrf_cloud_pgps"
#define SETTINGS_KEY_PGPS_HEADER		"pgps_header"
#define SETTINGS_FULL_PGPS_HEADER		SETTINGS_NAME "/" SETTINGS_KEY_PGPS_HEADER
#define SETTINGS_KEY_LOCATION			"location"
#define SETTINGS_FULL_LOCATION			SETTINGS_NAME "/" SETTINGS_KEY_LOCATION
#define SETTINGS_KEY_LEAP_SEC			"g2u_leap_sec"
#define SETTINGS_FULL_LEAP_SEC			SETTINGS_NAME "/" SETTINGS_KEY_LEAP_SEC

struct block_pool {
	int first_free;
	uint32_t last_alloc;
	bool block_used[NUM_PREDICTIONS];
};

static struct block_pool pool;
static uint8_t *block_pool_base;
static int num_blocks;

/* todo: get the correct values from somewhere */
static int gps_leap_seconds = GPS_TO_UTC_LEAP_SECONDS;
static struct gps_location saved_location;
static struct nrf_cloud_pgps_header saved_header;

static K_SEM_DEFINE(dl_active, 1, 1);

static struct download_client dlc;
static int socket_retries_left;
static npgps_buffer_handler_t buffer_handler;
static npgps_eot_handler_t eot_handler;

static int download_client_callback(const struct download_client_evt *event);
static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg);

SETTINGS_STATIC_HANDLER_DEFINE(nrf_cloud_pgps, SETTINGS_NAME, NULL, settings_set,
			       NULL, NULL);

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!key) {
		return -EINVAL;
	}

	LOG_DBG("Settings key:%s, size:%d", key, len_rd);

	if (!strncmp(key, SETTINGS_KEY_PGPS_HEADER,
		     strlen(SETTINGS_KEY_PGPS_HEADER)) &&
		(len_rd == sizeof(saved_header))) {
		if (read_cb(cb_arg, (void *)&saved_header, len_rd) == len_rd) {
			LOG_DBG("Read pgps_header: count:%u, period:%u, day:%d, time:%d",
				saved_header.prediction_count, saved_header.prediction_period_min,
				saved_header.gps_day, saved_header.gps_time_of_day);
			return 0;
		}
	}
	if (!strncmp(key, SETTINGS_KEY_LOCATION,
		     strlen(SETTINGS_KEY_LOCATION)) &&
	    (len_rd == sizeof(saved_location))) {
		if (read_cb(cb_arg, (void *)&saved_location, len_rd) == len_rd) {
			LOG_DBG("Read location:%d, %d, gps sec:%d",
				saved_location.latitude, saved_location.longitude,
				(int32_t)saved_location.gps_sec);
			return 0;
		}
	}
	if (!strncmp(key, SETTINGS_KEY_LEAP_SEC,
		     strlen(SETTINGS_KEY_LEAP_SEC)) &&
	    (len_rd == sizeof(gps_leap_seconds))) {
		if (read_cb(cb_arg, (void *)&gps_leap_seconds, len_rd) == len_rd) {
			LOG_DBG("Read gps to utc leap seconds offset:%d",
				gps_leap_seconds);
			return 0;
		}
	}
	return -ENOTSUP;
}

int npgps_download_lock(void)
{
	int err = k_sem_take(&dl_active, K_NO_WAIT);

	if (!err) {
		LOG_DBG("dl_active locked");
	} else {
		LOG_ERR("Unable to lock dl_active: %d", err);
	}
	return err;
}

void npgps_download_unlock(void)
{
	k_sem_give(&dl_active);
	LOG_DBG("dl_active unlocked");
}

int npgps_save_header(struct nrf_cloud_pgps_header *header)
{
	int ret = 0;

	LOG_DBG("Saving pgps header");
	ret = settings_save_one(SETTINGS_FULL_PGPS_HEADER, header, sizeof(*header));
	return ret;
}

const struct nrf_cloud_pgps_header *npgps_get_saved_header(void)
{
	return &saved_header;
}

/* @TODO: consider rate-limiting these updates to reduce Flash wear */
static int save_location(void)
{
	int ret = 0;

	LOG_DBG("Saving location:%d, %d; gps sec:%d",
		saved_location.latitude, saved_location.longitude,
		(int32_t)saved_location.gps_sec);
	ret = settings_save_one(SETTINGS_FULL_LOCATION,
				&saved_location, sizeof(saved_location));
	return ret;
}

const struct gps_location *npgps_get_saved_location(void)
{
	return &saved_location;
}

static int save_leap_sec(void)
{
	int ret = 0;

	LOG_DBG("Saving gps to utc leap seconds offset:%d", gps_leap_seconds);
	ret = settings_save_one(SETTINGS_FULL_LEAP_SEC,
				&gps_leap_seconds, sizeof(gps_leap_seconds));
	return ret;
}

int npgps_settings_init(void)
{
	int ret = 0;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Settings init failed:%d", ret);
		return ret;
	}
	ret = settings_load_subtree(settings_handler_nrf_cloud_pgps.name);
	if (ret) {
		LOG_ERR("Cannot load settings:%d", ret);
	}
	return ret;
}

void nrf_cloud_pgps_set_location_normalized(int32_t latitude, int32_t longitude)
{
	int64_t sec;

	if (npgps_get_time(&sec, NULL, NULL)) {
		sec = saved_location.gps_sec; /* could not get time; use prev */
	}

	if ((abs(latitude - saved_location.latitude) > SAVED_LOCATION_LAT_DELTA) ||
	    (abs(longitude - saved_location.longitude) > SAVED_LOCATION_LNG_DELTA) ||
	    ((sec - saved_location.gps_sec) > SAVED_LOCATION_MIN_DELTA_SEC)) {
		saved_location.latitude = latitude;
		saved_location.longitude = longitude;
		saved_location.gps_sec = sec;
		save_location();
	}
}

void nrf_cloud_pgps_set_location(double latitude, double longitude)
{
	int32_t lat;
	int32_t lng;

	lat = LAT_DEG_TO_DEV_UNITS(latitude);
	lng = LNG_DEG_TO_DEV_UNITS(longitude);

	nrf_cloud_pgps_set_location_normalized(lat, lng);
}

void nrf_cloud_pgps_clear_location(void)
{
	if (saved_location.gps_sec) {
		saved_location.gps_sec = 0;
		save_location();
	}
}

void nrf_cloud_pgps_set_leap_seconds(int leap_seconds)
{
	if (gps_leap_seconds != leap_seconds) {
		gps_leap_seconds = leap_seconds;
		save_leap_sec();
	}
}

static int64_t utc_to_gps_sec(const int64_t utc, int16_t *gps_time_ms)
{
	int64_t utc_sec;
	int64_t gps_sec;

	utc_sec = utc / MSEC_PER_SEC;
	if (gps_time_ms) {
		*gps_time_ms = (uint16_t)(utc - (utc_sec * MSEC_PER_SEC));
	}
	gps_sec = (utc_sec - GPS_TO_UNIX_UTC_OFFSET_SECONDS) + gps_leap_seconds;

	LOG_DBG("Converted UTC sec:%d to GPS sec:%d", (int32_t)utc_sec, (int32_t)gps_sec);
	return gps_sec;
}

int64_t npgps_gps_day_time_to_sec(uint16_t gps_day, uint32_t gps_time_of_day)
{
	int64_t gps_sec = (int64_t)gps_day * SEC_PER_DAY + gps_time_of_day;

#if PGPS_DEBUG
	LOG_DBG("Converted GPS day:%u, time of day:%u to GPS sec:%d",
		gps_day, gps_time_of_day, (int32_t)gps_sec);
#endif
	return gps_sec;
}

void npgps_gps_sec_to_day_time(int64_t gps_sec, uint16_t *gps_day,
			       uint32_t *gps_time_of_day)
{
	uint16_t day;
	uint32_t time;

	day = (uint16_t)(gps_sec / SEC_PER_DAY);
	time = (uint32_t)(gps_sec - (day * SEC_PER_DAY));
#if PGPS_DEBUG
	LOG_DBG("Converted GPS sec:%d to day:%u, time of day:%u, week:%u",
		(int32_t)gps_sec, day, time,
		(uint16_t)(day / DAYS_PER_WEEK));
#endif
	if (gps_day) {
		*gps_day = day;
	}
	if (gps_time_of_day) {
		*gps_time_of_day = time;
	}
}

int npgps_get_shifted_time(int64_t *gps_sec,
			   uint16_t *gps_day, uint32_t *gps_time_of_day,
			   uint32_t shift)
{
	int64_t now;
	int err;

	err = date_time_now(&now);
	if (!err) {
		now += (int64_t)shift * MSEC_PER_SEC;
		now = utc_to_gps_sec(now, NULL);
		npgps_gps_sec_to_day_time(now, gps_day, gps_time_of_day);
		if (gps_sec != NULL) {
			*gps_sec = now;
		}
	}
	return err;
}

int npgps_get_time(int64_t *gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day)
{
	return npgps_get_shifted_time(gps_sec, gps_day, gps_time_of_day, 0);
}

int ngps_block_pool_init(uint32_t base_address, int num)
{
	block_pool_base = (uint8_t *)base_address;
	num_blocks = num;
	return 0;
}

int npgps_alloc_block(void)
{
	int idx;

	if (pool.first_free < 0) {
		LOG_DBG("no blocks");
		return NO_BLOCK;
	}

	idx = pool.first_free;
	pool.block_used[pool.first_free] = true;
	pool.first_free = (pool.first_free + 1) % num_blocks;
	if (pool.block_used[pool.first_free]) {
		pool.first_free = NO_BLOCK;
	}
	LOG_DBG("alloc:%d", idx);
	return idx;
}

void npgps_free_block(int block)
{
	LOG_DBG("free:%d", block);
	if (pool.first_free < 0) {
		pool.first_free = block;
	}
	pool.block_used[block] = false;
}

int npgps_get_block_extent(int block)
{
	int i;
	int len = 0;

	for (i = 0; i < num_blocks; i++) {
		if (pool.block_used[block]) {
			break;
		}
		block = (block + 1) % num_blocks;
		len++;
	}
	return len;
}

void npgps_reset_block_pool(void)
{
	int i;

	LOG_DBG("resetting block pool");
	pool.first_free = 0;
	for (i = 0; i < num_blocks; i++) {
		pool.block_used[i] = false;
	}
}

void npgps_mark_block_used(int block, bool used)
{
	__ASSERT((block >= 0) && (block < num_blocks), "block %d out of range", block);
	pool.block_used[block] = used;
	LOG_DBG("mark idx:%d = %u", block, used);
}

void npgps_print_blocks(void)
{
	char map[num_blocks + 1];
	int i;

	LOG_DBG("num_blocks:%u, size:%u, first_free:%d", num_blocks,
		BLOCK_SIZE, pool.first_free);
	for (i = 0; i < num_blocks; i++) {
		map[i] = pool.block_used[i] ? '1' : '0';
	}
	map[i] = '\0';
	LOG_DBG("map:%s", map);
}


int npgps_num_free(void)
{
	int num = 0;

	for (int i = 0; i < num_blocks; i++) {
		if (!pool.block_used[i]) {
			num++;
		}
	}
	return num;
}

int npgps_find_first_free(int from_block)
{
	int i;
	int start = from_block;

	pool.first_free = NO_BLOCK;
	for (i = 0; i < num_blocks; i++) {
		if (!pool.block_used[from_block]) {
			pool.first_free = from_block;
			break;
		}
		from_block = (from_block + 1) % num_blocks;
	}
	LOG_DBG("first_free idx:%d from:%d", from_block, start);
	return pool.first_free;
}

int npgps_offset_to_block(uint32_t offset)
{
	int block = offset / BLOCK_SIZE;

	if ((block < 0) || (block >= num_blocks)) {
		LOG_ERR("invalid offset:%u", offset);
		block = NO_BLOCK;
	}
	return block;
}

uint32_t npgps_block_to_offset(int block)
{
	if ((block < 0) || (block >= num_blocks)) {
		LOG_ERR("invalid block:%d", block);
		return 0;
	}
	return block * BLOCK_SIZE;
}

int npgps_pointer_to_block(uint8_t *p)
{
	int ret = (uint32_t)(p - block_pool_base) / BLOCK_SIZE;

	LOG_DBG("ptr:%p to block idx:%d", (void *)p, ret);
	if ((ret < 0) || (ret >= num_blocks)) {
		return NO_BLOCK;
	}
	return ret;
}

void *npgps_block_to_pointer(int block)
{
	void *ret;

	if ((block < 0) || (block >= num_blocks)) {
		LOG_ERR("invalid block:%d", block);
		ret = NULL;
	} else {
		ret = (void *)(block_pool_base + block * BLOCK_SIZE);
	}
	LOG_DBG("block idx:%d to ptr:%p", block, ret);
	return ret;
}

int npgps_download_init(npgps_buffer_handler_t buf_handler, npgps_eot_handler_t end_handler)
{
	__ASSERT(buf_handler != NULL, "Must specify buffer handler");
	__ASSERT(end_handler != NULL, "Must specify end of transfer handler");
	buffer_handler = buf_handler;
	eot_handler = end_handler;

	return download_client_init(&dlc, download_client_callback);
}

int npgps_download_start(const char *host, const char *file, int sec_tag,
			 uint8_t pdn_id, size_t fragment_size)
{
	if (host == NULL || file == NULL) {
		return -EINVAL;
	}

	int err;

	socket_retries_left = SOCKET_RETRIES;

	struct download_client_cfg config = {
		.sec_tag = sec_tag,
		.pdn_id = pdn_id,
		.frag_size_override = fragment_size,
		.set_tls_hostname = (sec_tag != -1),
	};

	err = download_client_get(&dlc, host, &config, file, 0);
	if (err != 0) {
		download_client_disconnect(&dlc);
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
		err = buffer_handler((uint8_t *)event->fragment.buf,
				     event->fragment.len);
		if (!err) {
			return err;
		}
		break;
	case DOWNLOAD_CLIENT_EVT_DONE:
		LOG_INF("Download client done");
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
			return 0;
		}
		err = -EIO;
		break;
	}
	default:
		return 0;
	}

	int ret = download_client_disconnect(&dlc);

	if (ret) {
		LOG_ERR("Error disconnecting from "
			"download client:%d", ret);
		err = ret;
	}

	eot_handler(err);
	return err;
}
