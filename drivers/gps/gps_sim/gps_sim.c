/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <gpio.h>
#include <drivers/gps.h>
#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <logging/log.h>
#include <math.h>

#define BASE_GPS_SAMPLE_HOUR	(CONFIG_GPS_SIM_BASE_TIMESTAMP / 10000)
#define BASE_GPS_SAMPLE_MINUTE	((CONFIG_GPS_SIM_BASE_TIMESTAMP / 100) % 100)
#define BASE_GPS_SAMPLE_SECOND	(CONFIG_GPS_SIM_BASE_TIMESTAMP % 100)
#define BASE_GSP_SAMPLE_LAT	(CONFIG_GPS_SIM_BASE_LATITUDE / 1000.0)
#define BASE_GSP_SAMPLE_LNG	(CONFIG_GPS_SIM_BASE_LONGITUDE / 1000.0)

/* Whole NMEA sentence, including CRC. */
#define GPS_NMEA_SENTENCE "$GPGGA,%02d%02d%02d.200,%8.3f,%c,%09.3f,%c,1,"      \
			  "12,1.0,0.0,M,0.0,M,,*%02X"

LOG_MODULE_REGISTER(gps_sim, CONFIG_GPS_SIM_LOG_LEVEL);

struct gps_sim_data {
#if defined(CONFIG_GPS_SIM_TRIGGER)
	struct device *gpio;
	const char *gpio_port;
	u8_t gpio_pin;
	struct gpio_callback gpio_cb;
	struct k_sem gpio_sem;

	gps_trigger_handler_t drdy_handler;
	struct gps_trigger drdy_trigger;

	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_GPS_SIM_THREAD_STACK_SIZE);
	struct k_thread thread;
#endif /* CONFIG_GPS_SIM_TRIGGER */
};

static struct gps_data gps_sample;
static struct k_mutex trigger_mutex;

/**
 * @brief Callback for GPIO when using button as trigger.
 *
 * @param dev Pointer to device structure.
 * @param cb Pointer to GPIO callback structure.
 * @param pins Pin mask for callback.
 */
static void gps_sim_gpio_callback(struct device *dev, struct gpio_callback *cb,
				  u32_t pins)
{
	ARG_UNUSED(pins);
	struct gps_sim_data *drv_data =
		CONTAINER_OF(cb, struct gps_sim_data, gpio_cb);

	gpio_pin_disable_callback(dev, drv_data->gpio_pin);
	k_sem_give(&drv_data->gpio_sem);
}

/**
 * @brief Function that runs in the GPS simulator thread when using trigger.
 *
 * @param dev_ptr Pointer to GPS simulator device.
 */
static void gps_sim_thread(int dev_ptr)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct gps_sim_data *drv_data = dev->driver_data;

	while (true) {
		if (IS_ENABLED(CONFIG_GPS_SIM_TRIGGER_USE_TIMER)) {
			k_sleep(CONFIG_GPS_SIM_TRIGGER_TIMER_MSEC);
		} else if (IS_ENABLED(CONFIG_GPS_SIM_TRIGGER_USE_BUTTON)) {
			k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		}

		k_mutex_lock(&trigger_mutex, K_FOREVER);
		if (drv_data->drdy_handler != NULL) {
			drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
		}
		k_mutex_unlock(&trigger_mutex);

		if (IS_ENABLED(CONFIG_GPS_SIM_TRIGGER_USE_BUTTON)) {
			gpio_pin_enable_callback(drv_data->gpio,
						 drv_data->gpio_pin);
		}
	}
}

/**
 * @brief Initializing interrupt when simulator uses trigger
 *
 * @param dev Pointer to device instance.
 */
static int gps_sim_init_thread(struct device *dev)
{
	struct gps_sim_data *drv_data = dev->driver_data;

	k_mutex_init(&trigger_mutex);

	if (IS_ENABLED(CONFIG_GPS_SIM_TRIGGER_USE_BUTTON)) {
		drv_data->gpio = device_get_binding(drv_data->gpio_port);
		if (drv_data->gpio == NULL) {
			LOG_ERR("Failed to get pointer to %s device",
				drv_data->gpio_port);
			return -EINVAL;
		}

		gpio_pin_configure(drv_data->gpio, drv_data->gpio_pin,
				   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
					   GPIO_INT_ACTIVE_LOW |
					   GPIO_PUD_PULL_UP);

		gpio_init_callback(&drv_data->gpio_cb, gps_sim_gpio_callback,
				   BIT(drv_data->gpio_pin));

		if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
			LOG_ERR("Failed to set GPIO callback");
			return -EIO;
		}

		k_sem_init(&drv_data->gpio_sem, 0, 1);
	}

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_GPS_SIM_THREAD_STACK_SIZE,
			(k_thread_entry_t)gps_sim_thread, dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_GPS_SIM_THREAD_PRIORITY), 0, 0);

	return 0;
}

