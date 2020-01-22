/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <soc.h>
#include <errno.h>
#include <stdbool.h>
#include <device.h>
#include <kernel_includes.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <mpsl.h>
#include <mpsl_clock.h>
#include <multithreading_lock.h>
#if IS_ENABLED(CONFIG_USB_NRFX)
#include <hal/nrf_power.h>
#endif

static bool is_context_atomic(void)
{
	int key = irq_lock();
	irq_unlock(key);

	return arch_irq_unlocked(key) || k_is_in_isr();
}

static int hf_clock_start(void)
{
	int errcode;

	if (!is_context_atomic()) {
		errcode = MULTITHREADING_LOCK_ACQUIRE();
	} else {
		errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
	}
	if (errcode) {
		return -EFAULT;
	}
	errcode = mpsl_clock_hfclk_request(NULL);
	MULTITHREADING_LOCK_RELEASE();
	if (errcode) {
		return -EFAULT;
	}

	return 0;
}

static int hf_clock_stop(void)
{
	int errcode;

	if (!is_context_atomic()) {
		errcode = MULTITHREADING_LOCK_ACQUIRE();
	} else {
		errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
	}
	if (errcode) {
		return -EFAULT;
	}
	errcode = mpsl_clock_hfclk_release();
	MULTITHREADING_LOCK_RELEASE();
	if (errcode) {
		return -EFAULT;
	}

	return 0;
}

static enum clock_control_status hf_get_status(void)
{
	int errcode;
	u32_t is_running = 0;

	if (!is_context_atomic()) {
		errcode = MULTITHREADING_LOCK_ACQUIRE();
	} else {
		errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
	}
	if (errcode) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
	errcode = mpsl_clock_hfclk_is_running(&is_running);
	MULTITHREADING_LOCK_RELEASE();
	if (errcode) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return (is_running ? CLOCK_CONTROL_STATUS_ON
			   : CLOCK_CONTROL_STATUS_OFF);
}

#if IS_ENABLED(CONFIG_USB_NRFX)
static inline void power_event_cb(nrf_power_event_t event)
{
	extern void usb_dc_nrfx_power_event_callback(nrf_power_event_t event);

	usb_dc_nrfx_power_event_callback(event);
}
#endif

void nrf_power_clock_isr(void)
{
	MPSL_IRQ_CLOCK_Handler();
#if IS_ENABLED(CONFIG_USB_NRFX)
	bool usb_detected, usb_pwr_rdy, usb_removed;

	usb_detected = nrf_power_event_check(NRF_POWER,
					NRF_POWER_EVENT_USBDETECTED);
	usb_pwr_rdy = nrf_power_event_check(NRF_POWER,
					NRF_POWER_EVENT_USBPWRRDY);
	usb_removed = nrf_power_event_check(NRF_POWER,
					NRF_POWER_EVENT_USBREMOVED);

	if (usb_detected) {
		nrf_power_event_clear(NRF_POWER, NRF_POWER_EVENT_USBDETECTED);
		power_event_cb(NRF_POWER_EVENT_USBDETECTED);
	}

	if (usb_pwr_rdy) {
		nrf_power_event_clear(NRF_POWER, NRF_POWER_EVENT_USBPWRRDY);
		power_event_cb(NRF_POWER_EVENT_USBPWRRDY);
	}

	if (usb_removed) {
		nrf_power_event_clear(NRF_POWER, NRF_POWER_EVENT_USBREMOVED);
		power_event_cb(NRF_POWER_EVENT_USBREMOVED);
	}
#endif
}

static int clock_start(struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);

	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;

	switch (type) {
	case CLOCK_CONTROL_NRF_TYPE_HFCLK:
		return hf_clock_start();
	case CLOCK_CONTROL_NRF_TYPE_LFCLK:
		/* No-op. LFCLK is started by default by mpsl_init(). */
		return 0;
	default:
		return -EINVAL;
	}
}

static int clock_stop(struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);

	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;

	switch (type) {
	case CLOCK_CONTROL_NRF_TYPE_HFCLK:
		return hf_clock_stop();
	case CLOCK_CONTROL_NRF_TYPE_LFCLK:
		/* Stopping of LFCLK is not supported. */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}
}

static int clock_get_rate(struct device *dev, clock_control_subsys_t subsys,
			  u32_t *rate)
{
	ARG_UNUSED(dev);

	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;

	if (rate == NULL) {
		return -EINVAL;
	}

	switch (type) {
	case CLOCK_CONTROL_NRF_TYPE_HFCLK:
		*rate = MHZ(16);
	case CLOCK_CONTROL_NRF_TYPE_LFCLK:
		*rate = 32768;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum clock_control_status clock_get_status(struct device *dev,
						  clock_control_subsys_t subsys)
{
	ARG_UNUSED(dev);

	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;

	switch (type) {
	case CLOCK_CONTROL_NRF_TYPE_HFCLK:
		return hf_get_status();
	case CLOCK_CONTROL_NRF_TYPE_LFCLK:
		/* LFCLK is started by default by mpsl_init(),
		 * and stopping of it is not supported.
		 */
		return CLOCK_CONTROL_STATUS_ON;
	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

static int clock_control_init(struct device *dev)
{
	ARG_UNUSED(dev);
	/* No-op. LFCLK is initialized by mpsl_init() at PRE_KERNEL_1,
	 * see subsys/mpsl/mpsl_init.c.
	 */

	IRQ_CONNECT(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0,
		    DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0_PRIORITY,
		    nrf_power_clock_isr, 0, 0);

	return 0;
}

static const struct clock_control_driver_api clock_control_api = {
	.on = clock_start,
	.off = clock_stop,
	.get_rate = clock_get_rate,
	.get_status = clock_get_status,
};

DEVICE_AND_API_INIT(clock_nrf,
		    DT_INST_0_NORDIC_NRF_CLOCK_LABEL,
		    clock_control_init, NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &clock_control_api);

#if IS_ENABLED(CONFIG_USB_NRFX)

void nrf5_power_usb_power_int_enable(bool enable)
{
	u32_t mask;


	mask = NRF_POWER_INT_USBDETECTED_MASK |
	       NRF_POWER_INT_USBREMOVED_MASK |
	       NRF_POWER_INT_USBPWRRDY_MASK;

	if (enable) {
		nrf_power_int_enable(NRF_POWER, mask);
		irq_enable(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0);
	} else {
		nrf_power_int_disable(NRF_POWER, mask);
	}
}

#endif
