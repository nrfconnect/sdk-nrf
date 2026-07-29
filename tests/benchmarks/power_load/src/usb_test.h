/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <sample_usbd.h>

#define USB_THREAD_STACKSIZE (4096)
#define USB_THREAD_PRIORITY  (1)
#define USB_THREAD_SLEEP     (100)

#define RING_BUF_SIZE		  1024
#define TEST_TRANSMISSION_TIME_MS 4000
