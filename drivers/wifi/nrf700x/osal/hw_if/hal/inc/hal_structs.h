/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing structure declarations for the
 * HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_STRUCTS_H__
#define __HAL_STRUCTS_H__

#include <stdbool.h>
#include "lmac_if_common.h"
#include "host_rpu_common_if.h"
#include "osal_api.h"
#include "bal_api.h"

#define MAX_HAL_RPU_READY_WAIT (1 * 1000 * 1000) /* 1 sec */

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#define RPU_PS_WAKE_INTERVAL_MS 1
#define RPU_PS_WAKE_TIMEOUT_S 1
#endif /* CONFIG_NRF_WIFI_LOW_POWER */


enum RPU_PROC_TYPE {
	RPU_PROC_TYPE_MCU_LMAC = 0,
	RPU_PROC_TYPE_MCU_UMAC,
	RPU_PROC_TYPE_MAX
};

static inline const char *rpu_proc_to_str(enum RPU_PROC_TYPE proc)
{
	switch (proc) {
	case RPU_PROC_TYPE_MCU_LMAC:
		return "LMAC";
	case RPU_PROC_TYPE_MCU_UMAC:
		return "UMAC";
	default:
		return "UNKNOWN";
	}
}


enum NRF_WIFI_REGION_TYPE {
	NRF_WIFI_REGION_TYPE_GRAM,
	NRF_WIFI_REGION_TYPE_PKTRAM,
	NRF_WIFI_REGION_TYPE_SYSBUS,
	NRF_WIFI_REGION_TYPE_PBUS,
};


enum NRF_WIFI_HAL_MSG_TYPE {
	NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL,
	NRF_WIFI_HAL_MSG_TYPE_EVENT,
	NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX,
	NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_MGMT,
	NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX,
	NRF_WIFI_HAL_MSG_TYPE_MAX,
};

#ifdef CONFIG_NRF_WIFI_LOW_POWER
enum RPU_PS_STATE {
	RPU_PS_STATE_ASLEEP,
	RPU_PS_STATE_AWAKE,
	RPU_PS_STATE_MAX
};
#endif /* CONFIG_NRF_WIFI_LOW_POWER */


struct nrf_wifi_hal_cfg_params {
	unsigned int max_cmd_size;
	unsigned int max_event_size;

#ifndef CONFIG_NRF700X_RADIO_TEST
	unsigned char rx_buf_headroom_sz;
	unsigned char tx_buf_headroom_sz;
#ifdef CONFIG_NRF700X_DATA_TX
	unsigned int max_tx_frms;
#endif /* CONFIG_NRF700X_DATA_TX */
	struct rx_buf_pool_params rx_buf_pool[MAX_NUM_OF_RX_QUEUES];
	unsigned int max_tx_frm_sz;
	unsigned int max_ampdu_len_per_token;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
};


/**
 * struct nrf_wifi_hal_priv - Structure to hold context information for the
 *                           HAL layer.
 * @opriv: Pointer to the OS abstraction layer.
 * @bpriv: Pointer to the Bus abstraction layer.
 * @add_dev_callbk_data: Data to be passed back when invoking @add_dev_callbk_fn.
 * @add_dev_callbk_fn: Callback function to be called when a new device is being added.
 * @rem_dev_callbk_fn: Callback function to be called when a device is being removed.
 * @init_dev_callbk_fn: Callback function to be called when a device is being initialised.
 * @deinit_dev_callbk_fn: Callback function to be called when a device is being deinitialised.
 * @intr_callback_fn: Pointer to the function which needs to be called when an
 *                    interrupt is received.
 * @max_cmd_size: Maximum size of the command that can be sent to the
 *                Firmware.
 *
 * This structure maintains the context information necessary for the
 * operation of the HAL layer.
 */
struct nrf_wifi_hal_priv {
	struct nrf_wifi_osal_priv *opriv;
	struct nrf_wifi_bal_priv *bpriv;
	unsigned char num_devs;

	void *add_dev_callbk_data;
	void *(*add_dev_callbk_fn)(void *add_dev_callbk_data,
				    void *hal_dev_ctx);
	void (*rem_dev_callbk_fn)(void *mac_ctx);

	enum nrf_wifi_status (*init_dev_callbk_fn)(void *mac_ctx);
	void (*deinit_dev_callbk_fn)(void *mac_ctx);

	enum nrf_wifi_status (*intr_callbk_fn)(void *mac_ctx,
					       void *event_data,
					       unsigned int len);
	struct nrf_wifi_hal_cfg_params cfg_params;
	unsigned long addr_pktram_base;
};


