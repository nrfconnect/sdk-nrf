/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_FHN_PF_H_
#define BT_FAST_PAIR_FHN_PF_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @defgroup bt_fast_pair_fhn_pf Precision Finding API for Fast Pair FHN
 * @brief The Precision Finding API for the Find Hub Network (FHN) extension
 *
 *  The API is only available when the @kconfig{CONFIG_BT_FAST_PAIR_FMDN_PF} Kconfig option
 *  is enabled.
 *
 *  It is required to use the Precision Finding API in the cooperative thread context.
 *  API function exceptions that do not follow this rule mention alternative requirements
 *  explicitly in their API documentation. Following the cooperative thread context
 *  requirement guarantees proper synchronization between the user operations and the
 *  module operations.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Ranging technology IDs. */
enum bt_fast_pair_fhn_pf_ranging_tech_id {
	/** Ranging technology ID for Ultra Wideband (UWB). */
	BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_UWB = 0,

	/** Ranging technology ID for Bluetooth Low Energy (BLE) Channel Sounding (CS). */
	BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS = 1,

	/** Ranging technology ID for Wi-Fi Neighbor Awareness Networking (NAN) Round-Trip Time
	 * (RTT).
	 */
	BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_WIFI_NAN_RTT = 2,

	/** Ranging technology ID for Bluetooth Low Energy (BLE) Received Signal Strength Indicator
	 *  (RSSI).
	 */
	BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI = 3,
};

/** @brief Check if a ranging technology is set in a bitmask.
 *
 *  This function can be called from any context.
 *
 *  @param bm Ranging technology bitmask.
 *  @param id Ranging technology ID to check.
 *
 *  @return true if the ranging technology is set in the bitmask, false otherwise.
 */
bool bt_fast_pair_fhn_pf_ranging_tech_bm_check(uint16_t bm,
					       enum bt_fast_pair_fhn_pf_ranging_tech_id id);

/** @brief Set or clear a ranging technology bit in a bitmask.
 *
 *  This function can be called from any context.
 *
 *  @param bm    Pointer to the ranging technology bitmask.
 *  @param id    Ranging technology ID to modify.
 *  @param value true to set the bit, false to clear it.
 */
void bt_fast_pair_fhn_pf_ranging_tech_bm_write(uint16_t *bm,
					       enum bt_fast_pair_fhn_pf_ranging_tech_id id,
					       bool value);

/** Technology-specific payload for a ranging technology. */
struct bt_fast_pair_fhn_pf_ranging_tech_payload {
	/** Ranging technology ID. */
	enum bt_fast_pair_fhn_pf_ranging_tech_id id;

	/** Length or size of the technology-specific payload data in bytes. This field describes
	 *  the length of the @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data buffer if this
	 *  structure is used as the input parameter. Otherwise, if it is used as the input/output
	 *  parameter, it describes the size of the buffer and is expected to be modified by the
	 *  function that encodes the payload.
	 *
	 *  This field never accounts for the length of the Ranging technology ID and Size fields.
	 */
	uint8_t data_len;

	/** Pointer to the technology-specific payload data buffer that has the
	 *  @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data_len length.
	 *
	 *  This buffer never includes the Ranging technology ID and Size fields that are the first
	 *  two bytes of technology-specific payload.
	 */
	uint8_t *data;
};

