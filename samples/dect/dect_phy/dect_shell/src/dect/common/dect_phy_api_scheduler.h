/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_API_SCHEDULER_H
#define DECT_PHY_API_SCHEDULER_H

#include <zephyr/kernel.h>
#include "dect_common.h"
#include "dect_phy_common.h"

/* Following defines the time how much in advance is scheduled to modem */
#define DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS 500
#define DECT_PHY_API_SCHEDULER_OP_MAX_COUNT	 30

#define DECT_SCHEDULER_DELAYED_ERROR		       6666
#define DECT_SCHEDULER_SCHEDULER_FATAL_MEM_ALLOC_ERROR 6667

/**************************************************************************************************/

/* HARQ part in scheduler */

enum dect_harq_user {
	DECT_HARQ_CLIENT = 0,
	DECT_HARQ_BEACON,
};

#define DECT_HARQ_FEEDBACK_TX_HANDLE 666
#define DECT_HARQ_FEEDBACK_RX_HANDLE 667

struct dect_harq_tx_payload_data {
	/* User is responsible for dealloc() */
	struct dect_phy_api_scheduler_list_item *sche_list_item;
	enum dect_harq_user harq_user;
};

/**************************************************************************************************/

/* Callback definitions from DECT PHY API scheduler */

typedef void (*dect_phy_api_scheduler_op_interval_count_callback_t)(uint32_t handle);

typedef void (*dect_phy_api_scheduler_op_completed_callback_t)(
	struct dect_phy_common_op_completed_params *params, uint64_t frame_time);
typedef void (*dect_phy_api_scheduler_op_pdc_received_callback_t)(uint64_t time, uint8_t *data,
								  uint32_t data_length,
								  int16_t rx_rssi_dbm,
								  int16_t rx_pwr_dbm);

/* HARQ: For storing TX for possible re-TX based on HARQ feedback */
typedef void (*dect_phy_api_scheduler_harq_tx_process_payload_store_callback_t)(
	struct dect_harq_tx_payload_data *harq_data);

/**************************************************************************************************/
typedef enum {
	DECT_PRIORITY_NONE,
	DECT_PRIORITY0_FORCE_TX, /* Possible to schedule past of the scheduler mdm TX window */
	DECT_PRIORITY0_FORCE_RX, /* Possible to schedule past of the scheduler mdm RX window */
	DECT_PRIORITY1_TX,
	DECT_PRIORITY1_RX,
	DECT_PRIORITY1_RX_RSSI,
	DECT_PRIORITY2_TX,
	DECT_PRIORITY2_RX,
	DECT_PRIORITY_LOWEST_RX
} dect_phy_api_scheduling_priority_t;

#define DECT_PHY_API_SCHEDULER_PRIORITY_IS_RX(x)                                                   \
	((x == DECT_PRIORITY1_RX) || (x == DECT_PRIORITY2_RX) || (x == DECT_PRIORITY_LOWEST_RX) || \
	 (x == DECT_PRIORITY0_FORCE_RX))
#define DECT_PHY_API_SCHEDULER_PRIORITY_IS_TX(x)                                                   \
	((x == DECT_PRIORITY1_TX) || (x == DECT_PRIORITY2_TX) || (x == DECT_PRIORITY0_FORCE_TX))
#define DECT_PHY_API_SCHEDULER_PRIORITY_IS_WITH_FORCE(x)                                           \
	((x == DECT_PRIORITY0_FORCE_TX) || (x == DECT_PRIORITY0_FORCE_RX))

struct dect_phy_api_scheduler_list_item_config_tx {
	dect_phy_header_type_t header_type;
	union nrf_modem_dect_phy_hdr phy_header;

	uint32_t phy_lbt_period;
	int8_t phy_lbt_rssi_threshold_max;

	uint16_t encoded_payload_pdu_size;
	uint8_t *encoded_payload_pdu;

	/* HARQ */
	bool harq_feedback_requested;
	enum dect_harq_user harq_user;
	dect_phy_api_scheduler_harq_tx_process_payload_store_callback_t cb_harq_tx_store;

	/* Combined TX & RX req  */
	bool combined_tx_rx_use;

	/* RX operation data structure must be fully filled.
	 * Note that RX start time is relative to TX end.
	 */
	struct nrf_modem_dect_phy_rx_params combined_rx_op;
};

struct dect_phy_api_scheduler_list_item_config_rx {
	uint32_t duration; /* Overwrites length_slots/length_subslots */
	int8_t expected_rssi_level;
	uint32_t network_id; /* Whole network id: 32bit */
	struct nrf_modem_dect_phy_rx_filter filter;
	enum nrf_modem_dect_phy_rx_mode mode;
};

struct dect_phy_api_scheduler_list_item_config_rx_rssi {
	struct nrf_modem_dect_phy_rssi_params rssi_op_params;
};

struct dect_phy_api_scheduler_list_item_config {
	dect_mac_address_info_t address_info;

	dect_phy_api_scheduler_op_completed_callback_t cb_op_to_mdm;

	dect_phy_api_scheduler_op_interval_count_callback_t
		cb_op_to_mdm_with_interval_count_completed;

	dect_phy_api_scheduler_op_completed_callback_t cb_op_completed;

	dect_phy_api_scheduler_op_interval_count_callback_t
		cb_op_completed_with_count; /* Note: use range to have unique handle  */

