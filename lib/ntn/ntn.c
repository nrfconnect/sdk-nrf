/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_monitor.h>
#include <modem/at_parser.h>
#include <modem/lte_lc.h>
#include <modem/ntn.h>

LOG_MODULE_REGISTER(ntn, CONFIG_NTN_LOG_LEVEL);

#define PI				3.14159265358979323846
#define EARTH_RADIUS_METERS		(6371.0 * 1000.0)

#define LOCATION_MAX_VALIDITY_MS	(60 * MSEC_PER_SEC)

#define AT_LOCATION_SUBSCRIBE		"AT%%LOCATION=1"
#define AT_LOCATION_ACCURACY_INDEX	1
#define AT_LOCATION_NEXT_ACCURACY_INDEX	2
#define AT_LOCATION_NEXT_IN_S_INDEX	3
#define AT_LOCATION_UPDATE		"AT%%LOCATION=2,\"%.6f\",\"%.6f\",\"%.1f\",%d,%d"
#define AT_LOCATION_INVALIDATE		"AT%%LOCATION=2"

struct location {
	uint64_t timestamp;
	double latitude;
	double longitude;
	float altitude;
	float speed;
};

/* Current location. */
static struct location current_location;
/* Location set to the modem. */
static struct location modem_location;

/* Accuracy requested by the modem. */
static uint32_t req_accuracy;
/* Next accuracy requested by the modem. */
static uint32_t next_req_accuracy;

/* Previous updates_requested value in the NTN_EVT_MODEM_LOCATION_UPDATE event. */
static bool prev_updates_requested;

/* Event handler registered by the application. */
static ntn_evt_handler_t event_handler;

/* Work queue for NTN library. */
static K_THREAD_STACK_DEFINE(ntn_workq_stack, CONFIG_NTN_WORKQUEUE_STACK_SIZE);
static struct k_work_q ntn_workq;

AT_MONITOR(ntn_atmon_location, "%LOCATION", at_handler_location);

