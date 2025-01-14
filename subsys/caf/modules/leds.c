/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/pm/device.h>

#include <caf/events/power_event.h>
#include <caf/events/led_event.h>

#define MODULE leds
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_LEDS_LOG_LEVEL);

#define LED_ID(led) ((led) - &leds[0])

#define TIME_INVALID -1

#define MS_PER_FRAME (1000 / CONFIG_CAF_LEDS_FRAMES_PER_SECOND)

static struct k_work_delayable led_update_work;
static int64_t next_update = TIME_INVALID;

typedef uint8_t (*led_effect_easing_func_t)(uint8_t i, union led_effect_easing_param const *param);

struct led {
	const struct device *dev;
	uint8_t color_count;

	struct led_color color;
	const struct led_effect *effect;
	uint16_t repeat_count;
	uint16_t effect_step;
	struct led_color effect_step_start_color;
	int16_t effect_step_color_diff[_CAF_LED_COLOR_CHANNEL_COUNT];
	int64_t effect_step_start_time;
	led_effect_easing_func_t easing;
};

#ifdef CONFIG_CAF_LEDS_PWM
  #define DT_DRV_COMPAT pwm_leds
#elif defined(CONFIG_CAF_LEDS_GPIO)
  #define DT_DRV_COMPAT gpio_leds
#else
  #error "LED driver must be specified in configuration"
#endif

#define _LED_COLOR_ID(_unused) 0,

#define _LED_COLOR_COUNT(id)					\
	static const uint8_t led_colors_##id[] = {		\
		DT_INST_FOREACH_CHILD(id, _LED_COLOR_ID)	\
	};

DT_INST_FOREACH_STATUS_OKAY(_LED_COLOR_COUNT)

#define _LED_INSTANCE_DEF(id)						\
	{								\
		.dev = DEVICE_DT_GET(DT_DRV_INST(id)),			\
		.color_count = ARRAY_SIZE(led_colors_##id),		\
	},


static struct led leds[] = {
	DT_INST_FOREACH_STATUS_OKAY(_LED_INSTANCE_DEF)
};
BUILD_ASSERT(ARRAY_SIZE(leds) <= 32, "Too many LEDs");
static uint32_t leds_running;

static void led_stop(struct led *led);

// easing functions, copied from FastLED

static uint8_t scale8(uint8_t i, uint8_t scale) {
    return (((uint16_t)i) * (1 + (uint16_t)(scale))) >> 8;
}

static uint8_t ease8InOutQuad( uint8_t i, union led_effect_easing_param const *param)
{
    uint8_t j = i;
    if( j & 0x80 ) {
        j = 255 - j;
    }
    uint8_t jj  = scale8(  j, j);
    uint8_t jj2 = jj << 1;
    if( i & 0x80 ) {
        jj2 = 255 - jj2;
    }
    return jj2;
}
 
static uint8_t ease8InOutCubic( uint8_t i, union led_effect_easing_param const *param)
{
    uint8_t ii  = scale8(  i, i);
    uint8_t iii = scale8( ii, i);
 
    uint16_t r1 = (3 * (uint16_t)(ii)) - ( 2 * (uint16_t)(iii));
 
    /* the code generated for the above *'s automatically
       cleans up R1, so there's no need to explicitily call
       cleanup_R1(); */
 
    uint8_t result = r1;
 
    // if we got "256", return 255:
    if( r1 & 0x100 ) {
        result = 255;
    }
    return result;
}
 
static uint8_t ease8InOutApprox( uint8_t i, union led_effect_easing_param const *param)
{
    if( i < 64) {
        // start with slope 0.5
        i /= 2;
    } else if( i > (255 - 64)) {
        // end with slope 0.5
        i = 255 - i;
        i /= 2;
        i = 255 - i;
    } else {
        // in the middle, use slope 192/128 = 1.5
        i -= 64;
        i += (i / 2);
        i += 32;
    }
 
    return i;
}


static uint8_t sin8(uint8_t theta, union led_effect_easing_param const *param) {
	const uint8_t b_m16_interleave[] = {0, 49, 49, 41, 90, 27, 117, 10};
    theta = (param->sin.max_value * theta / UINT8_MAX) + param->sin.offset;
	uint8_t offset = theta;
    if (theta & 0x40) {
        offset = (uint8_t)255 - offset;
    }
    offset &= 0x3F; // 0..63
 
    uint8_t secoffset = offset & 0x0F; // 0..15
    if (theta & 0x40)
        ++secoffset;
 
    uint8_t section = offset >> 4; // 0..3
    uint8_t s2 = section * 2;
    const uint8_t *p = b_m16_interleave;
    p += s2;
    uint8_t b = *p;
    ++p;
    uint8_t m16 = *p;
 
    uint8_t mx = (m16 * secoffset) >> 4;
 
    int8_t y = mx + b;
    if (theta & 0x80)
        y = -y;
 
    y += 128;
 
    return y;
}

static uint8_t step(uint8_t i, union led_effect_easing_param const *param) {
	return param->step.step_fraction <= i ? 255 : 0;
}

static uint8_t linear(uint8_t i, union led_effect_easing_param const *param) {
	return i;
}

static uint8_t custom(uint8_t i, union led_effect_easing_param const *param) {
	return param->custom.func(i);
}

static const led_effect_easing_func_t easing_functions[] = {
	[LED_EFFECT_EASING_TYPE_LINEAR] = linear,
	[LED_EFFECT_EASING_TYPE_STEP] = step,
	[LED_EFFECT_EASING_TYPE_SIN] = sin8,
	[LED_EFFECT_EASING_TYPE_CUBIC] = ease8InOutCubic,
	[LED_EFFECT_EASING_TYPE_QUAD] = ease8InOutQuad,
	[LED_EFFECT_EASING_TYPE_APPROX] = ease8InOutApprox,
	[LED_EFFECT_EASING_TYPE_CUSTOM] = custom,
};

static int set_color_one_channel(struct led *led, struct led_color const *color)
{
	/* For a single color LED convert color to brightness. */
	unsigned int brightness = 0;

	for (size_t i = 0; i < ARRAY_SIZE(color->c); i++) {
		brightness += color->c[i];
	}
	brightness /= ARRAY_SIZE(color->c);

	return led_set_brightness(led->dev, 0, brightness);
}

static int set_color_all_channels(struct led *led, struct led_color const *color)
{
	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(color->c)) && !err; i++) {
		err = led_set_brightness(led->dev, i, color->c[i]);
		if (err) {
			LOG_ERR("Cannot set LED color %d brightness %u (err: %d)", i, color->c[i], err);
		}
	}

	return err;
}

