/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#ifndef NRF_CLOUD_PGPS_UTILS_H_
#define NRF_CLOUD_PGPS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* (6.1.1980 UTC - 1.1.1970 UTC) */
#define GPS_TO_UNIX_UTC_OFFSET_SECONDS	(315964800UL)
#define GPS_TO_UTC_LEAP_SECONDS		(18UL)
#define SEC_PER_HOUR			(MIN_PER_HOUR * SEC_PER_MIN)
#define SEC_PER_DAY			(HOUR_PER_DAY * SEC_PER_HOUR)
#define DAYS_PER_WEEK			(7UL)
#define SECONDS_PER_WEEK		(SEC_PER_DAY * DAYS_PER_WEEK)

#define LAT_DEG_TO_DEV_UNITS(_lat_)	((int32_t)(((_lat_) / 90.0) * (1 << 23)))
#define LNG_DEG_TO_DEV_UNITS(_lng_)	((int32_t)(((_lng_) / 360.0) * (1 << 24)))
#define SAVED_LOCATION_MIN_DELTA_SEC	(12 * SEC_PER_HOUR)
#define SAVED_LOCATION_LAT_DELTA	LAT_DEG_TO_DEV_UNITS(0.1) /* ~11km (at equator) */
#define SAVED_LOCATION_LNG_DELTA	LNG_DEG_TO_DEV_UNITS(0.1) /* ~11km */

#define PGPS_MARGIN_SEC			0
#define NUM_PREDICTIONS			CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS
#define NUM_BLOCKS			NUM_PREDICTIONS
#define BLOCK_SIZE			PGPS_PREDICTION_STORAGE_SIZE
#define NO_BLOCK			-1

struct gps_location {
	int32_t latitude;
	int32_t longitude;
	int64_t gps_sec;
};

struct nrf_cloud_pgps_header;

typedef int (*npgps_buffer_handler_t)(uint8_t *buf, size_t len);

typedef void (*npgps_eot_handler_t)(int err);

/* access control functions */
int npgps_download_lock(void);
void npgps_download_unlock(void);

/* settings functions */
int npgps_save_header(struct nrf_cloud_pgps_header *header);
const struct nrf_cloud_pgps_header *npgps_get_saved_header(void);
const struct gps_location *npgps_get_saved_location(void);
int npgps_settings_init(void);

/* time functions */
int64_t npgps_gps_day_time_to_sec(uint16_t gps_day, uint32_t gps_time_of_day);
void npgps_gps_sec_to_day_time(int64_t gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day);
int npgps_get_shifted_time(int64_t *gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day,
			   uint32_t shift);
int npgps_get_time(int64_t *gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day);

/* flash block allocation functions */
int ngps_block_pool_init(uint32_t base_address, int num);
int npgps_alloc_block(void);
void npgps_free_block(int block);
int npgps_get_block_extent(int block);
void npgps_reset_block_pool(void);
void npgps_mark_block_used(int block, bool used);
void npgps_print_blocks(void);
int npgps_num_free(void);
int npgps_find_first_free(int from_block);
int npgps_offset_to_block(uint32_t offset);
uint32_t npgps_block_to_offset(int block);
int npgps_pointer_to_block(uint8_t *p);
void *npgps_block_to_pointer(int block);

/* download functions */
int npgps_download_init(npgps_buffer_handler_t buf_handler, npgps_eot_handler_t eot_handler);
int npgps_download_start(const char *host, const char *file, int sec_tag,
			 uint8_t pdn_id, size_t fragment_size);


#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_PGPS_UTILS_H_ */
