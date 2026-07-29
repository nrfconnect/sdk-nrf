#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(power_load)

target_sources_ifdef(CONFIG_ADC app PRIVATE src/adc_test.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/bt_test.c)
target_sources_ifdef(CONFIG_COUNTER app PRIVATE src/counter_test.c)
target_sources_ifdef(CONFIG_FLASH app PRIVATE src/flash_test.c)
target_sources_ifdef(CONFIG_GPIO app PRIVATE src/gpio_test.c)
target_sources_ifdef(CONFIG_I2C app PRIVATE src/i2c_test.c)
target_sources_ifdef(CONFIG_I2S app PRIVATE src/i2s_test.c)
target_sources_ifdef(CONFIG_PWM app PRIVATE src/pwm_test.c)
target_sources_ifdef(CONFIG_SENSOR app PRIVATE src/sensor_test.c)
target_sources_ifdef(CONFIG_WATCHDOG app PRIVATE src/wdt_test.c)

zephyr_include_directories_ifdef(CONFIG_USB_DEVICE_STACK_NEXT ${ZEPHYR_BASE}/samples/subsys/usb/common)
zephyr_sources_ifdef(CONFIG_USB_DEVICE_STACK_NEXT ${ZEPHYR_BASE}/samples/subsys/usb/common/sample_usbd_init.c)
target_sources_ifdef(CONFIG_USB_DEVICE_STACK_NEXT app PRIVATE src/usb_test.c)

target_sources(app PRIVATE src/cpu_test.c src/main.c)
