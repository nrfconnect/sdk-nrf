/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "npm1300.h"

#define I2C_ADDR_DEV (0x6B)

int write_registers(void *user_handler, uint16_t addr, uint8_t *data, uint32_t num_bytes)
{
	const struct device *i2c_dev = (struct device *)user_handler;
	uint8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* nPM1300 address */
	wr_addr[0] = (addr >> 8) & 0xFF;
	wr_addr[1] = addr & 0xFF;

	/* Setup I2C messages */

	/* Send the address to write to */
	msgs[0].buf = wr_addr;
	msgs[0].len = 2U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Data to be written, and STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, I2C_ADDR_DEV);
}

int read_registers(void *user_handler, uint16_t addr, uint8_t *data, uint32_t num_bytes)
{
	const struct device *i2c_dev = (const struct device *)user_handler;
	uint8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* nPM1300 address */
	wr_addr[0] = (addr >> 8) & 0xFF;
	wr_addr[1] = addr & 0xFF;

	/* Setup I2C messages */

	/* Send the address to read from */
	msgs[0].buf = wr_addr;
	msgs[0].len = 2U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, I2C_ADDR_DEV);
}

/** @brief Possible events from requested nPM device */
typedef enum {
	APP_CHARGER_EVENT_BATTERY_DETECTED,             /** Event registered when battery connection detected */
	APP_CHARGER_EVENT_BATTERY_REMOVED,              /** Event registered when battery connection removed */
	APP_CHARGER_EVENT_VBUS_DETECTED,                /** Event registered when VBUS connection detected */
	APP_CHARGER_EVENT_VBUS_REMOVED,                 /** Event registered when VBSU connection removed */
	APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED,      /** Event registered when trickle charging started */
	APP_CHARGER_EVENT_CHARGING_CC_STARTED,          /** Event registered when constant current charging started */
	APP_CHARGER_EVENT_CHARGING_CV_STARTED,          /** Event registered when constant voltage charging started */
	APP_CHARGER_EVENT_CHARGING_COMPLETED,           /** Event registered when charging completed*/
	APP_CHARGER_EVENT_BATTERY_LOW_ALERT1,           /** Event registered when first low battery voltage alert detected */
	APP_CHARGER_EVENT_BATTERY_LOW_ALERT2,           /** Event registered when second low battery voltage alert detected */
} npm1300_charger_event_t;

/** @brief Possible nPM device working states */
typedef enum {
	APP_STATE_BATTERY_DISCONNECTED,                         /** State when VBUSIN disconnected and battery disconnected*/
	APP_STATE_BATTERY_CONNECTED,                            /** State when VBUSIN disconnected and battery connected */
	APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED,          /** State when VBUSIN connected and battery disconnected */
	APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED,             /** State when VBUSIN connected and battery connected */
	APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE,               /** State when VBUSIN connected, battery connected and charger in trickle mode */
	APP_STATE_VBUS_CONNECTED_CHARGING_CC,                   /** State when VBUSIN connected, battery connected and charger in constant current mode */
	APP_STATE_VBUS_CONNECTED_CHARGING_CV,                   /** State when VBUSIN connected, battery connected and charger in constant voltage mode */
	APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED,            /** State when VBUSIN connected, battery connected and charger completed charging */
	APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING,               /** State when VBUSIN disconnected, battery connected */
	APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1,        /** State when VBUSIN disconnected, battery connected and battery voltage is below first alert threshold */
	APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT2,        /** State when VBUSIN disconnected, battery connected and battery voltage is below second alert threshold */
} npm1300_state_t;

/**
 * @brief Register the new event received from nPM device
 *
 * @param event new event type
 */