	dect_phy_api_scheduler_op_pdc_received_callback_t cb_pdc_received;

	uint16_t channel;

	uint32_t interval_mdm_ticks; /* If non-zero, scheduler re-schedules at next interval */
	int32_t interval_count_left; /* If -1: continuous, i.e. no limit. */

	/* Can be used only with interval_mdm_ticks is used */
	bool phy_op_handle_range_used;
	uint32_t phy_op_handle_range_start;
	uint32_t phy_op_handle_range_end;

	/** @brief scheduled frame in modem time (ticks) */
	uint64_t frame_time;

	/** @brief scheduled slot within a frame in modem time */
	bool subslot_used;     /* Default: false, slots used.*/
	uint8_t start_slot;    /* 0 = 1st slot */
	uint8_t start_subslot; /* 0 = 1st subslot */
	uint16_t length_slots;
	uint16_t length_subslots;

	struct dect_phy_api_scheduler_list_item_config_rx rx;        /* RX specifics */
	struct dect_phy_api_scheduler_list_item_config_tx tx;        /* TX specifics */
	struct dect_phy_api_scheduler_list_item_config_rx_rssi rssi; /* RSSI specifics */
};

struct dect_phy_api_scheduler_list_item {
	/* Common scheduler stuff to be filled */
	sys_dnode_t dnode;

	dect_phy_api_scheduling_priority_t priority;

	uint32_t phy_op_handle; /* Not applicable if range is used with repeating items */
	bool silent_fail;	/* If true, warnigs/errors are not printed by scheduler */

	struct dect_phy_api_scheduler_list_item_config sched_config;

	/* Private internals used by scheduler */
	bool stop_requested;
};

/**************************************************************************************************/

/* Scheduler list API */

struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_alloc_tx_element(
	struct dect_phy_api_scheduler_list_item_config **item_conf);
struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_alloc_rx_element(
	struct dect_phy_api_scheduler_list_item_config **item_conf);
struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_alloc_rssi_element(
	struct dect_phy_api_scheduler_list_item_config **item_conf);
void dect_phy_api_scheduler_list_item_dealloc(struct dect_phy_api_scheduler_list_item *list_item);
struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_create_new_copy(
	struct dect_phy_api_scheduler_list_item *item_to_be_copied, uint64_t new_frame_time,
	bool also_tx_data);
void dect_phy_api_scheduler_list_item_mem_copy(struct dect_phy_api_scheduler_list_item *target,
					       struct dect_phy_api_scheduler_list_item *source);

struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_list_item_add(struct dect_phy_api_scheduler_list_item *list_item);

struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_list_item_remove_by_phy_op_handle(uint32_t handle);

bool dect_phy_api_scheduler_list_item_running_by_phy_op_handle(uint32_t handle);

void dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle(uint32_t handle);
void dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle_range(uint32_t range_start,
									    uint32_t range_end);

void dect_phy_api_scheduler_th_list_item_remove_dealloc_by_phy_op_handle(uint32_t handle);

void dect_phy_api_scheduler_list_item_pdu_payload_update_by_phy_handle(
	uint32_t handle, uint8_t *new_encoded_payload_pdu, uint16_t size);

void dect_phy_api_scheduler_list_item_beacon_tx_sched_config_update_by_phy_op_handle(
	uint32_t handle, struct dect_phy_api_scheduler_list_item_config *tx_conf);

void dect_phy_api_scheduler_list_item_sched_config_frame_time_update_by_phy_op_handle(
	uint32_t handle, int64_t frame_time_diff);

void dect_phy_api_scheduler_list_item_beacon_rx_sched_config_update_by_phy_op_handle_range(
	uint16_t range_start, uint16_t range_end,
	struct dect_phy_api_scheduler_list_item_config *rx_conf);
void dect_phy_api_scheduler_list_item_tx_phy_header_update_by_phy_handle(
	uint32_t handle, union nrf_modem_dect_phy_hdr *phy_header,
	dect_phy_header_type_t header_type);

void dect_phy_api_scheduler_list_status_print(void);
void dect_phy_api_scheduler_list_delete_all_items(void); /* purge */

uint64_t dect_phy_api_scheduler_list_item_last_scheduled_modem_frame_time_get(void);

bool dect_phy_api_scheduler_list_is_empty(void);

/**************************************************************************************************/

/* Scheduler done list API */

bool dect_phy_api_scheduler_done_list_is_empty(void);

/**************************************************************************************************/

struct dect_phy_api_scheduler_op_pdc_type_rcvd_params {
	uint64_t time;

	int16_t rx_rssi_dbm;
	int16_t rx_pwr_dbm;

	uint32_t data_length;
	uint8_t data[DECT_DATA_MAX_LEN];
};

/* API to inform mdm operations */

int dect_phy_api_scheduler_mdm_op_completed(
	struct dect_phy_common_op_completed_params *params);
int dect_phy_api_scheduler_mdm_pdc_data_recv(
	struct dect_phy_api_scheduler_op_pdc_type_rcvd_params *rcv_data);

/**************************************************************************************************/

/* Scheduler control API */

int dect_phy_api_scheduler_suspend(void);
int dect_phy_api_scheduler_resume(void);

int dect_phy_api_scheduler_next_frame(void);

/**************************************************************************************************/

#endif /* DECT_PHY_API_SCHEDULER_H */
