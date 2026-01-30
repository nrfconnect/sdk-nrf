/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file
 *
 * @brief Header containing structure declarations for the HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_STRUCTS_COMMON_H__
#define __HAL_STRUCTS_COMMON_H__

#include <nrf71_wifi_ctrl.h>
#include "osal_api.h"
#include "bal_api.h"

/** 1 sec */
#define MAX_HAL_RPU_READY_WAIT (1 * 1000 * 1000)

#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
#define RPU_PS_WAKE_INTERVAL_MS 1
#define RPU_PS_WAKE_TIMEOUT_S 1
#endif /* NRF_WIFI_LOW_POWER */

/**
 * @brief Enumeration of RPU processor types.
 */
enum RPU_PROC_TYPE {
	/** MCU LMAC processor type */
	RPU_PROC_TYPE_MCU_LMAC = 0,
	/** MCU UMAC processor type */
	RPU_PROC_TYPE_MCU_UMAC,
	/** Maximum number of processor types */
	RPU_PROC_TYPE_MAX
};

/**
 * @brief Convert RPU_PROC_TYPE enum to string.
 *
 * @param proc The RPU_PROC_TYPE enum value.
 * @return The string representation of the RPU_PROC_TYPE.
 */
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
};
/**
 * @brief Enumeration of NRF Wi-Fi region types.
 */
enum NRF_WIFI_REGION_TYPE {
	/** GRAM region type */
	NRF_WIFI_REGION_TYPE_GRAM,
	/** PKTRAM region type */
	NRF_WIFI_REGION_TYPE_PKTRAM,
	/** SYSBUS region type */
	NRF_WIFI_REGION_TYPE_SYSBUS,
	/** PBUS region type */
	NRF_WIFI_REGION_TYPE_PBUS,
};

/**
 * @brief Enumeration of NRF Wi-Fi HAL message types.
 */
enum NRF_WIFI_HAL_MSG_TYPE {
	/** Command control message type */
	NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL,
	/** Event message type */
	NRF_WIFI_HAL_MSG_TYPE_EVENT,
	/** Command data RX message type */
	NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX,
	/** Command data management message type */
	NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_MGMT,
	/** Command data TX message type */
	NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX,
	/** Maximum number of message types */
	NRF_WIFI_HAL_MSG_TYPE_MAX,
};

#if defined(NRF_WIFI_LOW_POWER)  || defined(__DOXYGEN__)
/**
 * @brief Enumeration of RPU power states.
 */
enum RPU_PS_STATE {
		/** RPU is asleep */
	RPU_PS_STATE_ASLEEP,
	/** RPU is awake */
	RPU_PS_STATE_AWAKE,
	/** Maximum number of power states */
	RPU_PS_STATE_MAX
};
#endif /* NRF_WIFI_LOW_POWER */

/**
 * @brief Enumeration of NRF WiFi HAL status.
 */
enum NRF_WIFI_HAL_STATUS {
	/** HAL is enabled */
	NRF_WIFI_HAL_STATUS_ENABLED,
	/** HAL is disabled */
	NRF_WIFI_HAL_STATUS_DISABLED,
};


/**
 * @brief Structure to hold buffer mapping information for the HAL layer.
 */
struct nrf_wifi_hal_buf_map_info {
	/** Flag indicating if the buffer is mapped */
	bool mapped;
	/** Virtual address of the buffer */
	unsigned long virt_addr;
	/** Physical address of the buffer */
	unsigned long phy_addr;
	/** Length of the buffer */
	unsigned int buf_len;
};

/**
 * @brief Structure to hold configuration parameters for the HAL layer
 * in all modes of operation.
 */
struct nrf_wifi_hal_cfg_params {
	/** Maximum command size */
	unsigned int max_cmd_size;
	/** Maximum event size */
	unsigned int max_event_size;
	/** RX buffer headroom size */
	unsigned char rx_buf_headroom_sz;
	/** TX buffer headroom size */
	unsigned char tx_buf_headroom_sz;
#if defined(NRF71_DATA_TX)  || defined(__DOXYGEN__)
	/** Maximum TX frames */
	unsigned int max_tx_frms;
#endif /* CONFIG_NRF71_DATA_TX */
	/** RX buffer pool parameters */
	struct rx_buf_pool_params rx_buf_pool[MAX_NUM_OF_RX_QUEUES];
	/** Maximum TX frame size */
	unsigned int max_tx_frm_sz;
	/** Maximum AMPDU length per token */
	unsigned int max_ampdu_len_per_token;
};


/**
 * @brief Structure to hold context information for the HAL layer.
 */
