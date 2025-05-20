/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>

#include "modem_battery.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_printf, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_scanf, const char *, const char *, ...);

typedef int(*nrf_modem_at_printf_t)(const char *, char *);

#define EXAMPLE_VBAT 3456
#define EXAMPLE_POFWARN POFWARN_3100

static void low_level_handler(int battery_voltage)
{
}

static void pofwarn_handler(void)
{
}

static int nrf_modem_at_printf_custom_pofwarn_enable_mdmev_efault(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%MDMEV=1", fmt);

	return -1;
}

static int nrf_modem_at_printf_custom_pofwarn_enable_mdmev(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%MDMEV=1", fmt);

	return 0;
}

static int nrf_modem_at_printf_custom_pofwarn_enable_xpofwarn_efault(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XPOFWARN=1,%d", fmt);

	int val = va_arg(args, int);

	TEST_ASSERT_EQUAL(EXAMPLE_POFWARN, val);

	return -1;
}

static int nrf_modem_at_printf_custom_pofwarn_enable_xpofwarn(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XPOFWARN=1,%d", fmt);

	int val = va_arg(args, int);

	TEST_ASSERT_EQUAL(EXAMPLE_POFWARN, val);

	return 0;
}

static int nrf_modem_at_printf_custom_low_level_enable_efault(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XVBATLVL=1", fmt);

	return -1;
}

static int nrf_modem_at_printf_custom_low_level_enable(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XVBATLVL=1", fmt);

	return 0;
}

static int nrf_modem_at_printf_custom_low_level_disable_efault(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XVBATLVL=0", fmt);

	return -1;
}

static int nrf_modem_at_printf_custom_low_level_disable(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XVBATLVL=0", fmt);

	return 0;
}

static int nrf_modem_at_printf_custom_low_level_set_efault(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XVBATLOWLVL=%d", fmt);

	int val = va_arg(args, int);

	TEST_ASSERT_EQUAL(CONFIG_MODEM_BATTERY_LOW_LEVEL, val);

	return -1;
}

static int nrf_modem_at_printf_custom_low_level_set(const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XVBATLOWLVL=%d", fmt);

	int val = va_arg(args, int);

	TEST_ASSERT_EQUAL(CONFIG_MODEM_BATTERY_LOW_LEVEL, val);

	return 0;
}

static int nrf_modem_at_scanf_custom_vbat_efault(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XVBAT", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XVBAT: %d", fmt);

	return -1;
}

static int nrf_modem_at_scanf_custom_vbat(const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%XVBAT", cmd);
	TEST_ASSERT_EQUAL_STRING("%%XVBAT: %d", fmt);

	int *val = va_arg(args, int *);

	*val = EXAMPLE_VBAT;

	return 1;
}

void setUp(void)
{
	RESET_FAKE(nrf_modem_at_printf);
	RESET_FAKE(nrf_modem_at_scanf);
}

void tearDown(void)
{
}

void test_modem_battery_low_level_handler_set_einval(void)
{
	int ret;

	ret = modem_battery_low_level_handler_set(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_modem_battery_low_level_handler_set_success(void)
{
	int ret;

	ret = modem_battery_low_level_handler_set(low_level_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_modem_battery_pofwarn_handler_set_einval(void)
{
	int ret;

	ret = modem_battery_pofwarn_handler_set(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_modem_battery_pofwarn_handler_set_success(void)
{
	int ret;

	ret = modem_battery_pofwarn_handler_set(pofwarn_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_modem_battery_pofwarn_enable_efault(void)
{
	int ret;
	nrf_modem_at_printf_t funcs[2] = {
		nrf_modem_at_printf_custom_pofwarn_enable_mdmev,
		nrf_modem_at_printf_custom_pofwarn_enable_xpofwarn_efault,
	};

	nrf_modem_at_printf_fake.custom_fake =
		nrf_modem_at_printf_custom_pofwarn_enable_mdmev_efault;

	ret = modem_battery_pofwarn_enable(EXAMPLE_POFWARN);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);

	nrf_modem_at_printf_fake.custom_fake_seq = funcs;
	nrf_modem_at_printf_fake.custom_fake_seq_len = 2;

	ret = modem_battery_pofwarn_enable(EXAMPLE_POFWARN);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
	TEST_ASSERT_EQUAL(3, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_pofwarn_enable_success(void)
{
	int ret;
	nrf_modem_at_printf_t funcs[2] = {
		nrf_modem_at_printf_custom_pofwarn_enable_mdmev,
		nrf_modem_at_printf_custom_pofwarn_enable_xpofwarn,
	};

	nrf_modem_at_printf_fake.custom_fake_seq = funcs;
	nrf_modem_at_printf_fake.custom_fake_seq_len = 2;

	ret = modem_battery_pofwarn_enable(EXAMPLE_POFWARN);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(2, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_pofwarn_disable_success(void)
{
	int ret;

	ret = modem_battery_pofwarn_disable();
	TEST_ASSERT_EQUAL(0, ret);
}

void test_modem_battery_low_level_enable_efault(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_custom_low_level_enable_efault;

	ret = modem_battery_low_level_enable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_low_level_enable(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_custom_low_level_enable;

	ret = modem_battery_low_level_enable();
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_low_level_disable_efault(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_custom_low_level_disable_efault;

	ret = modem_battery_low_level_disable();
	TEST_ASSERT_EQUAL(-EFAULT, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_low_level_disable(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_custom_low_level_disable;

	ret = modem_battery_low_level_disable();
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_low_level_set_efault(void)
{
	int ret;
	int battery_level = CONFIG_MODEM_BATTERY_LOW_LEVEL;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_custom_low_level_set_efault;

	ret = modem_battery_low_level_set(battery_level);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_low_level_set_success(void)
{
	int ret;
	int battery_level = CONFIG_MODEM_BATTERY_LOW_LEVEL;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_custom_low_level_set;

	ret = modem_battery_low_level_set(battery_level);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_printf_fake.call_count);
}

void test_modem_battery_voltage_get_einval(void)
{
	int ret;

	ret = modem_battery_voltage_get(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_modem_battery_voltage_get_efault(void)
{
	int ret;
	int battery_voltage = 0;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_vbat_efault;

	ret = modem_battery_voltage_get(&battery_voltage);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

void test_modem_battery_voltage_get_success(void)
{
	int ret;
	int battery_voltage = 0;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_vbat;

	ret = modem_battery_voltage_get(&battery_voltage);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(EXAMPLE_VBAT, battery_voltage);
	TEST_ASSERT_EQUAL(1, nrf_modem_at_scanf_fake.call_count);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