static void req_accuracy_update_work_fn(struct k_work *work);
static void modem_location_update_work_fn(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(req_accuracy_update_work, req_accuracy_update_work_fn);
K_WORK_DELAYABLE_DEFINE(modem_location_update_work, modem_location_update_work_fn);

static int parse_location_notif(
	const char *at_response,
	uint32_t *accuracy,
	uint32_t *next_accuracy,
	uint32_t *next_in_s)
{
	int err;
	struct at_parser parser;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(accuracy != NULL);
	__ASSERT_NO_MSG(next_accuracy != NULL);
	__ASSERT_NO_MSG(next_in_s != NULL);

	*accuracy = 0;
	*next_accuracy = 0;
	*next_in_s = 0;

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	err = at_parser_num_get(&parser, AT_LOCATION_ACCURACY_INDEX, accuracy);
	if (err) {
		LOG_ERR("Failed to parse location notification, error: %d", err);
		return err;
	}

	/* Optional parameters */

	err = at_parser_num_get(&parser, AT_LOCATION_NEXT_ACCURACY_INDEX, next_accuracy);
	if (err && err != -EIO) {
		LOG_ERR("Failed to parse location notification, error: %d", err);
		return err;
	}

	err = at_parser_num_get(&parser, AT_LOCATION_NEXT_IN_S_INDEX, next_in_s);
	if (err && err != -EIO) {
		LOG_ERR("Failed to parse location notification, error: %d", err);
		return err;
	}

	return 0;
}

static uint32_t location_age_get_ms(struct location *location)
{
	if (location->timestamp == 0) {
		return UINT32_MAX;
	} else {
		return (uint32_t)(k_uptime_get() - location->timestamp);
	}
}

/* Returns the distance between two coordinates in meters. The distance is calculated using the
 * haversine formula.
 */
static double distance_calculate(struct location *modem, struct location *current)
{
	double d_lat_rad;
	double d_lon_rad;
	double lat1_rad;
	double lat2_rad;
	double a;
	double c;

	d_lat_rad = (current->latitude - modem->latitude) * PI / 180.0;
	d_lon_rad = (current->longitude - modem->longitude) * PI / 180.0;

	lat1_rad = modem->latitude * PI / 180.0;
	lat2_rad = current->latitude * PI / 180.0;

	a = pow(sin(d_lat_rad / 2), 2) + pow(sin(d_lon_rad / 2), 2) * cos(lat1_rad) * cos(lat2_rad);

	c = 2 * asin(sqrt(a));

	return EARTH_RADIUS_METERS * c;
}

static uint32_t location_validity_time_calculate(struct location *location)
{
	double speed;
	double validity_ms;
	uint32_t validity_s;

	/* Avoid division by zero. */
	speed = (location->speed > 0.0f) ? location->speed : 0.1f;

	/* Location validity depends on requested accuracy and current speed. */
	validity_ms = req_accuracy * MSEC_PER_SEC / speed;

	/* Subtract the age of the location from the validity time. */
	validity_ms -= k_uptime_get() - location->timestamp;

	/* Limit maximum location validity time. */
	validity_ms = MIN(validity_ms, LOCATION_MAX_VALIDITY_MS);

	/* Validity needs to be at least 2 seconds to have some margin for the next update. */
	validity_s = MAX(validity_ms / 1000, 2);

	return validity_s;
}

static void event_dispatch(const struct ntn_evt *evt)
{
	if (event_handler != NULL) {
		event_handler(evt);
	}
}

static void req_accuracy_update_work_fn(struct k_work *work)
{
	struct ntn_evt evt = {
		.type = NTN_EVT_MODEM_LOCATION_UPDATE,
		.modem_location.updates_requested = false
	};

	LOG_DBG("Updating requested accuracy from %d m to %d m", req_accuracy, next_req_accuracy);

	req_accuracy = next_req_accuracy;
	next_req_accuracy = 0;

	if (req_accuracy == 0) {
		LOG_INF("Location updates not requested");

		k_work_cancel_delayable(&modem_location_update_work);
	} else {
		LOG_INF("Location updates requested, accuracy: %d m", req_accuracy);

		evt.modem_location.updates_requested = true;

		/* Send location update to modem immediately to avoid delays, if the current
		 * location is available.
		 */
		k_work_reschedule_for_queue(&ntn_workq,	&modem_location_update_work, K_NO_WAIT);
	}

	if (prev_updates_requested != evt.modem_location.updates_requested) {
		event_dispatch(&evt);

		prev_updates_requested = evt.modem_location.updates_requested;
	}
}

static void modem_location_update_work_fn(struct k_work *work)
{
	int err;
	uint32_t validity;

	/* Location which is over 5 seconds old is considered too old. */
	if (location_age_get_ms(&current_location) > (5 * MSEC_PER_SEC)) {
		LOG_WRN("Modem needs location, but current location is too old or unavailable");
		memset(&modem_location, 0, sizeof(modem_location));
		return;
	}

	validity = location_validity_time_calculate(&current_location);

	err = nrf_modem_at_printf(AT_LOCATION_UPDATE,
				  current_location.latitude,
				  current_location.longitude,
				  (double)current_location.altitude,
				  req_accuracy,
				  validity);
	if (err) {
		LOG_ERR("Failed to update location to modem, error: %d", err);
	} else {
		modem_location = current_location;
		/* Schedule next update 1s before expiration. */
		k_work_reschedule_for_queue(&ntn_workq,
					    &modem_location_update_work,
					    K_SECONDS(validity - 1));
		LOG_INF("Location updated to modem, validity: %d s", validity);
	}
}

static void at_handler_location(const char *notif)
{
	int err;
	uint32_t next_in_s;
	struct ntn_evt evt = {
		.type = NTN_EVT_MODEM_LOCATION_UPDATE,
		.modem_location.updates_requested = false
	};

	err = parse_location_notif(notif, &req_accuracy, &next_req_accuracy, &next_in_s);
	if (err) {
		return;
	}

	if (req_accuracy == 0) {
		LOG_INF("Location updates not required immediately");

		k_work_cancel_delayable(&modem_location_update_work);
	} else {
		LOG_INF("Location updates required immediately, accuracy: %d m", req_accuracy);

		evt.modem_location.updates_requested = true;

		/* Send location update to modem immediately to avoid delays, if the current
		 * location is available.
		 */
		k_work_reschedule_for_queue(&ntn_workq,	&modem_location_update_work, K_NO_WAIT);
	}

	if (next_in_s > 0) {
		if (next_req_accuracy > 0 && req_accuracy == 0) {
			/* Location is requested later. The event is sent before the requested time
			 * to allow enough time for GNSS to get a fix. Modem needs to have
			 * the current location available at the given time.
			 */
			next_in_s = next_in_s - CONFIG_NTN_MODEM_LOCATION_REQ_MARGIN;
		}

		k_work_reschedule_for_queue(
			&ntn_workq,
			&req_accuracy_update_work,
			K_SECONDS(next_in_s));

		LOG_INF("Location updates required in %d s, accuracy: %d m",
			next_in_s, next_req_accuracy);
	} else {
		k_work_cancel_delayable(&req_accuracy_update_work);
	}

	if (prev_updates_requested != evt.modem_location.updates_requested) {
		event_dispatch(&evt);

		prev_updates_requested = evt.modem_location.updates_requested;
	}
}

#if defined(CONFIG_UNITY)
void ntn_on_modem_cfun(int mode, void *ctx)
#else
NRF_MODEM_LIB_ON_CFUN(ntn_cfun_hook, ntn_on_modem_cfun, NULL);

static void ntn_on_modem_cfun(int mode, void *ctx)
#endif /* CONFIG_UNITY */
{
	int err;

	ARG_UNUSED(ctx);

	if (mode == LTE_LC_FUNC_MODE_NORMAL || mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		/* Subscribe to %LOCATION notifications. */
		err = nrf_modem_at_printf(AT_LOCATION_SUBSCRIBE);
		if (err) {
			LOG_WRN("Enabling location notifications failed, error: %d", err);
			LOG_WRN("Modem firmware with NTN support required");
		}
	}
}

void ntn_register_handler(ntn_evt_handler_t handler)
{
	event_handler = handler;
}

void ntn_modem_location_update(double latitude, double longitude, float altitude, float speed)
{
	double distance;

	current_location.timestamp = k_uptime_get();
	current_location.latitude = latitude;
	current_location.longitude = longitude;
	current_location.altitude = altitude;
	current_location.speed = speed;

	if (req_accuracy == 0) {
		/* Location updates not requested. */
		LOG_DBG("Location updates not requested");
		return;
	}

	distance = distance_calculate(&modem_location, &current_location);
	LOG_DBG("Location update, distance: %.1f m", distance);

	/* Check if the current location is still accurate enough. Use a 50 meter threshold to
	 * ensure that location gets updated before the old location gets too inaccurate.
	 */
	if (distance < (req_accuracy - 50)) {
		LOG_DBG("Location update not needed, within accuracy");
		return;
	}

	k_work_reschedule_for_queue(&ntn_workq, &modem_location_update_work, K_NO_WAIT);
}

int ntn_modem_location_set(double latitude, double longitude, float altitude, uint32_t validity)
{
	int err;

	err = nrf_modem_at_printf(
		AT_LOCATION_UPDATE,
		latitude,
		longitude,
		(double)altitude,
		0, /* accuracy is set to 0 meters which means it's always accurate enough */
		validity);
	if (err) {
		LOG_ERR("Failed to set location to modem, error: %d", err);
		return -EFAULT;
	}

	return 0;
}

int ntn_modem_location_invalidate(void)
{
	int err;

	err = nrf_modem_at_printf(AT_LOCATION_INVALIDATE);
	if (err) {
		LOG_ERR("Failed to invalidate location, error: %d", err);
		return -EFAULT;
	}

	return 0;
}

static int ntn_init(void)
{
	/* Initialize work queue. */
	k_work_queue_init(&ntn_workq);
	k_work_queue_start(&ntn_workq, ntn_workq_stack,
			   K_THREAD_STACK_SIZEOF(ntn_workq_stack),
			   CONFIG_NTN_WORKQUEUE_PRIORITY, NULL);

	return 0;
}

SYS_INIT(ntn_init, APPLICATION, 0);
