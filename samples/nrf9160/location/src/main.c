/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/location.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <date_time.h>
#endif /* CONFIG_NRF_CLOUD_PGPS */

K_SEM_DEFINE(location_event, 0, 1);

static void antenna_configure(void)
{
	int err;

	if (strlen(CONFIG_LOCATION_SAMPLE_GNSS_AT_MAGPIO) > 0) {
		err = nrf_modem_at_printf("%s", CONFIG_LOCATION_SAMPLE_GNSS_AT_MAGPIO);
		if (err) {
			printk("Failed to set MAGPIO configuration\n");
		}
	}

	if (strlen(CONFIG_LOCATION_SAMPLE_GNSS_AT_COEX0) > 0) {
		err = nrf_modem_at_printf("%s", CONFIG_LOCATION_SAMPLE_GNSS_AT_COEX0);
		if (err) {
			printk("Failed to set COEX0 configuration\n");
		}
	}
}

#if defined(CONFIG_NRF_CLOUD_PGPS)
K_SEM_DEFINE(time_update_finished, 0, 1);

static void date_time_evt_handler(const struct date_time_evt *evt)
{
	/* Don't care about the event, just indicate that we got it. */
	k_sem_give(&time_update_finished);
}

static int current_time_get(void)
{
	int err;

	/* If we wait for a while after LTE attach, it's more likely that modem has got the current
	 * time from the LTE network and no NTP is needed.
	 */
	k_sleep(K_SECONDS(1));

	err = date_time_update_async(date_time_evt_handler);
	if (err) {
		printk("Requesting the current time failed, error: %d\n", err);
		return -1;
	}

	k_sem_take(&time_update_finished, K_FOREVER);

	if (date_time_is_valid() == false) {
		printk("Failed to get current time");
		return -1;
	}

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

static void location_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		printk("Got location:\n");
		printk("  method: %s\n", location_method_str(event_data->location.method));
		printk("  latitude: %.06f\n", event_data->location.latitude);
		printk("  longitude: %.06f\n", event_data->location.longitude);
		printk("  accuracy: %.01f m\n", event_data->location.accuracy);
		if (event_data->location.datetime.valid) {
			printk("  date: %04d-%02d-%02d\n",
				event_data->location.datetime.year,
				event_data->location.datetime.month,
				event_data->location.datetime.day);
			printk("  time: %02d:%02d:%02d.%03d UTC\n",
				event_data->location.datetime.hour,
				event_data->location.datetime.minute,
				event_data->location.datetime.second,
				event_data->location.datetime.ms);
		}
		printk("  Google maps URL: https://maps.google.com/?q=%.06f,%.06f\n\n",
			event_data->location.latitude, event_data->location.longitude);
		break;

	case LOCATION_EVT_TIMEOUT:
		printk("Getting location timed out\n\n");
		break;

	case LOCATION_EVT_ERROR:
		printk("Getting location failed\n\n");
		break;

	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		printk("Getting location assistance requested. Not doing anything.\n\n");
		break;

	default:
		printk("Getting location: Unknown event\n\n");
		break;
	}

	k_sem_give(&location_event);
}

static void location_event_wait(void)
{
	k_sem_take(&location_event, K_FOREVER);
}

/**
 * @brief Retrieve location so that fallback is applied.
 *
 * @details This is achieved by setting GNSS as first priority method and giving it too short
 * timeout. Then a fallback to next method, which is cellular in this example, occurs.
 */
static void location_with_fallback_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {LOCATION_METHOD_GNSS, LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, sizeof(methods), methods);
	/* GNSS timeout is set to 1 second to force a failure. */
	config.methods[0].gnss.timeout = 1;
	/* Default cellular configuration may be overridden here. */
	config.methods[1].cellular.timeout = 30;

	printk("Requesting location with short GNSS timeout to trigger fallback to cellular...\n");

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return;
	}

	location_event_wait();
}

/**
 * @brief Retrieve location with default configuration.
 *
 * @details This is achieved by not passing configuration at all to location_request().
 */
static void location_default_get(void)
{
	int err;

	printk("Requesting location with the default configuration...\n");

	err = location_request(NULL);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return;
	}

	location_event_wait();
}

/**
 * @brief Retrieve location with GNSS low accuracy.
 */
static void location_gnss_low_accuracy_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, sizeof(methods), methods);
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_LOW;

	printk("Requesting low accuracy GNSS location...\n");

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return;
	}

	location_event_wait();
}

#if defined(CONFIG_LOCATION_METHOD_WIFI)
/**
 * @brief Retrieve location with Wi-Fi positioning as first priority, GNSS as second
 * and cellular as third.
 */
static void location_wifi_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {
		LOCATION_METHOD_WIFI,
		LOCATION_METHOD_GNSS,
		LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, sizeof(methods), methods);

	printk("Requesting Wi-Fi location with GNSS and cellular fallback...\n");

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return;
	}

	location_event_wait();
}
#endif

/**
 * @brief Retrieve location periodically with GNSS as first priority and cellular as second.
 */
static void location_gnss_periodic_get(void)
{
	int err;
	struct location_config config;
	enum location_method methods[] = {LOCATION_METHOD_GNSS, LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, sizeof(methods), methods);
	config.interval = 30;

	printk("Requesting 30s periodic GNSS location with cellular fallback...\n");

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, error: %d\n", err);
		return;
	}
}

int main(void)
{
	int err;

	printk("Location sample started\n\n");

	antenna_configure();

	printk("Connecting to LTE...\n");

	lte_lc_init();
	/* Enable PSM. */
	lte_lc_psm_req(true);
	lte_lc_connect();

	printk("Connected to LTE\n\n");

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* P-GPS needs to know the current time. */
	err = current_time_get();
	if (err) {
		printk("Getting current time for P-GPS use failed");
		return -1;
	}
#endif /* CONFIG_NRF_CLOUD_PGPS */

	err = location_init(location_event_handler);
	if (err) {
		printk("Initializing the Location library failed, error: %d\n", err);
		return -1;
	}

	/* The fallback case is run first, otherwise GNSS might get a fix even with a 1 second
	 * timeout.
	 */
	location_with_fallback_get();

	location_default_get();

	location_gnss_low_accuracy_get();

#if defined(CONFIG_LOCATION_METHOD_WIFI)
	location_wifi_get();
#endif

	location_gnss_periodic_get();

	return 0;
}
