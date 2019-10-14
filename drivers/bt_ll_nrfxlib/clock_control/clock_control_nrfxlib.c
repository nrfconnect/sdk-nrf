/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <soc.h>
#include <errno.h>
#include <stdbool.h>
#include <device.h>
#include <kernel_includes.h>
#include <clock_control.h>
#ifdef CONFIG_BT_LL_NRFXLIB
#include <ble_controller.h>
#include <ble_controller_soc.h>
#else /* CONFIG_MPSL */
#include <mpsl.h>
#include <mpsl_clock.h>
#endif
#include <multithreading_lock.h>
#if IS_ENABLED(CONFIG_USB_NRF52840)
#include <hal/nrf_power.h>
#endif

static bool is_context_atomic(void)
{
	int key = irq_lock();
	irq_unlock(key);

	return arch_irq_unlocked(key) || k_is_in_isr();
}

static int hf_clock_start(struct device *dev, clock_control_subsys_t sub_system)
{
	int errcode;

	ARG_UNUSED(dev);

	bool blocking = POINTER_TO_UINT(sub_system);

	if (!is_context_atomic()) {
		errcode = MULTITHREADING_LOCK_ACQUIRE();
	} else {
		errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
	}
	if (errcode) {
		return -EFAULT;
	}

#ifdef CONFIG_BT_LL_NRFXLIB
	errcode = ble_controller_hf_clock_request(NULL);
#else /* CONFIG_MPSL */
	errcode = mpsl_clock_hfclk_request(NULL);
#endif
	MULTITHREADING_LOCK_RELEASE();
	if (errcode) {
		return -EFAULT;
	}

	if (blocking) {
#ifdef CONFIG_BT_LL_NRFXLIB
		bool is_running = false;
#else /* CONFIG_MPSL */
		u32_t is_running = 0;
#endif

		do {
			if (!is_context_atomic()) {
				errcode = MULTITHREADING_LOCK_ACQUIRE();
			} else {
				errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
			}
			if (errcode) {
				return -EFAULT;
			}

#ifdef CONFIG_BT_LL_NRFXLIB
			errcode = ble_controller_hf_clock_is_running(&is_running);
#else /* CONFIG_MPSL */
			errcode = mpsl_clock_hfclk_is_running(&is_running);
#endif
			MULTITHREADING_LOCK_RELEASE();
			if (errcode) {
				return -EFAULT;
			}

			if (!is_running) {
				if (!k_is_in_isr()) {
					k_yield();
				} else { /* in isr */
					k_cpu_idle();
				}

			}
		} while (!is_running);
	}

	return 0;
}

static int hf_clock_stop(struct device *dev, clock_control_subsys_t sub_system)
{
	int errcode;

	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	if (!is_context_atomic()) {
		errcode = MULTITHREADING_LOCK_ACQUIRE();
	} else {
		errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
	}
	if (errcode) {
		return -EFAULT;
	}

#ifdef CONFIG_BT_LL_NRFXLIB
	errcode = ble_controller_hf_clock_release();
#else /* CONFIG_MPSL */
	errcode = mpsl_clock_hfclk_release();
#endif
	MULTITHREADING_LOCK_RELEASE();
	if (errcode) {
		return -EFAULT;
	}

	return 0;
}

static int hf_clock_get_rate(struct device *dev,
			     clock_control_subsys_t sub_system, u32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	if (rate == NULL) {
		return -EINVAL;
	}

	*rate = MHZ(16);
	return 0;
}

static int lf_clock_start(struct device *dev, clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	/* No-op. LFCLK is started by default by ble_controller_init(). */

	return 0;
}

static int lf_clock_get_rate(struct device *dev,
			     clock_control_subsys_t sub_system, u32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	if (rate == NULL) {
		return -EINVAL;
	}

	*rate = 32768;
	return 0;
}

#if IS_ENABLED(CONFIG_USB_NRF52840)
static inline void power_event_cb(nrf_power_event_t event)
{
	extern void usb_dc_nrfx_power_event_callback(nrf_power_event_t event);

	usb_dc_nrfx_power_event_callback(event);
}
#endif

void nrf_power_clock_isr(void)
{
#if IS_ENABLED(CONFIG_USB_NRF52840)
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

#ifdef CONFIG_BT_LL_NRFXLIB
	/* FIXME/NOTE: This handler is called _after_ the USB registers are
	 * checked. The reason is that the BLE controller also checks these
	 * registsers _and clears them_, but there is currently no event
	 * propagated from the controller.
	 */
	ble_controller_POWER_CLOCK_IRQHandler();
#else /* CONFIG_MPSL */
	MPSL_IRQ_CLOCK_Handler();
#endif
}

static int clock_control_init(struct device *dev)
{
	ARG_UNUSED(dev);
	/* No-op. For BLE Controller, LFCLK is initialized
	 * in subsys/bluetooth/controller/hci_driver.c by
	 * ble_init() at PRE_KERNEL_1. ble_init() will call
	 * ble_controller_init() that in turn will start the LFCLK.
	 * For MPSL, LFCLK is initialized in subsys/mpsl/mpsl_init.c by
	 * mpsl_lib_init() which calls mpsl_init() at PRE_KERNEL_1.
	 */

	IRQ_CONNECT(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0,
		    DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0_PRIORITY,
		    nrf_power_clock_isr, 0, 0);

	return 0;
}

static const struct clock_control_driver_api hf_clock_control_api = {
	.on = hf_clock_start,
	.off = hf_clock_stop,
	.get_rate = hf_clock_get_rate,
};

DEVICE_AND_API_INIT(hf_clock,
		    DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_16M",
		    clock_control_init, NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &hf_clock_control_api);

/* LFCLK doesn't have stop function to replicate the nRF5 Power Clock driver
 * behavior.
 */
static const struct clock_control_driver_api lf_clock_control_api = {
	.on = lf_clock_start,
	.off = NULL,
	.get_rate = lf_clock_get_rate,
};

DEVICE_AND_API_INIT(lf_clock,
		    DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_32K",
		    clock_control_init, NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &lf_clock_control_api);

#if IS_ENABLED(CONFIG_USB_NRF52840)

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
