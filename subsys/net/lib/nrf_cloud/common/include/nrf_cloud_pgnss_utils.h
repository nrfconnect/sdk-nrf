/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#ifndef NRF_CLOUD_PGNSS_UTILS_H_
#define NRF_CLOUD_PGNSS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* (6.1.1980 UTC - 1.1.1970 UTC) */
#define GPS_TO_UNIX_UTC_OFFSET_SECONDS (315964800UL)
#define GPS_TO_UTC_LEAP_SECONDS	       (18UL)
#define DAYS_PER_WEEK		       (7UL)
#define SECONDS_PER_WEEK	       (SEC_PER_DAY * DAYS_PER_WEEK)

#define LAT_DEG_TO_DEV_UNITS(_lat_)  ((int32_t)(((_lat_) / 90.0) * (1 << 23)))
#define LNG_DEG_TO_DEV_UNITS(_lng_)  ((int32_t)(((_lng_) / 360.0) * (1 << 24)))
#define SAVED_LOCATION_MIN_DELTA_SEC (12 * SEC_PER_HOUR)
#define SAVED_LOCATION_LAT_DELTA     LAT_DEG_TO_DEV_UNITS(0.1) /* ~11km (at equator) */
#define SAVED_LOCATION_LNG_DELTA     LNG_DEG_TO_DEV_UNITS(0.1) /* ~11km */

#define PGNSS_MARGIN_SEC 0
#define NUM_PREDICTIONS CONFIG_NRF_CLOUD_PGNSS_NUM_PREDICTIONS
#define NUM_BLOCKS	NUM_PREDICTIONS
#define BLOCK_SIZE	PGNSS_PREDICTION_STORAGE_SIZE
#define NO_BLOCK	-1

struct gps_location {
	int32_t latitude;
	int32_t longitude;
	int64_t gps_sec;
};

struct nrf_cloud_pgnss_header;

typedef int (*npgnss_buffer_handler_t)(uint8_t *buf, size_t len);

typedef void (*npgnss_eot_handler_t)(int err);

/* access control functions */
int npgnss_download_lock(void);
void npgnss_download_unlock(void);

/* settings functions */
int npgnss_save_header(struct nrf_cloud_pgnss_header *header);
const struct nrf_cloud_pgnss_header *npgnss_get_saved_header(void);
const struct gps_location *npgnss_get_saved_location(void);
int npgnss_settings_init(void);

/* time functions */
int64_t npgnss_gps_day_time_to_sec(uint16_t gps_day, uint32_t gps_time_of_day);
void npgnss_gps_sec_to_day_time(int64_t gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day);
int npgnss_get_shifted_time(int64_t *gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day,
			   uint32_t shift);
int npgnss_get_time(int64_t *gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day);

/* flash block allocation functions */
int ngps_block_pool_init(uint32_t base_address, int num);
int npgnss_alloc_block(void);
void npgnss_undo_alloc_block(int block);
void npgnss_free_block(int block);
int npgnss_get_block_extent(int block);
void npgnss_reset_block_pool(void);
void npgnss_mark_block_used(int block, bool used);
void npgnss_print_blocks(void);
int npgnss_num_free(void);
int npgnss_find_first_free(int from_block);
int npgnss_offset_to_block(uint32_t offset);
uint32_t npgnss_block_to_offset(int block);
int npgnss_pointer_to_block(uint8_t *p);
void *npgnss_block_to_pointer(int block);

/* download functions */
int npgnss_download_init(npgnss_buffer_handler_t buf_handler, npgnss_eot_handler_t eot_handler);
int npgnss_download_start(const char *host, const char *file, int sec_tag, uint8_t pdn_id,
			 size_t fragment_size);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_PGNSS_UTILS_H_ */