/**
 * struct nrf_wifi_hal_info - Structure to hold RPU information.
 * @hpqm_info: HPQM queue(s) related information.
 * @rx_cmd_base: The base address for posting RX commands.
 * @tx_cmd_base: The base address for posting TX commands.
 *
 * This structure contains RPU related information needed by the
 * HAL layer.
 */
struct nrf_wifi_hal_info {
	struct host_rpu_hpqm_info hpqm_info;
	unsigned int rx_cmd_base;
	unsigned int tx_cmd_base;
};


struct nrf_wifi_hal_buf_map_info {
	bool mapped;
	unsigned long virt_addr;
	unsigned long phy_addr;
	unsigned int buf_len;
};


/**
 * struct nrf_wifi_hal_dev_ctx - Structure to hold per device context information
 *                              for the HAL layer.
 * @hpriv: Pointer to the HAL abstraction layer.
 * @idx: The index of the HAL instantiation (the instance of the device to
 *       which this HAL instance is associated to)
 * @mac_ctx: Pointer to the per device MAC context which is using the HAL layer.
 * @dev_ctx: Pointer to the per device BUS context which is being used by the
 *           HAL layer.
 * @rpu_info: RPU specific information necessary for the operation
 *            of the HAL.
 * @num_cmds: Debug counter for number of commands sent by the host to the RPU.
 * @cmd_q: Queue to hold commands before they are sent to the RPU.
 * @event_q: Queue to hold events received from the RPU before they are
 *           processed by the host.
 * @curr_proc: The RPU MCU whose context is active for a given HAL operation.
 *             This is needed only during FW loading and not necessary during
 *             the regular operation after the FW has been loaded.
 * @lock_hal: Lock to be used for atomic HAL operations.
 * @lock_rx: Lock to be used for atomic RX operations.
 * @event_tasklet: Pointer to the bottom half handler for RX events.
 * @rpu_ps_state: PS state of the RPU.
 * @rpu_ps_timer: Inactivity timer used to put RPU back to sleep after
 *                waking it up.
 * @rpu_ps_lock: Lock to be used for atomic RPU PS operations.
 * @num_isrs: Debug counter for number of interrupts received from the RPU.
 * @num_events: Debug counter for number of events received from the RPU.
 * @num_events_resubmit: Debug counter for number of event pointers
 *                       resubmitted back to the RPU.
 *
 * This structure maintains the context information necessary for the
 * operation of the HAL. Some of the elements of the structure need to be
 * initialized durign the initialization of the driver while others need to
 * be kept updated over the duration of the driver operation.
 */
struct nrf_wifi_hal_dev_ctx {
	struct nrf_wifi_hal_priv *hpriv;
	void *mac_dev_ctx;
	void *bal_dev_ctx;
	unsigned char idx;

	struct nrf_wifi_hal_info rpu_info;

	unsigned int num_cmds;

	void *cmd_q;
	void *event_q;

	/* This is only used during FW loading where we need the information
	 * about the processor whose core memory the code/data needs to be
	 * loaded into (since the FW image parser does not have the processor
	 * information coded into it). Necessitated due to FullMAC
	 * configuration where the RPU has 2 MCUs.
	 */
	enum RPU_PROC_TYPE curr_proc;

	void *lock_hal;
	void *event_tasklet;
	void *lock_rx;

	struct nrf_wifi_hal_buf_map_info *rx_buf_info[MAX_NUM_OF_RX_QUEUES];
	struct nrf_wifi_hal_buf_map_info *tx_buf_info;

	unsigned long addr_rpu_pktram_base;
	unsigned long addr_rpu_pktram_base_tx;
	unsigned long addr_rpu_pktram_base_rx;
	unsigned long addr_rpu_pktram_base_rx_pool[MAX_NUM_OF_RX_QUEUES];
	unsigned long tx_frame_offset;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	enum RPU_PS_STATE rpu_ps_state;
	void *rpu_ps_timer;
	void *rpu_ps_lock;
	bool dbg_enable;
	bool irq_ctx;
	bool rpu_fw_booted;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	char *event_data;
	char *event_data_curr;
	unsigned int event_data_len;
	unsigned int event_data_pending;
	unsigned int event_resubmit;
};


/**
 * struct nrf_wifi_hal_msg - Structure to hold information about a HAL message.
 * @len: Length of the HAL message.
 * @data: Pointer to the buffer containing the HAL message.
 *
 * This structure contains information about a HAL message (command/event).
 */
struct nrf_wifi_hal_msg {
	unsigned int len;
	char data[0];
};
#endif /* __HAL_STRUCTS_H__ */