static int gps_sim_trigger_set(struct device *dev,
			       const struct gps_trigger *trig,
			       gps_trigger_handler_t handler)
{
	int ret = 0;
	struct gps_sim_data *drv_data = dev->driver_data;

	switch (trig->type) {
	case GPS_TRIG_DATA_READY:
		k_mutex_lock(&trigger_mutex, K_FOREVER);
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = *trig;

		if (IS_ENABLED(CONFIG_GPS_SIM_TRIGGER_USE_BUTTON)) {
			if (handler) {
				gpio_pin_enable_callback(drv_data->gpio,
							 drv_data->gpio_pin);
			} else {
				gpio_pin_disable_callback(drv_data->gpio,
							  drv_data->gpio_pin);
			}
		}

		k_mutex_unlock(&trigger_mutex);
		break;
	default:
		LOG_ERR("Unsupported GPS trigger");
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

/**
 * @brief Calculates NMEA sentence checksum
 *
 * @param gps_data Pointer to NMEA sentence.
 */
static u8_t nmea_checksum_get(const u8_t *gps_data)
{
	const u8_t *i = gps_data + 1;
	u8_t checksum = 0;

	while (*i != '*') {
		if ((i - gps_data) > GPS_NMEA_SENTENCE_MAX_LENGTH) {
			return 0;
		}
		checksum ^= *i;
		i++;
	}

	return checksum;
}

/**
 * @brief Calculates sine from uptime
 * The input to the sin() function is limited to avoid overflow issues
 *
 * @param offset Offset for the sine.
 * @param amplitude Amplitude of sine.
 */
static double generate_sine(double offset, double amplitude)
{
	u32_t time = k_uptime_get_32() / K_MSEC(CONFIG_GPS_SIM_FIX_TIME);

	return offset + amplitude * sin(time % UINT16_MAX);
}

/**
 * @brief Calculates cosine from uptime
 * The input to the sin() function is limited to avoid overflow issues.
 *
 * @param offset Offset for the cosine.
 * @param amplitude Amplitude of cosine.
 */
static double generate_cosine(double offset, double amplitude)
{
	u32_t time = k_uptime_get_32() / K_MSEC(CONFIG_GPS_SIM_FIX_TIME);

	return offset + amplitude * cos(time % UINT16_MAX);
}

/**
 * @brief Generates a pseudo-random number between -1 and 1.
 */
static double generate_pseudo_random(void)
{
	return (double)rand() / ((double)RAND_MAX / 2.0) - 1.0;
}

/**
 * @brief Function generatig GPS data
 *
 * @param gps_data Pointer to gpssim_nmea struct where the NMEA
 * sentence will be stored.
 * @param max_variation The maximum value the latitude and longitude in the
 * generated sentence can vary for each iteration. In units of minutes.
 */
static void generate_gps_data(struct gps_data *gps_data,
			      double max_variation)
{
	static u8_t hour = BASE_GPS_SAMPLE_HOUR;
	static u8_t minute = BASE_GPS_SAMPLE_MINUTE;
	static u8_t second = BASE_GPS_SAMPLE_SECOND;
	static u32_t last_uptime;
	double lat = BASE_GSP_SAMPLE_LAT;
	double lng = BASE_GSP_SAMPLE_LNG;
	char lat_heading = 'N';
	char lng_heading = 'E';

	if (IS_ENABLED(CONFIG_GPS_SIM_ELLIPSOID)) {
		lat = generate_sine(BASE_GSP_SAMPLE_LAT, max_variation / 2.0);
		lng = generate_cosine(BASE_GSP_SAMPLE_LNG, max_variation);
	}

	if (IS_ENABLED(CONFIG_GPS_SIM_PSEUDO_RANDOM)) {
		static double acc_lat;
		static double acc_lng;

		acc_lat += max_variation * generate_pseudo_random();
		acc_lng += max_variation * generate_pseudo_random();
		lat = BASE_GSP_SAMPLE_LAT + acc_lat;
		lng = BASE_GSP_SAMPLE_LNG + acc_lng;
	}

	u32_t uptime = k_uptime_get_32() / MSEC_PER_SEC;

	second += (uptime - last_uptime);
	last_uptime = uptime;

	if (second > 59) {
		second = second % 60;
		minute += 1;
		if (minute > 59) {
			minute = minute % 60;
			hour += 1;
			hour = hour % 24;
		}
	}

	if (lat < 0) {
		lat *= -1.0;
		lat_heading = 'S';
	}

	if (lng < 0) {
		lng *= -1.0;
		lng_heading = 'W';
	}

	/* Format the sentence, excluding the CRC. */
	snprintf(gps_data->nmea.buf,
		 GPS_NMEA_SENTENCE_MAX_LENGTH, GPS_NMEA_SENTENCE,
		 hour, minute, second, lat, lat_heading, lng, lng_heading, 0);

	/* Calculate the CRC (stop when '*' is found, thus excluding the CRC),
	 * then reformat the string, this time including the CRC.
	 *
	 * An alternative way to do this would have been to allocate a small
	 * buffer on the stack, format the CRC into it, and strcat it with
	 * the NMEA sentence.
	 */

	u8_t checksum = nmea_checksum_get(gps_data->nmea.buf);

	gps_data->nmea.len =
		snprintf(gps_data->nmea.buf, GPS_NMEA_SENTENCE_MAX_LENGTH,
			 GPS_NMEA_SENTENCE, hour, minute, second, lat,
			 lat_heading, lng, lng_heading, checksum);

	LOG_DBG("%s (%d bytes)", gps_data->nmea.buf, gps_data->nmea.len);
}

static int gps_sim_init(struct device *dev)
{
	if (IS_ENABLED(CONFIG_GPS_SIM_TRIGGER)) {
#ifdef CONFIG_GPS_SIM_TRIGGER_USE_BUTTON
		struct gps_sim_data *drv_data = dev->driver_data;

		drv_data->gpio_port = CONFIG_GPS_SIM_GPIO_CONTROLLER;
		drv_data->gpio_pin = CONFIG_GPS_SIM_GPIO_PIN;
#endif /* GPS_SIM_TRIGGER_USE_BUTTON */

		/* Initialize random seed. */
		srand(k_cycle_get_32());

		if (gps_sim_init_thread(dev) < 0) {
			LOG_ERR("Failed to initialize trigger interrupt");
			return -EIO;
		}
	}

	return 0;
}

/**
 * @brief Generates simulated GPS data for a channel.
 *
 * @param chan Channel to generate data for.
 */
static int gps_sim_generate_data(enum gps_channel chan)
{
	switch (chan) {
	case GPS_CHAN_NMEA:
		generate_gps_data(&gps_sample,
				  CONFIG_GPS_SIM_MAX_STEP / 1000.0);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gps_sim_sample_fetch(struct device *dev)
{
	return gps_sim_generate_data(GPS_CHAN_NMEA);
}

static int gps_sim_channel_get(struct device *dev, enum gps_channel chan,
			       struct gps_data *sample)
{
	switch (chan) {
	case GPS_CHAN_NMEA:
		memcpy(sample->nmea.buf, gps_sample.nmea.buf,
		       gps_sample.nmea.len);
		sample->nmea.len = gps_sample.nmea.len;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct gps_sim_data gps_sim_data;

static const struct gps_driver_api gps_sim_api_funcs = {
	.sample_fetch = gps_sim_sample_fetch,
	.channel_get = gps_sim_channel_get,
#if defined(CONFIG_GPS_SIM_TRIGGER)
	.trigger_set = gps_sim_trigger_set
#endif
};

/* TODO: Remove this when the GPS API has Kconfig that sets the priority */
#ifdef CONFIG_GPS_INIT_PRIORITY
#define GPS_INIT_PRIORITY CONFIG_GPS_INIT_PRIORITY
#else
#define GPS_INIT_PRIORITY 90
#endif

DEVICE_AND_API_INIT(gps_sim, CONFIG_GPS_SIM_DEV_NAME, gps_sim_init,
		    &gps_sim_data, NULL, POST_KERNEL, GPS_INIT_PRIORITY,
		    &gps_sim_api_funcs);