void register_state_change(npm1300_charger_event_t event)
{
	static npm1300_state_t state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING;

	switch (event) {
	case APP_CHARGER_EVENT_BATTERY_DETECTED:
		if (state == APP_STATE_BATTERY_DISCONNECTED) {
			state = APP_STATE_BATTERY_CONNECTED;
			printk("State: BATTERY_CONNECTED\n");
		}

		if (state == APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED;
			printk("State: VBUS_CONNECTED_BATTERY_CONNECTED\n");
		}
		break;
	case APP_CHARGER_EVENT_BATTERY_REMOVED:
		if (state == APP_STATE_BATTERY_CONNECTED) {
			state = APP_STATE_BATTERY_DISCONNECTED;
			printk("State: BATTERY_DISCONNECTED\n");
		}

		if (state == APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CC ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CV ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_DISCONNECTED;
			printk("State: VBUS_CONNECTED_BATTERY_DISCONNECTED\n");
		}
		break;
	case APP_CHARGER_EVENT_VBUS_DETECTED:
		if (state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING ||
		    state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1 ||
		    state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT2) {
			state = APP_STATE_VBUS_CONNECTED_BATTERY_CONNECTED;
			printk("State: VBUS_CONNECTED_BATTERY_CONNECTED\n");
		}
		break;

	case APP_CHARGER_EVENT_VBUS_REMOVED:
		if (state == APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CC ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_CV ||
		    state == APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING;
			printk("State: VBUS_NOT_CONNECTED_DISCHARGING\n");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_CC &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_CV &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_TRICKE;
			printk("State: VBUS_CONNECTED_CHARGING_TRICKE\n");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_CC_STARTED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_CC &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_CV &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_CC;
			printk("State: VBUS_CONNECTED_CHARGING_CC\n");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_CV_STARTED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_CV &&
		    state != APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_CV;
			printk("State: VBUS_CONNECTED_CHARGING_CV\n");
		}
		break;
	case APP_CHARGER_EVENT_CHARGING_COMPLETED:
		if (state != APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED) {
			state = APP_STATE_VBUS_CONNECTED_CHARGING_COMPLETED;
			printk("State: VBUS_CONNECTED_CHARGING_COMPLETED\n");
		}
		break;
	case APP_CHARGER_EVENT_BATTERY_LOW_ALERT1:
		if (state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING) {
			state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1;
			printk("State: VBUS_NOT_CONNECTED_DISCHARGING_ALERT1\n");
		}
		break;
	case APP_CHARGER_EVENT_BATTERY_LOW_ALERT2:
		if (state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING ||
		    state == APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT1) {
			state = APP_STATE_VBUS_NOT_CONNECTED_DISCHARGING_ALERT2;
			printk("State: VBUS_NOT_CONNECTED_DISCHARGING_ALERT2\n");
		}
		break;

	default:
		printk("Unsupported event: %d\n", event);
		break;
	}
}

void generic_callback(npm1300_instance_t *pm, npm1300_callback_type_t type, uint32_t arg)
{
	printk("[%lld]\t %s:\t\t", k_uptime_get(), npm1300_callback_to_str(type));
	for (uint8_t i = 0; i < 8; i++) {
		if ((1U << i) & arg) {
			printk("%s, ", npm1300_callback_bit_to_str(type, i));
		}
	}
	printk("\n");
}

void charger_battery_callback(npm1300_instance_t *pm, npm1300_callback_type_t type, uint32_t arg)
{
	if (arg & (uint32_t)NPM1300_EVENT_GROUP_BATTERY_DETECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_BATTERY_DETECTED);
	}

	if (arg & (uint32_t)NPM1300_EVENT_GROUP_BATTERY_REMOVED_MASK) {
		register_state_change(APP_CHARGER_EVENT_BATTERY_REMOVED);
	}
}

void charger_status_callback(npm1300_instance_t *pm, npm1300_callback_type_t type, uint32_t arg)
{
	npm1300_charger_status_mask_t status;

	if (npm1300_charger_status_get(pm, &status)) {
		if (status & NPM1300_CHARGER_STATUS_TRICKLE_CHARGE_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED);
		}

		if (status & NPM1300_CHARGER_STATUS_CONSTANT_CURRENT_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_CC_STARTED);
		}

		if (status & NPM1300_CHARGER_STATUS_CONSTANT_VOLTAGE_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_CV_STARTED);
		}

		if (status & NPM1300_CHARGER_STATUS_COMPLETED_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_COMPLETED);
		}
	}
}

void vbusin_callback(npm1300_instance_t *pm, npm1300_callback_type_t type, uint32_t arg)
{
	if (arg & (uint32_t)NPM1300_EVENT_GROUP_VBUSIN_DETECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_DETECTED);
	}

	if (arg & (uint32_t)NPM1300_EVENT_GROUP_VBUSIN_REMOVED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_REMOVED);
	}
}

