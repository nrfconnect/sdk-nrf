/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_battery.h>
#include <modem/at_monitor.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#define AT_XVBATLVL_SET_ENABLE	"AT%%XVBATLVL=1"
#define AT_XVBATLVL_SET_DISABLE	"AT%%XVBATLVL=0"
#define AT_XVBATLOWLVL_SET      "AT%%XVBATLOWLVL=%d"
#define AT_XVBAT_SET            "AT%XVBAT"
#define AT_XVBATLOWLVL_READ     "AT%XVBATLOWLVL?"
#define AT_XPOFWARN_SET_ENABLE  "AT%%XPOFWARN=1,%d"
#define AT_XPOFWARN_SET_DISABLE "AT%%XPOFWARN=0"
#define AT_MDMEV_SET_ENABLE     "AT%%MDMEV=1"

LOG_MODULE_REGISTER(modem_battery, CONFIG_MODEM_BATTERY_LOG_LEVEL);

AT_MONITOR(xvbatlowlvl_mon, "%XVBATLOWLVL", at_handler_xvbatlowlvl);
AT_MONITOR(pofwarn_mon, "%MDMEV: ME BATTERY LOW", at_handler_pofwarn);

NRF_MODEM_LIB_ON_INIT(init_hook, on_modem_init, NULL);

static modem_battery_low_level_handler_t xvbatlowlvl_handler;
static modem_battery_pofwarn_handler_t pofwarn_handler;

static void at_handler_xvbatlowlvl(const char *response)
{
	int ret;
	int battery_voltage;

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%XVBATLOWLVL notification: %s", response);

	ret = sscanf(response, "%%XVBATLOWLVL: %d", &battery_voltage);
	/* Match exactly once. */
	if (ret != 1) {
		LOG_ERR("Failed to read battery low level notification, error: %d", ret);
		return;
	}

	if (xvbatlowlvl_handler == NULL) {
		return;
	}

	xvbatlowlvl_handler(battery_voltage);
}

static void at_handler_pofwarn(const char *response)
{
	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%MDMEV notification: %s", response);

	if (pofwarn_handler == NULL) {
		return;
	}

	pofwarn_handler();
}

int modem_battery_low_level_handler_set(modem_battery_low_level_handler_t handler)
{
	if (handler == NULL) {
		LOG_ERR("Invalid handler (handler=0x%08X)", (uint32_t)handler);
		return -EINVAL;
	}

	xvbatlowlvl_handler = handler;

	return 0;
}

int modem_battery_pofwarn_handler_set(modem_battery_pofwarn_handler_t handler)
{
	if (handler == NULL) {
		LOG_ERR("Invalid handler (handler=0x%08X)", (uint32_t)handler);
		return -EINVAL;
	}

	pofwarn_handler = handler;

	return 0;
}

int modem_battery_pofwarn_enable(enum pofwarn_level level)
{
	int err;

	err = nrf_modem_at_printf(AT_MDMEV_SET_ENABLE);
	if (err) {
		return -EFAULT;
	}

	err = nrf_modem_at_printf(AT_XPOFWARN_SET_ENABLE, level);
	if (err) {
		return -EFAULT;
	}

	at_monitor_resume(&pofwarn_mon);

	return 0;
}

int modem_battery_pofwarn_disable(void)
{
	at_monitor_pause(&pofwarn_mon);

	return 0;
}

int modem_battery_low_level_enable(void)
{
	return nrf_modem_at_printf(AT_XVBATLVL_SET_ENABLE) ? -EFAULT : 0;
}

int modem_battery_low_level_disable(void)
{
	return nrf_modem_at_printf(AT_XVBATLVL_SET_DISABLE) ? -EFAULT : 0;
}

int modem_battery_low_level_set(int battery_level)
{
	return nrf_modem_at_printf(AT_XVBATLOWLVL_SET, battery_level) ? -EFAULT : 0;
}

int modem_battery_low_level_get(int *battery_level)
{
	int ret;
	int xvbatlowlvl;

	if (battery_level == NULL) {
		return -EINVAL;
	}

	/* Format of XVBATLOWLVL AT command response:
	 * %XVBATLOWLVL: <battery_level>
	 */
	ret = nrf_modem_at_scanf(AT_XVBATLOWLVL_READ, "%%XVBATLOWLVL: %d", &xvbatlowlvl);
	/* Match exactly once. */
	if (ret != 1) {
		LOG_ERR("Failed to read battery low level, error: %d", ret);
		return -EFAULT;
	}

	*battery_level = xvbatlowlvl;

	return 0;
}

int modem_battery_voltage_get(int *battery_voltage)
{
	int ret;
	int xvbat;

	if (battery_voltage == NULL) {
		return -EINVAL;
	}

	/* Format of XVBAT AT command response:
	 * %XVBAT: <vbat>
	 */
	ret = nrf_modem_at_scanf("AT%XVBAT", "%%XVBAT: %d", &xvbat);
	/* Match exactly once. */
	if (ret != 1) {
		LOG_ERR("Failed to read battery voltage, error: %d", ret);
		return -EFAULT;
	}

	*battery_voltage = xvbat;

	return 0;
}

static void on_modem_init(int ret, void *ctx)
{
	int err;

	err = modem_battery_low_level_set(CONFIG_MODEM_BATTERY_LOW_LEVEL);
	if (err) {
		LOG_ERR("Failed to set battery low level, error: %d", err);
		return;
	}

	err = modem_battery_low_level_enable();
	if (err) {
		LOG_ERR("Failed to enable battery low level, error: %d", err);
		return;
	}

	err = modem_battery_pofwarn_enable(CONFIG_MODEM_BATTERY_POFWARN_VOLTAGE);
	if (err) {
		LOG_ERR("Failed to enable power off warnings, error: %d", err);
		return;
	}
}