/** @brief Send a Ranging Capability Response message.
 *
 *  This function sends a Ranging Capability Response message over the Bluetooth GATT Beacon
 *  Actions characteristic to the connected peer. It should be called after receiving the
 *  @ref bt_fast_pair_fhn_pf_ranging_mgmt_cb.ranging_capability_request callback to indicate
 *  supported ranging technologies and their configuration payloads.

 *  The @p capability_payloads array must contain the technology-specific capability data for
 *  each ranging technology that is supported. The @p capability_payload_num parameter must
 *  indicate the number of elements in the @p capability_payloads array.

 *  The array order of the @p capability_payloads parameter determines the ranging technology
 *  priority. The first element in the array has the highest priority.
 *
 *  Calling this function in the wrong context, that is without the preceding ranging capability
 *  request callback, will result in an error. Indicating the ranging technologies in the
 *  @p ranging_tech_bm parameter that were not requested will also result in an error.
 *
 *  It is possible to call this function asynchronously outside of the callback context, as the
 *  capability preparation of certain ranging technologies may require time to complete.
 *
 *  @param conn                   Connection object.
 *  @param ranging_tech_bm        Bitmask of the supported ranging technologies, composed of
 *                                @ref bt_fast_pair_fhn_pf_ranging_tech_id bit positions.
 *  @param capability_payloads    Array of technology-specific capability payloads.
 *  @param capability_payload_num Number of elements in the @p capability_payloads array.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fhn_pf_ranging_capability_response_send(
	struct bt_conn *conn,
	uint16_t ranging_tech_bm,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payloads,
	uint8_t capability_payload_num);

/** @brief Send a Ranging Configuration Response message.
 *
 *  This function sends a Ranging Configuration Response message over the Bluetooth GATT Beacon
 *  Actions characteristic to the connected peer. It should be called after receiving the
 *  @ref bt_fast_pair_fhn_pf_ranging_mgmt_cb.ranging_config_request callback to confirm which
 *  ranging technologies were successfully configured and started.
 *
 *  Calling this function in the wrong context, that is without the preceding ranging
 *  configuration request callback, will result in an error. Indicating the ranging technologies in
 *  the @p ranging_tech_bm parameter that were not requested will also result in an error.
 *
 *  It is possible to call this function asynchronously outside of the callback context, as the
 *  configuration and starting of certain ranging technologies may require time to complete.
 *
 *  @param conn            Connection object.
 *  @param ranging_tech_bm Bitmask of the ranging technologies that were successfully configured
 *                         and started, composed of @ref bt_fast_pair_fhn_pf_ranging_tech_id bit
 *                         positions.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fhn_pf_ranging_config_response_send(struct bt_conn *conn,
						     uint16_t ranging_tech_bm);

/** @brief Send a Stop Ranging Response message.
 *
 *  This function sends a Stop Ranging Response message over the Bluetooth GATT Beacon Actions
 *  characteristic to the connected peer. It should be called after receiving the
 *  @ref bt_fast_pair_fhn_pf_ranging_mgmt_cb.stop_ranging_request callback to confirm which
 *  ranging technologies were successfully stopped.
 *
 *  Calling this function in the wrong context, that is without the preceding stop ranging
 *  request callback, will result in an error. Indicating the ranging technologies in the
 *  @p ranging_tech_bm parameter that were not requested will also result in an error.
 *
 *  It is possible to call this function asynchronously outside of the callback context, as the
 *  teardown of certain ranging technologies may require time to complete.
 *
 *  @param conn            Connection object.
 *  @param ranging_tech_bm Bitmask of the ranging technologies that were successfully stopped,
 *                         composed of @ref bt_fast_pair_fhn_pf_ranging_tech_id bit positions.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fhn_pf_stop_ranging_response_send(struct bt_conn *conn,
						   uint16_t ranging_tech_bm);

/** Ranging management callback structure. */
struct bt_fast_pair_fhn_pf_ranging_mgmt_cb {
	/** @brief Ranging Capability Request received.
	 *
	 *  This callback is mandatory. It is called when a Ranging Capability Request message
	 *  is received by the application from the connected peer. The application must determine
	 *  which of the requested ranging technologies it supports and then respond using the
	 *  @ref bt_fast_pair_fhn_pf_ranging_capability_response_send API. The response can be sent
	 *  asynchronously if the capability preparation of certain ranging technologies requires
	 *  time to complete.
	 *
	 *  This callback is executed in the cooperative thread context. You can learn about the
	 *  exact thread context by analyzing the @kconfig{CONFIG_BT_RECV_CONTEXT} Kconfig option.
	 *  By default, this callback is executed in the Bluetooth-specific workqueue thread
	 *  (@kconfig{CONFIG_BT_RECV_WORKQ_BT}).
	 *
	 *  @param conn            Connection object.
	 *  @param ranging_tech_bm Bitmask of the ranging technologies requested by the connected
	 *                         peer, composed of @ref bt_fast_pair_fhn_pf_ranging_tech_id bit
	 *                         positions.
	 */
	void (*ranging_capability_request)(struct bt_conn *conn, uint16_t ranging_tech_bm);

