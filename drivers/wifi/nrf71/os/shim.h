/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing OS interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __SHIM_H__
#define __SHIM_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_pkt.h>
#include <common/mem_mgmt.h>

/**
 * struct zep_shim_bus_qspi_priv - Structure to hold context information for the Linux OS
 *                        shim.
 * @pcie_callbk_data: Callback data to be passed to the PCIe callback functions.
 * @pcie_prb_callbk: The callback function to be called when a PCIe device
 *                   has been probed.
 * @pcie_rem_callbk: The callback function to be called when a PCIe device
 *                   has been removed.
 *
 * This structure maintains the context information necessary for the operation
 * of the Linux shim. Some of the elements of the structure need to be
 * initialized during the initialization of the Linux shim while others need to
 * be kept updated over the duration of the Linux shim operation.
 */
struct zep_shim_bus_qspi_priv {
	void *qspi_dev;

	bool dev_added;
	bool dev_init;
};

struct zep_shim_intr_priv {
	struct gpio_callback gpio_cb_data;
	void *callbk_data;
	int (*callbk_fn)(void *callbk_data);
	struct k_work_delayable work;
};

#endif /* __SHIM_H__ */