void rst_cause_callback(npm1300_instance_t *pm, npm1300_callback_type_t type, uint32_t arg)
{
	(void) type;
	printk("RSTCAUSE callback, arg: %d\n", (int)arg);
}

void adc_callback(npm1300_instance_t *pm, npm1300_callback_type_t type, uint32_t arg)
{
	if ((arg & (uint32_t)NPM1300_EVENT_GROUP_ADC_BAT_READY_MASK)) {
		static uint16_t battery_voltage_millivolts_last = 0; /* For debug information, to see voltage increasing or decreasing */
		uint16_t battery_voltage_millivolts;
		if (npm1300_adc_vbat_get(pm, &battery_voltage_millivolts)) {
			if (battery_voltage_millivolts != battery_voltage_millivolts_last) {
				battery_voltage_millivolts_last = battery_voltage_millivolts;
				printk("[%lld]\t %d mV\n", k_uptime_get(), battery_voltage_millivolts);
			}
			if (battery_voltage_millivolts < 3550) {
				register_state_change(APP_CHARGER_EVENT_BATTERY_LOW_ALERT2);
			} else if (battery_voltage_millivolts < 3590) {
				register_state_change(APP_CHARGER_EVENT_BATTERY_LOW_ALERT1);
			}
		}
	}
}

static struct gpio_callback callback;
static struct gpio_callback pof_callback;
static npm1300_instance_t pm;

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static const struct gpio_dt_spec npm_spec_irq_0 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(npm_irq0), gpios, { 0 });
static const struct gpio_dt_spec npm_spec_irq_1 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(npm_irq1), gpios, { 0 });

static void irq_handler(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	/* TODO: sometimes this interrupt is called twice before entering to npm1300_proc().
	        It should be investigated */
	gpio_pin_interrupt_configure_dt(&npm_spec_irq_0, GPIO_INT_DISABLE);
	npm1300_interrupt(&pm);
}

static void pof_irq_handler(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	printk("POF\n"); /* After pof event, only short messages can be printed*/
}