static void set_color(struct led *led, struct led_color const *color)
{
	int err;

	if (led->color_count == ARRAY_SIZE(color->c)) {
		err = set_color_all_channels(led, color);
	} else {
		err = set_color_one_channel(led, color);
	}

	if (err) {
		LOG_ERR("Cannot set LED brightness (err: %d)", err);
	}
}

static void set_off(struct led *led)
{
	struct led_color nocolor = {0};

	set_color(led, &nocolor);
}

static void load_step(struct led *led)
{
	if(led->effect->steps[led->effect_step].easing.type >= LED_EFFECT_EASING_TYPE_COUNT) {
		led->easing = NULL;
	} else {
		led->easing = easing_functions[led->effect->steps[led->effect_step].easing.type];
	}
	led->effect_step_start_color = led->color;
	for (size_t i = 0; i < _CAF_LED_COLOR_CHANNEL_COUNT; i++) {
		led->effect_step_color_diff[i] = led->effect->steps[led->effect_step].color.c[i] - led->color.c[i];
	}
	LOG_DBG("LED %u, step %u, easing type %u, duration %d, from %u, %u, %u to %u, %u, %u",
		LED_ID(led), led->effect_step, led->effect->steps[led->effect_step].easing.type,
		led->effect->steps[led->effect_step].duration,
		led->color.c[0], led->color.c[1], led->color.c[2],
		led->effect->steps[led->effect_step].color.c[0], led->effect->steps[led->effect_step].color.c[1], led->effect->steps[led->effect_step].color.c[2]);
}

static void update_led(struct led *led)
{
	if (led->effect == NULL) {
		set_off(led);
		return;
	}
	__ASSERT_NO_MSG(next_update != TIME_INVALID);
	while(true) {
		int64_t time_diff = next_update - led->effect_step_start_time;
		uint16_t duration = led->effect->steps[led->effect_step].duration;
		if (duration < time_diff) { // step complete
			led->color = led->effect->steps[led->effect_step].color;
			set_color(led, &led->color);
			led->effect_step_start_time += duration;
			led->effect_step++;
			if (led->effect_step == led->effect->step_count) {
				led->repeat_count++;
				LOG_DBG("LED %u effect %p performed %u/%u times", LED_ID(led), led->effect, led->repeat_count, led->effect->repeat_count);
				if ((led->effect->repeat_count == 0) || (led->repeat_count < led->effect->repeat_count)) {
					led->effect_step = 0;
				} else {
					led_stop(led);
					struct led_ready_event *event = new_led_ready_event();
					event->led_id = LED_ID(led);
					event->led_effect = led->effect;
					APP_EVENT_SUBMIT(event);
					return;
				}
			}
			load_step(led);
			// update again - possible that multiple steps are completed
			continue;
		}
		uint8_t fraction = time_diff * UINT8_MAX / led->effect->steps[led->effect_step].duration;
		if (led->easing == NULL) {
			set_color(led, &led->effect->steps[led->effect_step].color);
			return;
		}
		uint8_t eased_fraction = led->easing(fraction, &led->effect->steps[led->effect_step].easing.param);
		for (size_t i = 0; i < _CAF_LED_COLOR_CHANNEL_COUNT; i++) {
			led->color.c[i] = (uint8_t)(led->effect_step_start_color.c[i] + (led->effect_step_color_diff[i] * eased_fraction / LED_EFFECT_OUTPUT_MAX));
		}
	#if CONFIG_CAF_LEDS_LOG_ALL_LED_STEPS
		LOG_DBG("LED %u, step %u, fraction %u, eased fraction %u, color %u, %u, %u",
			LED_ID(led), led->effect_step, fraction, eased_fraction, led->color.c[0], led->color.c[1], led->color.c[2]);
	#endif
		set_color(led, &led->color);
		break;
	}
}