struct nrf_wifi_hal_priv {
	/** Pointer to BAL private data */
	struct nrf_wifi_bal_priv *bpriv;
	/** Number of devices */
	unsigned char num_devs;
	/** Additional device callback data */
	void *add_dev_callbk_data;
	/** Add device callback function */
	void *(*add_dev_callbk_fn)(void *add_dev_callbk_data,
				   void *hal_dev_ctx);
	/** Remove device callback function */
	void (*rem_dev_callbk_fn)(void *mac_ctx);
	/** Initialize device callback function */
	enum nrf_wifi_status (*init_dev_callbk_fn)(void *mac_ctx);
	/** Deinitialize device callback function */
	void (*deinit_dev_callbk_fn)(void *mac_ctx);
	/** Interrupt callback function */
	enum nrf_wifi_status (*intr_callbk_fn)(void *mac_ctx,
					       void *event_data,
					       unsigned int len);
	/** RPU recovery callback function */
	enum nrf_wifi_status (*rpu_recovery_callbk_fn)(void *mac_ctx,
						       void *event_data,
						       unsigned int len);
	struct nrf_wifi_hal_cfg_params cfg_params;
	/** PKTRAM base address */
	unsigned long addr_pktram_base;
};

/**
 * @brief Structure to hold per device context information for the HAL layer.
 */
struct nrf_wifi_hal_dev_ctx {
	/** Pointer to HAL private data */
	struct nrf_wifi_hal_priv *hpriv;
	/** MAC device context */
	void *mac_dev_ctx;
	/** BAL device context */
	void *bal_dev_ctx;
	/** Device index */
	unsigned char idx;
	/** RPU information */
	void *ipc_msg;
	/** Number of commands */
	unsigned int num_cmds;
	/** Command queue */
	void *cmd_q;
	/** Event queue */
	void *event_q;
	/** Current RPU processor type:
	 * This is only used during FW loading where we need the information
	 * about the processor whose core memory the code/data needs to be
	 * loaded into (since the FW image parser does not have the processor
	 * information coded into it). Necessitated due to FullMAC
	 * configuration where the RPU has 2 MCUs.
	 */
	enum RPU_PROC_TYPE curr_proc;
	/** HAL lock */
	void *lock_hal;
	/** Event tasklet */
	void *event_tasklet;
	/** RX lock */
	void *lock_rx;
	/** RX buffer information */
	struct nrf_wifi_hal_buf_map_info *rx_buf_info[MAX_NUM_OF_RX_QUEUES];
	/** TX buffer information */
	struct nrf_wifi_hal_buf_map_info *tx_buf_info;
	/** RPU PKTRAM base address */
	unsigned long addr_rpu_pktram_base;
	/** RPU PKTRAM base address for TX */
	unsigned long addr_rpu_pktram_base_tx;
	/** RPU PKTRAM base address for RX */
	unsigned long addr_rpu_pktram_base_rx;
	/** RPU PKTRAM base address for RX pool */
	unsigned long addr_rpu_pktram_base_rx_pool[MAX_NUM_OF_RX_QUEUES];
	/** TX frame offset */
	unsigned long tx_frame_offset;
#if defined(NRF_WIFI_RPU_RECOVERY)  || defined(__DOXYGEN__)
	/** RPU wake up now asserted flag */
	bool is_wakeup_now_asserted;
	/** RPU wake up now asserted time */
	unsigned long last_wakeup_now_asserted_time_ms;
	/** RPU wake up now asserted time */
	unsigned long last_wakeup_now_deasserted_time_ms;
	/** RPU sleep opp time */
	unsigned long last_rpu_sleep_opp_time_ms;
	/** Number of watchdog timer interrupts received */
	int wdt_irq_received;
	/** Number of watchdog timer interrupts ignored */
	int wdt_irq_ignored;
#endif /* NRF_WIFI_RPU_RECOVERY */
#if defined(NRF_WIFI_LOW_POWER)  || defined(__DOXYGEN__)
	/** RPU power state */
	enum RPU_PS_STATE rpu_ps_state;
	/** RPU power state timer */
	void *rpu_ps_timer;
	/** RPU power state lock */
	void *rpu_ps_lock;
	/** Debug enable flag */
	bool dbg_enable;
	/** IRQ context flag */
	bool irq_ctx;
	/** RPU firmware booted flag */
	bool rpu_fw_booted;
#endif /* NRF_WIFI_LOW_POWER */
	/** Event data */
	char *event_data;
	/** Current event data */
	char *event_data_curr;
	/** Event data length */
	unsigned int event_data_len;
	/** Pending event data */
	unsigned int event_data_pending;
	/** Event resubmit flag */
	unsigned int event_resubmit;
	/** HAL status */
	enum NRF_WIFI_HAL_STATUS hal_status;
	/** Recovery tasklet */
	void *recovery_tasklet;
	/** Recovery lock */
	void *lock_recovery;
};

/**
 * @brief Structure to hold information about a HAL message.
 */
struct nrf_wifi_hal_msg {
	/** Length of the message */
	unsigned int len;
	/** Message data */
	char data[0];
};
#endif /* __HAL_STRUCTS_COMMON_H__ */