void main(void)
{
	pm.user_handler = (void *)i2c_dev;
	pm.write_registers = write_registers;
	pm.read_registers = read_registers;
	pm.generic_cb = generic_callback;

	int ret = 0;

	if (!device_is_ready(i2c_dev)) {
		printk("I2C: Device is not ready.\n");
		return;
	}

	if (!device_is_ready(npm_spec_irq_0.port)) {
		printk("IRQ0: Device is not ready.\n");
		return;
	}

	if (!device_is_ready(npm_spec_irq_1.port)) {
		printk("IRQ1: Device is not ready.\n");
		return;
	}

	ret = gpio_pin_configure_dt(&npm_spec_irq_0, GPIO_INPUT);
	if (ret < 0) {
		printk("Failed to configure port %s pin %u, error: %d",
		       npm_spec_irq_0.port->name, npm_spec_irq_0.pin, ret);
	}

	gpio_init_callback(&callback, irq_handler, BIT(npm_spec_irq_0.pin));
	ret = gpio_add_callback(npm_spec_irq_0.port, &callback);

	if (ret < 0) {
		printk("Failed to add the callback for port %s pin %u, error: %d",
		       npm_spec_irq_0.port->name, npm_spec_irq_0.pin, ret);
	}

	ret = gpio_pin_interrupt_configure_dt(&npm_spec_irq_0, GPIO_INT_LEVEL_HIGH);

	if (ret < 0) {
		printk("Failed to configure interrupt for port %s pin %u, "
		       "error: %d",
		       npm_spec_irq_0.port->name, npm_spec_irq_0.pin, ret);
	}

	ret = gpio_pin_configure_dt(&npm_spec_irq_1, GPIO_INPUT);
	if (ret < 0) {
		printk("Failed to configure port %s pin %u, error: %d",
		       npm_spec_irq_1.port->name, npm_spec_irq_1.pin, ret);
	}

	gpio_init_callback(&pof_callback, pof_irq_handler, BIT(npm_spec_irq_1.pin));
	ret = gpio_add_callback(npm_spec_irq_1.port, &pof_callback);

	if (ret < 0) {
		printk("Failed to add the pof_callback for port %s pin %u, error: %d",
		       npm_spec_irq_1.port->name, npm_spec_irq_1.pin, ret);
	}

	ret = gpio_pin_interrupt_configure_dt(&npm_spec_irq_1, GPIO_INT_EDGE_RISING);

	npm1300_init(&pm);

	npm1300_register_cb(&pm, rst_cause_callback, NPM1300_CALLBACK_TYPE_RSTCAUSE);

	npm1300_register_cb(&pm, adc_callback, NPM1300_CALLBACK_TYPE_EVENT_ADC);

	npm1300_register_cb(&pm, charger_battery_callback, NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT);

	npm1300_register_cb(&pm, charger_status_callback, NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS);

	npm1300_register_cb(&pm, vbusin_callback, NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE);

	ret = npm1300_stop_boot_timer(&pm);

	if (ret) {
		printk("Boot timer stopped.\r\n");
	} else {
		printk("Boot timer stopping failed.\r\n");
	}

	ret = npm1300_connection_check(&pm);

	if (ret) {
		printk("Connection check ok.\r\n");
	} else {
		printk("Connection check failed.\r\n");
	}

	npm1300_gpio_mode_set(&pm, NPM1300_GPIO_INSTANCE_0, NPM1300_GPIO_MODE_OUTPUT_IRQ);

	npm1300_gpio_mode_set(&pm, NPM1300_GPIO_INSTANCE_1, NPM1300_GPIO_MODE_OUTPUT_PLW);

	npm1300_pof_enable(&pm, NTC1300_POF_POLARITY_HI, NTC1300_POF_THRESHOLD_3V3);

	/* Disable charger before changing charge current */
	npm1300_charger_module_disable(&pm, NPM1300_CHARGER_MODULE_CHARGER_MASK);

	npm1300_charger_charging_current_set(&pm, 200);

	npm1300_charger_termination_volatge_normal_set(&pm, NPM1300_CHARGER_VOLTAGE_4V10);

	/* Enable charger for events handling */
	npm1300_charger_module_enable(&pm,
				      NPM1300_CHARGER_MODULE_CHARGER_MASK
				      | NPM1300_CHARGER_MODULE_RECHARGE_MASK
				      | NPM1300_CHARGER_MODULE_NTC_LIMITS_MASK);

	npm1300_event_interrupt_enable(&pm,
				       NPM1300_EVENT_GROUP_VBUSIN_VOLTAGE,
				       NPM1300_EVENT_GROUP_VBUSIN_DETECTED_MASK
				       | NPM1300_EVENT_GROUP_VBUSIN_REMOVED_MASK);

	npm1300_event_interrupt_enable(&pm,
				       NPM1300_EVENT_GROUP_BAT_CHAR_BAT,
				       NPM1300_EVENT_GROUP_BATTERY_DETECTED_MASK
				       | NPM1300_EVENT_GROUP_BATTERY_REMOVED_MASK);

	/* Enable all charging status events */
	npm1300_event_interrupt_enable(&pm,
				       NPM1300_EVENT_GROUP_BAT_CHAR_STATUS,
				       NPM1300_EVENT_GROUP_CHARGER_SUPPLEMENT_MASK
				       | NPM1300_EVENT_GROUP_CHARGER_TRICKLE_MASK
				       | NPM1300_EVENT_GROUP_CHARGER_CC_MASK
				       | NPM1300_EVENT_GROUP_CHARGER_CV_MASK
				       | NPM1300_EVENT_GROUP_CHARGER_COMPLETED_MASK
				       | NPM1300_EVENT_GROUP_CHARGER_ERROR_MASK);

	/* Enable ADC measurements ready interrupts */
	npm1300_event_interrupt_enable(&pm,
				       NPM1300_EVENT_GROUP_ADC,
				       NPM1300_EVENT_GROUP_ADC_BAT_READY_MASK);

	/* Enable VBAT auto measurement every 1 second */
	npm1300_adc_vbat_auto_meas_enable(&pm);

	npm1300_adc_ntc_set(&pm, NPM1300_BATTERY_NTC_TYPE_10_K);

	npm1300_check_errors(&pm);

	while (1) {
		npm1300_proc(&pm);
		gpio_pin_interrupt_configure_dt(&npm_spec_irq_0, GPIO_INT_LEVEL_HIGH);
	}
}