static void work_handler(struct k_work *work)
{
	if (leds_running == 0 || next_update == TIME_INVALID) {
		return;
	}
	int64_t now = k_uptime_get();
	if (next_update > now) {
		// not time to update yet, postpone
		k_work_reschedule(&led_update_work, K_MSEC(next_update - now));
		return;
	}
	int64_t time_diff = now - next_update;
	int64_t frames_missed = time_diff / MS_PER_FRAME;
	if (frames_missed > 0) {
		LOG_DBG("%lld frames skipped", frames_missed);
		// update next_update to the last frame boundary before updating LEDs
		next_update += frames_missed * MS_PER_FRAME;
	}
	// update all LEDs
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (leds_running & (1 << i)) {
			update_led(&leds[i]);
		}
	}
	// update current time, in case processing took some ms
	now = k_uptime_get();
	// calculate time until next frame by finding the next frame boundary after current time
	int64_t time_until_next_update = MS_PER_FRAME - ((now - next_update) % MS_PER_FRAME);
	next_update = now + time_until_next_update;
	k_work_reschedule(&led_update_work, K_MSEC(time_until_next_update));
}

static void led_stop(struct led *led)
{
	leds_running &= ~(1 << LED_ID(led));
	if (leds_running == 0) {
		next_update = TIME_INVALID;
		k_work_cancel_delayable(&led_update_work);
	}
}

static void led_start(struct led *led)
{
	led->effect_step = 0;

	if (!led->effect) {
		LOG_DBG("No effect set");
		return;
	}

	__ASSERT_NO_MSG(led->effect->steps);

	if (led->effect->step_count > 0) {
		LOG_DBG("LED %u effect %p starting, %u steps, %d repeats", LED_ID(led), led->effect, led->effect->step_count, led->effect->repeat_count);
		led->repeat_count = 0;
		load_step(led);
		leds_running |= 1 << LED_ID(led);
		if (next_update == TIME_INVALID) {
			next_update = k_uptime_get() + MS_PER_FRAME;
			k_work_reschedule(&led_update_work, K_MSEC(MS_PER_FRAME));
		}
		led->effect_step_start_time = next_update;
	} else {
		LOG_WRN("LED %u effect %p with no effect", LED_ID(led), led->effect);
		led_stop(led);
	}
}

static int leds_init(void)
{
	int err = 0;

	BUILD_ASSERT(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) > 0, "No LEDs defined");

	k_work_init_delayable(&led_update_work, work_handler);

	for (size_t i = 0; (i < ARRAY_SIZE(leds)) && !err; i++) {
		struct led *led = &leds[i];

		led->effect_step_start_time = TIME_INVALID;

		__ASSERT_NO_MSG((led->color_count == 1) || (led->color_count == 3));

		if (!device_is_ready(led->dev)) {
			LOG_ERR("Device %s is not ready", led->dev->name);
			err = -ENODEV;
		} else {
			led_start(led);
		}
	}

	return err;
}

static void leds_start(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_CAF_LEDS_GPIO)
		/* Zephyr power management API is not implemented for GPIO LEDs.
		 * LEDs are turned off by CAF LEDs module.
		 */
		int err = pm_device_action_run(leds[i].dev, PM_DEVICE_ACTION_RESUME);

		if (err) {
			LOG_ERR("Failed to set LED driver into active state (err: %d)", err);
		}
#endif
		led_start(&leds[i]);
	}
}

static void leds_stop(void)
{
	k_work_cancel_delayable(&led_update_work);
	leds_running = 0;
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		set_off(&leds[i]);

#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_CAF_LEDS_GPIO)
		/* Zephyr power management API is not implemented for GPIO LEDs.
		 * LEDs are turned off by CAF LEDs module.
		 */
		int err = pm_device_action_run(leds[i].dev, PM_DEVICE_ACTION_SUSPEND);

		if (err) {
			LOG_ERR("Failed to set LED driver into suspend state (err: %d)", err);
		}
#endif
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	static bool initialized;

	if (is_led_event(aeh)) {
		const struct led_event *event = cast_led_event(aeh);

		__ASSERT_NO_MSG(event->led_id < ARRAY_SIZE(leds));

		struct led *led = &leds[event->led_id];

		led->effect = event->led_effect;

		if (initialized) {
			led_start(led);
		}

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err = leds_init();

			if (!err) {
				initialized = true;
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_LEDS_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
		if (!initialized) {
			leds_start();
			initialized = true;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_LEDS_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		const struct power_down_event *event =
			cast_power_down_event(aeh);

		/* Leds should keep working on system error. */
		if (event->error) {
			return false;
		}

		if (initialized) {
			leds_stop();
			initialized = false;
			module_set_state(MODULE_STATE_OFF);
		}

		return initialized;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, led_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_CAF_LEDS_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
