/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_PGPS_H_
#define NRF_CLOUD_PGPS_H_

/** @file nrf_cloud_pgps.h
 * @brief Module to provide nRF Cloud Predicted GPS (P-GPS) support to nRF9160 SiP.
 */

#include <zephyr.h>
#include <drivers/gps.h>
#include "nrf_cloud_agps_schema_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_pgps nRF Cloud P-GPS
 * @{
 */

#define NRF_CLOUD_PGPS_NUM_SV				(32U)

/** @brief nrf_cloud_pgps_system_time is a special version of
 * nrf_cloud_agps_system_time that does not include the full array of sv_tow
 * values; this is transferred from the cloud as part of each prediction,
 * to indicate the date_day and time_full_s for the center of each prediction's
 * validity period.
 */
struct nrf_cloud_pgps_system_time {
	/** Number of days since GPS time began on 6 Jan 1980 for the
	 * center of a prediction's validity period.
	 */
	uint16_t date_day;
	/** Seconds into GPS day for center of validity period. */
	uint32_t time_full_s;
	/** Will be 0. */
	uint16_t time_frac_ms;
	/** Will be 0. */
	uint32_t sv_mask;
	/** Placeholder where sv_tow[32] is for A-GPS. */
	uint32_t pad;
} __packed;

/** @brief P-GPS prediction Flash storage format. */
struct nrf_cloud_pgps_prediction {
	/** Set to NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK. */
	uint8_t time_type;
	/** Will be 1. */
	uint16_t time_count;
	/** Information about when this prediction is applicable. */
	struct nrf_cloud_pgps_system_time time;
	/** Not from cloud; inserted during storage to ease reusing A-GPS code. */
	uint8_t schema_version;
	/** Set to NRF_CLOUD_AGPS_EPHEMERIDES. */
	uint8_t ephemeris_type;
	/** Usually will be NRF_CLOUD_PGPS_NUM_SV. */
	uint16_t ephemeris_count;
	/** Array of satellite orbital equation coefficients. */
	struct nrf_cloud_agps_ephemeris ephemerii[NRF_CLOUD_PGPS_NUM_SV];
	/** Not from cloud; appended during storage to verify integrity on retrieval. */
	uint32_t sentinel;
} __packed;

/** @brief P-GPS request type */
struct gps_pgps_request {
	/** Number of predictions desired. */
	uint16_t prediction_count;
	/** Validity time per prediction, in minutes. Valid range 120 to 480. */
	uint16_t prediction_period_min;
	/** Number of days since GPS time began on 6 Jan 1980 for the
	 * center of the first prediction desired.
	 */
	uint16_t gps_day;
	/** Number of seconds since the start of this GPS day for the
	 * center of the first prediction desired.  Valid range 0 to 86399.
	 */
	uint32_t gps_time_of_day;
} __packed;

/** @brief P-GPS error code: current time unknown. */
#define ETIMEUNKNOWN	8000
/** @brief P-GPS error code: not found but loading in progress. */
#define ELOADING	8001

/** @brief Value to mark the ephemeris as unavailable for satellites for which
 * no predictions are available from the cloud.
 */
#define NRF_CLOUD_PGPS_EMPTY_EPHEM_HEALTH (0xff)

/** @brief P-GPS event passed to the registered pgps_event_handler. */
enum nrf_cloud_pgps_event {
	/** P-GPS initialization beginning. */
	PGPS_EVT_INIT,
	/** There are currently no P-GPS predictions available. */
	PGPS_EVT_UNAVAILABLE,
	/** P-GPS predictions are being loaded from the cloud. */
	PGPS_EVT_LOADING,
	/** A P-GPS prediction is available now for the current date and time. */
	PGPS_EVT_AVAILABLE,
	/** All P-GPS predictions are available. */
	PGPS_EVT_READY
};

/**
 * @brief  Event handler registered with the module to handle asynchronous
 * events from the module.
 *
 * @param[in] event The event that just occurred.
 * @param[in] p For event PGPS_EVT_AVAILABLE, a pointer to the prediction;
 * otherwise, NULL.
 */
typedef void (*pgps_event_handler_t)(enum nrf_cloud_pgps_event event,
				     struct nrf_cloud_pgps_prediction *p);

/**@brief Initialization parameters for the module. */
struct nrf_cloud_pgps_init_param {
	/** Event handler that is registered with the module. */
	pgps_event_handler_t event_handler;
	/** Flash storage address. Must be on a Flash page boundary. */
	uint32_t storage_base;
	/** Flash storage size. Must be a multiple of a Flash page in size, in bytes. */
	uint32_t storage_size;
};

/**@brief Update storage of the most recent known location, in modem-specific
 * normalized format (int32_t).
 * Current time is also stored.
 * Normalization:
 *   (latitude / 90.0) * (1 << 23)
 *   (longitude / 360.0) * (1 << 24)
 *
 * @param latitude Current latitude normalized.
 * @param longitude Current longitude in normalized.
 */
void nrf_cloud_pgps_set_location_normalized(int32_t latitude, int32_t longitude);

/**@brief Update the storage of the most recent known location in degrees.
 * This will be injected along with the current time and relevant predicted
 * ephemerides to the GPS unit in order to get the fastest possible fix, when
 * the P-GPS subsystem is built with A-GPS disabled, or when A-GPS data is
 * unavailable due to lack of a cloud connection.
 * Current time is also stored.
 *
 * @param latitude Current latitude in degrees.
 * @param longitude Current longitude in degrees.
 */
void nrf_cloud_pgps_set_location(double latitude, double longitude);

/**@brief Update the storage of the leap second offset between GPS time
 * and UTC. This called automatically by the A-GPS subsystem (if enabled)
 * when it receives a UTC assistance element, setting leap_seconds to  the delta_tls field.
 *
 * @param leap_seconds Offset in seconds.
 */
void nrf_cloud_pgps_set_leap_seconds(int leap_seconds);

/**@brief Schedule a callback when prediction for current time is
 * available.  Callback could be immediate, if data already stored in
 * Flash, or later, after loading from the cloud.
 *
 * @return 0 if scheduled successfully, or negative error code if
 * could not send request to cloud.
 */
int nrf_cloud_pgps_notify_prediction(void);

/**@brief Tries to find an appropriate GPS prediction for the current time.
 *
 * @param prediction Pointer to a pointer to a prediction; the pointer at
 * this pointer will be modified to point to the prediction if the return value
 * is 0.  Will be set to NULL on failure.
 *
 * @return 0..NumPredictions-1 if successful; -ETIMEUNKNOWN if current date and time
 * not known; -ETIMEDOUT if all predictions stored are expired;
 * -EINVAL if prediction for the current time is invalid.
 */
int nrf_cloud_pgps_find_prediction(struct nrf_cloud_pgps_prediction **prediction);

/**@brief Requests specified P-GPS data from nRF Cloud.
 *
 * @param request Pointer to structure specifying what P-GPS data is desired.
 * The request may fail if there no cloud connection; if the specified GPS
 * day and GPS time of day is in the past or more than 2 weeks in the future;
 * if the GPS time of day is larger than 86339; or if the prediction_period_min
 * field is not within the range 120 to 480.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_pgps_request(const struct gps_pgps_request *request);

/**@brief Requests all available P-GPS data from nRF Cloud.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_pgps_request_all(void);

/**@brief Processes binary P-GPS data received from nRF Cloud.
 *
 * @param buf Pointer to data received from nRF Cloud.
 * @param buf_len Buffer size of data to be processed.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_pgps_process(const char *buf, size_t buf_len);

/**@brief Injects binary P-GPS data to the modem. If request is NULL,
 * it is assumed that only ephemerides assistance should be injected.
 *
 * @param p Pointer to a prediction.
 * @param request Which assistance elements the modem needs. May be NULL.
 * @param socket Pointer to GNSS socket to which P-GPS data will be injected.
 * If NULL, the nRF9160 GPS driver is used to inject the data.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_pgps_inject(struct nrf_cloud_pgps_prediction *p,
			  const struct gps_agps_request *request,
			  const int *socket);

/**@brief Find out if P-GPS update is in progress
 *
 * @return True if request sent but loading not yet completed.
 */
bool nrf_cloud_pgps_loading(void);

/**@brief Download more predictions if less than CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD
 * predictions remain which are still valid.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int nrf_cloud_pgps_preemptive_updates(void);

/**@brief Initialize P-GPS subsystem. Validates what is stored, then
 * requests any missing predictions, or full set if expired or missing.
 * When successful, it is ready to provide valid ephemeris predictions.
 *
 * @warning It must return successfully before using P-GPS services.
 *
 * @param[in] param Initialization parameters.
 *
 * @return 0 if valid or request begun; nonzero on error.
 */
int nrf_cloud_pgps_init(struct nrf_cloud_pgps_init_param *param);

/** @} */

#ifdef __cplusplus
#endif

#endif /* NRF_CLOUD_PGPS_H_ */
