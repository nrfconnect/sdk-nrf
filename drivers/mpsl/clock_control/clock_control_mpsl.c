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

/* MPSL clock control structure */
struct mpsl_clock_control_data {
	sys_slist_t async_on_list;
	u8_t hfclk_count;
};

/* MPSL clock control callbacks */
static void hf_clock_started_callback(void);

static bool is_context_atomic(void)
{
	int key = irq_lock();
	irq_unlock(key);

	return arch_irq_unlocked(key) || k_is_in_isr();
}

static bool clock_list_find(sys_slist_t *list, sys_snode_t *node)
{
	bool found = false;
	int key = irq_lock();
	struct clock_control_async_data *clock_async_data = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(list, clock_async_data, node) {
		if (&(clock_async_data->node) == node) {
			found = true;
			break;
		}
	}

	irq_unlock(key);
	return found;
}

static void clock_list_append(sys_slist_t *list, sys_snode_t *node)
{
	int key = irq_lock();

	sys_slist_append(list, node);
	irq_unlock(key);
}

static int hf_clock_start(mpsl_clock_hfclk_callback_t hfclk_callback)
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
	errcode = mpsl_clock_hfclk_request(hfclk_callback);
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
	struct mpsl_clock_control_data *mpsl_control_data = dev->driver_data;
	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;
	int errcode = 0;
	int key;

	switch (type) {
	case CLOCK_CONTROL_NRF_TYPE_HFCLK:
		key = irq_lock();
		if ((u8_t)(mpsl_control_data->hfclk_count + 1) == 0) {
			/* overflow the start count that it can support */
			irq_unlock(key);
			return -ENOTSUP;
		} else if (mpsl_control_data->hfclk_count == 0) {
			errcode = hf_clock_start(hf_clock_started_callback);
		}

		if (errcode == 0) {
			++mpsl_control_data->hfclk_count;
		}

		irq_unlock(key);
		return errcode;
	case CLOCK_CONTROL_NRF_TYPE_LFCLK:
		/* No-op. LFCLK is started by default by mpsl_init(). */
		return 0;
	default:
		return -EINVAL;
	}
}

static int clock_stop(struct device *dev, clock_control_subsys_t subsys)
{
	struct mpsl_clock_control_data *mpsl_control_data = dev->driver_data;
	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;
	int errcode = 0;
	int key;

	switch (type) {
	case CLOCK_CONTROL_NRF_TYPE_HFCLK:
		key = irq_lock();
		if (mpsl_control_data->hfclk_count == 0) {
			irq_unlock(key);
			return -EALREADY;
		} else if (mpsl_control_data->hfclk_count == 1) {
			sys_slist_t *list = &(mpsl_control_data->async_on_list);

			errcode = hf_clock_stop();
			if (errcode == 0) {
				sys_slist_init(list);
			}
		}

		if (errcode == 0) {
			--mpsl_control_data->hfclk_count;
		}

		irq_unlock(key);
		return errcode;
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

static int clock_async_start(struct device *dev, clock_control_subsys_t subsys,
			     struct clock_control_async_data *data)
{
	struct mpsl_clock_control_data *mpsl_control_data = dev->driver_data;
	int errcode = 0;
	int key = irq_lock();
	enum clock_control_status clock_status = clock_get_status(dev, subsys);

	switch (clock_status) {
	case CLOCK_CONTROL_STATUS_ON:
		if (subsys == CLOCK_CONTROL_NRF_SUBSYS_HF) {
			if ((u8_t)(mpsl_control_data->hfclk_count + 1) == 0) {
				/*  the counter of HFCLK gets overflow */
				irq_unlock(key);
				return -ENOTSUP;
			}
			++mpsl_control_data->hfclk_count;
		}

		/* LFCLK is started by default and not able to stop it.
		 * Therefore, LFCLK is always handled in case STATUS_ON.
		 */
		if ((data != NULL) && (data->cb != NULL)) {
			data->cb(dev, data->user_data);
		}

		break;
	case CLOCK_CONTROL_STATUS_STARTING:
	case CLOCK_CONTROL_STATUS_OFF:
		if ((data != NULL) && (data->cb != NULL)) {
			sys_slist_t *list = &(mpsl_control_data->async_on_list);

			/* if the same request is already scheduled and not yet
			 * completed.
			 */
			if (clock_list_find(list, &data->node)) {
				irq_unlock(key);
				return -EBUSY;
			}

			clock_list_append(list, &data->node);
		}

		errcode =  clock_start(dev, subsys);
		break;
	default:
		errcode = -ENOTSUP;
		break;
	}

	irq_unlock(key);
	return errcode;
}

static int clock_control_init(struct device *dev)
{
	struct mpsl_clock_control_data *mpsl_control_data = dev->driver_data;

	/* No-op. LFCLK is initialized by mpsl_init() at PRE_KERNEL_1,
	 * see subsys/mpsl/mpsl_init.c.
	 */

	IRQ_CONNECT(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0,
		    DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0_PRIORITY,
		    nrf_power_clock_isr, 0, 0);

	sys_slist_init(&(mpsl_control_data->async_on_list));
	mpsl_control_data->hfclk_count = 0;
	return 0;
}

static const struct clock_control_driver_api clock_control_api = {
	.on = clock_start,
	.off = clock_stop,
	.async_on = clock_async_start,
	.get_rate = clock_get_rate,
	.get_status = clock_get_status,
};

static struct mpsl_clock_control_data clock_control_data;

DEVICE_AND_API_INIT(clock_nrf,
		    DT_INST_0_NORDIC_NRF_CLOCK_LABEL,
		    clock_control_init, &clock_control_data, NULL, PRE_KERNEL_1,
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

static void hf_clock_started_callback(void)
{
	/* NOTE: Don't acquire the lock of mpsl API twice when calling
	 * mpsl_clock_hfclk_is_running() here.
	 * When this callback is running in ISR, it won't be preempted by other
	 * threads.
	 * If it is called in a thread context, the lock of mpsl is acquired in
	 * hf_clock_start().
	 */
	int key = irq_lock();
	u32_t is_running = 0;
	int errcode = mpsl_clock_hfclk_is_running(&is_running);

	if (errcode) {
		__ASSERT_NO_MSG(errcode == 0); /* it should never happen */
		irq_unlock(key);
		return;
	}

	sys_slist_t *list = &(clock_control_data.async_on_list);
	struct device *dev = DEVICE_GET(clock_nrf);

	if (is_running) {
		struct clock_control_async_data *clock_async_data = NULL;

		SYS_SLIST_FOR_EACH_CONTAINER(list, clock_async_data, node) {
			clock_async_data->cb(dev, clock_async_data->user_data);
		}
	}

	sys_slist_init(list);
	irq_unlock(key);
}
