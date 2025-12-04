/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_monitor.h>
#include <modem/at_parser.h>
#include <modem/lte_lc.h>
#include <modem/ntn.h>

LOG_MODULE_REGISTER(ntn, CONFIG_NTN_LOG_LEVEL);

/* %LOCATION notification parameter indexes. */
#define AT_LOCATION_ACCURACY_INDEX			1
#define AT_LOCATION_NEXT_ACCURACY_INDEX			2
#define AT_LOCATION_NEXT_REQ_ACCURACY_IN_S_INDEX	3

/* Accuracy requested by the modem. */
static uint32_t req_accuracy;
/* Next accuracy requested by the modem. */
static uint32_t next_req_accuracy;

/* Previous sent NTN_EVT_LOCATION_REQUEST event. */
static struct ntn_evt prev_location_request_evt;

/* Event handler registered by the application. */
static ntn_evt_handler_t event_handler;

/* Work queue for NTN library. */
static K_THREAD_STACK_DEFINE(ntn_workq_stack, CONFIG_NTN_WORKQUEUE_STACK_SIZE);
static struct k_work_q ntn_workq;

AT_MONITOR(ntn_atmon_location, "%LOCATION", at_handler_location);

static void req_accuracy_update_work_fn(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(req_accuracy_update_work, req_accuracy_update_work_fn);

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

	err = at_parser_num_get(&parser, AT_LOCATION_NEXT_REQ_ACCURACY_IN_S_INDEX, next_in_s);
	if (err && err != -EIO) {
		LOG_ERR("Failed to parse location notification, error: %d", err);
		return err;
	}

	return 0;
}

static void event_dispatch(const struct ntn_evt *evt)
{
	if (event_handler != NULL) {
		event_handler(evt);
	}
}

static bool location_request_evt_changed(struct ntn_location_request *evt)
{
	if (evt->requested == prev_location_request_evt.location_request.requested &&
	    evt->accuracy == prev_location_request_evt.location_request.accuracy) {
		return false;
	} else {
		return true;
	}
}

static void req_accuracy_update_work_fn(struct k_work *work)
{
	struct ntn_evt evt = {
		.type = NTN_EVT_LOCATION_REQUEST,
		.location_request.requested = false
	};

	LOG_DBG("Updating requested accuracy from %d m to %d m", req_accuracy, next_req_accuracy);

	req_accuracy = next_req_accuracy;
	next_req_accuracy = 0;

	if (req_accuracy == 0) {
		LOG_INF("Location updates not requested");
	} else {
		LOG_INF("Location updates requested, accuracy: %d m", req_accuracy);

		evt.location_request.requested = true;
		evt.location_request.accuracy = req_accuracy;
	}

	if (location_request_evt_changed(&evt.location_request)) {
		event_dispatch(&evt);

		prev_location_request_evt = evt;
	}
}

static void at_handler_location(const char *notif)
{
	int err;
	uint32_t next_req_acc_in_s;
	struct ntn_evt evt = {
		.type = NTN_EVT_LOCATION_REQUEST,
		.location_request.requested = false
	};

	err = parse_location_notif(notif, &req_accuracy, &next_req_accuracy, &next_req_acc_in_s);
	if (err) {
		return;
	}

	/* If the time until next accuracy is shorter or equal to the configured advance,
	 * take the next requested accuracy into use immediately.
	 */
	if (next_req_acc_in_s != 0 && next_req_acc_in_s <= CONFIG_NTN_LOCATION_REQUEST_ADVANCE) {
		next_req_acc_in_s = 0;
		req_accuracy = next_req_accuracy;
	}

	if (req_accuracy == 0) {
		LOG_INF("Location updates not requested immediately");
	} else {
		LOG_INF("Location updates requested immediately, accuracy: %d m", req_accuracy);

		evt.location_request.requested = true;
		evt.location_request.accuracy = req_accuracy;
	}

	if (next_req_acc_in_s > 0) {
		if (next_req_accuracy > 0 && req_accuracy == 0) {
			/* Location is requested later. The event is sent before the requested time
			 * to allow enough time for GNSS to get a fix. Modem needs to have
			 * the current location available at the given time.
			 */
			next_req_acc_in_s =
				next_req_acc_in_s - CONFIG_NTN_LOCATION_REQUEST_ADVANCE;
		}

		k_work_reschedule_for_queue(
			&ntn_workq,
			&req_accuracy_update_work,
			K_SECONDS(next_req_acc_in_s));

		LOG_INF("Location updates requested in %d s, accuracy: %d m",
			next_req_acc_in_s, next_req_accuracy);
	} else {
		k_work_cancel_delayable(&req_accuracy_update_work);
	}

	if (location_request_evt_changed(&evt.location_request)) {
		event_dispatch(&evt);

		prev_location_request_evt = evt;
	}
}

static void location_ntf_subscribe(void)
{
	/* Error is ignored because this basically only fails when the firmware does not have
	 * NTN support. There are applications which support both NTN and non-NTN modem
	 * firmware, so we don't want to log an error here with non-NTN firmware.
	 */
	(void)nrf_modem_at_printf("AT%%LOCATION=1");
}

#if defined(CONFIG_UNITY)
void ntn_on_modem_init(int err, void *ctx)
#else
NRF_MODEM_LIB_ON_INIT(ntn_init_hook, ntn_on_modem_init, NULL);

static void ntn_on_modem_init(int err, void *ctx)
#endif
{
	ARG_UNUSED(ctx);

	if (err) {
		return;
	}

	location_ntf_subscribe();
}

#if defined(CONFIG_UNITY)
void ntn_on_modem_cfun(int mode, void *ctx)
#else
NRF_MODEM_LIB_ON_CFUN(ntn_cfun_hook, ntn_on_modem_cfun, NULL);

static void ntn_on_modem_cfun(int mode, void *ctx)
#endif /* CONFIG_UNITY */
{
	ARG_UNUSED(ctx);

	if (mode == LTE_LC_FUNC_MODE_POWER_OFF) {
		location_ntf_subscribe();
	}
}

void ntn_register_handler(ntn_evt_handler_t handler)
{
	event_handler = handler;
}

int ntn_location_set(double latitude, double longitude, float altitude, uint32_t validity)
{
	int err;

	err = nrf_modem_at_printf(
		"AT%%LOCATION=2,\"%.6f\",\"%.6f\",\"%.1f\",%d,%d",
		latitude,
		longitude,
		(double)altitude,
		req_accuracy, /* accuracy should be the same as requested by the modem */
		validity);
	if (err) {
		LOG_ERR("Failed to set location to modem, error: %d", err);
		return -EFAULT;
	}

	LOG_DBG("lat: %.6f, lon: %.6f, alt: %.1f m, validity: %d s",
		latitude, longitude, (double)altitude, validity);

	return 0;
}

int ntn_location_invalidate(void)
{
	int err;

	err = nrf_modem_at_printf("AT%%LOCATION=2");
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
