/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "include/music_led_sync.h"

#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include "dsp/transform_functions.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(music_led_sync, CONFIG_LOG_MUSIC_LED_SYNC_LEVEL);

static const struct pwm_dt_spec red_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

static struct k_thread music_led_sync_thread_data;
static k_tid_t music_led_sync_id;

K_THREAD_STACK_DEFINE(music_led_sync_thread_stack, CONFIG_MUSIC_LED_SYNC_STACK_SIZE);

K_SEM_DEFINE(music_led_sync_timer_sem, 0, 1);
K_SEM_DEFINE(music_led_sync_value_sem, 0, 1);

char pcm_data_stereo[CONFIG_PCM_NUM_BYTES_STEREO];

extern void my_expiry_function(struct k_timer *timer_id)
{
	k_sem_give(&music_led_sync_timer_sem);
}

K_TIMER_DEFINE(music_led_sync_timer, my_expiry_function, NULL);

static void music_led_sync_fft_analyse(q15_t *fft_data, uint16_t len)
{
	int ret;
	uint32_t frequency_modules[3] = { 0 };
	uint32_t red_pwm_led_duty_cycle, green_pwm_led_duty_cycle, blue_pwm_led_duty_cycle;
	uint32_t rgb_led_period = red_pwm_led.period;

	/* Assigning FFT bins into modules */
	for (int i = 0; i < len; i++) {
		/* Checks only every second element of the array (real part),
		 * as every other element in the array is the belonging imaginary part.
		 */
		if (fft_data[(i + 1) * 2] > 0) {
			/* Upscaling of values according to CMSIS doc */
			if (i < CONFIG_RGB_RED_FREQ_BIN_RANGE) {
				frequency_modules[0] += (fft_data[i] << 7);
			} else if (i < CONFIG_RGB_GREEN_FREQ_BIN_RANGE) {
				frequency_modules[1] += (fft_data[i] << 7);
			} else if (i < CONFIG_RGB_BLUE_FREQ_BIN_RANGE) {
				frequency_modules[2] += (fft_data[i] << 7);
			}
		}
	}

	/* Reducing intensity of less dominant modules*/
	if ((frequency_modules[0] > frequency_modules[1]) &&
	    (frequency_modules[0] > frequency_modules[2])) {
		frequency_modules[0] *= 1.5;
		frequency_modules[1] *= 0.5;
		frequency_modules[2] *= 0.5;
	} else if ((frequency_modules[1] > frequency_modules[0]) &&
		   (frequency_modules[1] > frequency_modules[2])) {
		frequency_modules[0] *= 0.5;
		frequency_modules[1] *= 1.5;
		frequency_modules[2] *= 0.5;
	} else if ((frequency_modules[2] > frequency_modules[0]) &&
		   frequency_modules[2] > frequency_modules[1]) {
		frequency_modules[0] *= 0.5;
		frequency_modules[1] *= 0.5;
		frequency_modules[2] *= 1.5;
	}

	/* Calculating duty cycle values, +1 added to prevent division by 0 */
	uint32_t total_modules_val =
		frequency_modules[0] + frequency_modules[1] + frequency_modules[2] + 1;

	red_pwm_led_duty_cycle = (rgb_led_period / total_modules_val * frequency_modules[0]);
	green_pwm_led_duty_cycle = (rgb_led_period / total_modules_val * frequency_modules[1]);
	blue_pwm_led_duty_cycle = (rgb_led_period / total_modules_val * frequency_modules[2]);

	ret = pwm_set_pulse_dt(&red_pwm_led, red_pwm_led_duty_cycle);
	if (ret) {
		LOG_DBG("Failed to write to red RGB, ret: %d", ret);
	}

	ret = pwm_set_pulse_dt(&green_pwm_led, green_pwm_led_duty_cycle);
	if (ret) {
		LOG_DBG("Failed to write to green RGB, ret: %d", ret);
	}

	ret = pwm_set_pulse_dt(&blue_pwm_led, blue_pwm_led_duty_cycle);
	if (ret) {
		LOG_DBG("Failed to write to blue RGB, ret: %d", ret);
	}
}

static int music_led_sync_thread(void *dummy1, void *dummy2, void *dummy3)
{
	arm_rfft_instance_q15 fft_instance;
	arm_status status;
	/* Output has to be twice the size of sample size */
	q15_t output[CONFIG_FFT_SAMPLE_SIZE * 2];

	status = arm_rfft_init_q15(&fft_instance, CONFIG_FFT_SAMPLE_SIZE,
				   CONFIG_FFT_FLAG_INVERSE_TRANSFORM, CONFIG_FFT_FLAG_BIT_REVERSAL);
	if (status != ARM_MATH_SUCCESS) {
		LOG_ERR("Failed to init FFT instance - (err: %d)", status);
		return status;
	}

	while (1) {
		k_sem_take(&music_led_sync_timer_sem, K_FOREVER);
		k_sem_take(&music_led_sync_value_sem, K_FOREVER);
		arm_rfft_q15(&fft_instance, (q15_t *)pcm_data_stereo, output);
		k_sem_give(&music_led_sync_value_sem);

		arm_abs_q15(output, output, (CONFIG_FFT_SAMPLE_SIZE * 2));
		music_led_sync_fft_analyse(output, CONFIG_FFT_SAMPLE_SIZE);
	}
}

/**
 * @brief Initialize the music color synchronization
 *
 * @return 0 on success, error otherwise
 */

static int music_led_sync_init(const struct device *unused)
{
	int ret;

	if (!device_is_ready(red_pwm_led.dev) || !device_is_ready(green_pwm_led.dev) ||
	    !device_is_ready(blue_pwm_led.dev)) {
		LOG_WRN("Error: one or more PWM devices not ready");
		return -ENXIO;
	}

	music_led_sync_id =
		k_thread_create(&music_led_sync_thread_data, music_led_sync_thread_stack,
				CONFIG_MUSIC_LED_SYNC_STACK_SIZE,
				(k_thread_entry_t)music_led_sync_thread, NULL, NULL, NULL,
				K_PRIO_PREEMPT(CONFIG_MUSIC_LED_SYNC_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(music_led_sync_id, "MUSIC LED SYNC");
	if (ret) {
		LOG_ERR("Failed to name thread - (err: %d)", ret);
		return ret;
	}

	k_timer_start(&music_led_sync_timer, K_MSEC(150), K_MSEC(150));

	LOG_DBG("Music led synchronization initialized");

	return 0;
}

int music_led_sync_data_update(char const *const temp_pcm_data_stereo, uint16_t len)
{
	if (k_sem_take(&music_led_sync_value_sem, K_NO_WAIT)) {
		memcpy(pcm_data_stereo, temp_pcm_data_stereo, len);
		k_sem_give(&music_led_sync_value_sem);
		return 0;
	} else {
		return -EBUSY;
	}
}

void music_led_sync_play(void)
{
	k_thread_resume(music_led_sync_id);
}

int music_led_sync_pause(void)
{
	int ret;

	k_thread_suspend(music_led_sync_id);
	ret = pwm_set_pulse_dt(&red_pwm_led, 0);
	if (ret) {
		LOG_DBG("Failed to write to red RGB, ret: %d", ret);
		return ret;
	}

	ret = pwm_set_pulse_dt(&green_pwm_led, 0);
	if (ret) {
		LOG_DBG("Failed to write to green RGB, ret: %d", ret);
		return ret;
	}

	ret = pwm_set_pulse_dt(&blue_pwm_led, 0);
	if (ret) {
		LOG_DBG("Failed to write to blue RGB, ret: %d", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(music_led_sync_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