	/** @brief Ranging Configuration Request received.
	 *
	 *  This callback is mandatory. It is called when a Ranging Configuration message is
	 *  received by the application from the connected peer. The application must configure
	 *  the requested ranging technologies according to the provided technology-specific
	 *  configuration payloads, start the ranging sessions and then respond using the
	 *  @ref bt_fast_pair_fhn_pf_ranging_config_response_send API. The response can be sent
	 *  asynchronously if the configuration and starting of certain ranging technologies
	 *  requires time to complete.
	 *
	 *  The @p config_payloads array contains the technology-specific configuration data for
	 *  each ranging technology that must be configured and started. The @p config_payload_num
	 *  parameter indicates the number of elements in the @p config_payloads array.
	 *
	 *  This callback is executed in the cooperative thread context. You can learn about the
	 *  exact thread context by analyzing the @kconfig{CONFIG_BT_RECV_CONTEXT} Kconfig option.
	 *  By default, this callback is executed in the Bluetooth-specific workqueue thread
	 *  (@kconfig{CONFIG_BT_RECV_WORKQ_BT}).
	 *
	 *  @param conn               Connection object.
	 *  @param ranging_tech_bm    Bitmask of the ranging technologies to configure and start,
	 *                            composed of @ref bt_fast_pair_fhn_pf_ranging_tech_id bit
	 positions.
	 *  @param config_payloads    Array of technology-specific configuration payloads.
	 *  @param config_payload_num Number of elements in the @p config_payloads array.
	 */
	void (*ranging_config_request)(
		struct bt_conn *conn,
		uint16_t ranging_tech_bm,
		struct bt_fast_pair_fhn_pf_ranging_tech_payload *config_payloads,
		uint8_t config_payload_num);

	/** @brief Stop Ranging Request received.
	 *
	 *  This callback is mandatory. It is called when a Stop Ranging message is received from
	 *  the connected peer. The application must stop the specified ranging technologies and
	 *  respond using the @ref bt_fast_pair_fhn_pf_stop_ranging_response_send API.
	 *
	 *  The response can be sent asynchronously if the teardown of certain ranging technologies
	 *  requires time to complete. The @p ranging_tech_bm parameter contains the bitmask of the
	 *  ranging technologies to stop.
	 *
	 *  This callback is executed in the cooperative thread context. You can learn about the
	 *  exact thread context by analyzing the @kconfig{CONFIG_BT_RECV_CONTEXT} Kconfig option.
	 *  By default, this callback is executed in the Bluetooth-specific workqueue thread
	 *  (@kconfig{CONFIG_BT_RECV_WORKQ_BT}).
	 *
	 *  @param conn            Connection object.
	 *  @param ranging_tech_bm Bitmask of the ranging technologies to stop, composed of
	 *                         @ref bt_fast_pair_fhn_pf_ranging_tech_id bit positions.
	 */
	void (*stop_ranging_request)(struct bt_conn *conn, uint16_t ranging_tech_bm);

	/** @brief Communication channel terminated.
	 *
	 *  This callback is called when the communication channel for managing ranging has been
	 *  terminated. This occurs on disconnection of the Bluetooth connection or when the
	 *  Precision Finding module is disabled.
	 *
	 *  This callback is optional and the application may use it to implement the ranging
	 *  timeout and clean up the ranging state associated with this connection. If any ranging
	 *  technologies are still active, the application is responsible for stopping them.
	 *
	 *  This callback is by default executed in the cooperative thread context as part of
	 *  of the delayable work item defined in the system workqueue. The cooperative
	 *  properties of this callback depend on the @kconfig{CONFIG_SYSTEM_WORKQUEUE_PRIORITY}
	 *  Kconfig option.
	 *
	 *  @param conn Connection object.
	 */
	void (*comm_channel_terminated)(struct bt_conn *conn);
};

/** @brief Register ranging management callbacks.
 *
 *  This function registers the ranging management callbacks that are used to communicate with
 *  the peer device to negotiate supported ranging technologies, and to configure, start or
 *  stop a ranging session.
 *
 *  If the @kconfig{CONFIG_BT_FAST_PAIR_FMDN_PF} Kconfig option is enabled, you must call this API
 *  before you enable Fast Pair with the @ref bt_fast_pair_enable function. Otherwise, the enable
 *  operation fails.
 *
 *  All callbacks in the @ref bt_fast_pair_fhn_pf_ranging_mgmt_cb structure, except for the
 *  @ref bt_fast_pair_fhn_pf_ranging_mgmt_cb.comm_channel_terminated callback, must be non-NULL.
 *
 *  You can call this function only in the disabled state of the Fast Pair module (see
 *  @ref bt_fast_pair_is_ready function).
 *
 *  This function must be called in the cooperative thread context or in the system initialization
 *  context (SYS_INIT macro).
 *
 *  @param cb Ranging management callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fhn_pf_ranging_mgmt_cb_register(
	const struct bt_fast_pair_fhn_pf_ranging_mgmt_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_FHN_PF_H_ */
