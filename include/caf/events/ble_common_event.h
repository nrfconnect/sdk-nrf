/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_COMMON_EVENT_H_
#define _BLE_COMMON_EVENT_H_

/**
 * @file
 * @defgroup caf_ble_common_event CAF Bluetooth LE Common Event
 * @{
 * @brief CAF Bluetooth LE Common Event.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Peer states.
 *
 * State of the Bluetooth connected peer.
 */
enum peer_state {
	/** Bluetooth stack disconnected from the remote peer. */
	PEER_STATE_DISCONNECTED,

	/** An application module called bt_conn_disconnect() to disconnect the remote peer.
	 *  This state can be reported by the module that requests peer disconnection to let other
	 *  application modules prepare for the planned disconnection.
	 */
	PEER_STATE_DISCONNECTING,

	/** Bluetooth stack successfully connected to the remote peer. */
	PEER_STATE_CONNECTED,

	/** Bluetooth stack set the connection security to at least level 2 (that is, encryption and
	 *  no authentication).
	 */
	PEER_STATE_SECURED,

	/** Bluetooth stack failed to connect the remote peer. */
	PEER_STATE_CONN_FAILED,

	/** Number of peer states. */
	PEER_STATE_COUNT,

	/** Unused in code, required for inter-core compatibility. */
	APP_EM_ENFORCE_ENUM_SIZE(PEER_STATE)
};

/** @brief Peer operations.
 */
enum peer_operation {
	/** The peer change is triggered and waiting for confirmation. The operation is not
	 *  performed by the Bluetooth stack before the selection is confirmed.
	 *  PEER_OPERATION_SELECTED should be used for confirmed operation.
	 */
	PEER_OPERATION_SELECT,

	/** The peer change is confirmed. That results in swapping used local identity. The peers
	 *  that are connected to previously used Bluetooth local identity should be disconnected.
	 *  The application can start peer search for peers related to a new local identity.
	 */
	PEER_OPERATION_SELECTED,

	/** Bluetooth scanning is requested. */
	PEER_OPERATION_SCAN_REQUEST,

	/** The bond erase is triggered and waiting for confirmation. The operation is not performed
	 *  by the Bluetooth stack before the erase is confirmed. PEER_OPERATION_ERASED should be
	 *  used for a confirmed operation.
	 */
	PEER_OPERATION_ERASE,

	/** Trigger erase advertising for selected local identity. */
	PEER_OPERATION_ERASE_ADV,

	/** Cancel ongoing erase advertising. */
	PEER_OPERATION_ERASE_ADV_CANCEL,

	/** The bond erase is confirmed. That results in removing all of the Bluetooth bonds of
	 *  related local identity.
	 */
	PEER_OPERATION_ERASED,

	/** Cancel ongoing peer operation. If PEER_OPERATION_ERASE_ADV is cancelled,
	 *  PEER_OPERATION_ERASE_ADV_CANCEL must be used instead.
	 */
	PEER_OPERATION_CANCEL,

	/** Number of peer operations. */
	PEER_OPERATION_COUNT,

	/** Unused in code, required for inter-core compatibility. */
	APP_EM_ENFORCE_ENUM_SIZE(PEER_OPERATION)
};

/** @brief Bluetooth LE peer event.
 *
 * The Bluetooth LE peer event is submitted to inform about state change of a Bluetooth connected
 * peer.
 */
struct ble_peer_event {
	/** Event header. */
	struct app_event_header header;

	/** State of the Bluetooth LE peer. */
	enum peer_state state;

	/** Reason code related to state of the Bluetooth LE peer. The field is used only for
	 *  PEER_STATE_CONN_FAILED (error code related to connection establishment failure),
	 *  PEER_STATE_DISCONNECTING, PEER_STATE_DISCONNECTED (disconnection reason). Other peer
	 *  states do not use this field and set the value to zero.
	 */
	uint8_t reason;

	/** ID used to identify Bluetooth connection - pointer to the bt_conn. */
	void *id;
};

/** @brief Bluetooth LE peer operation event.
 *
 * The Bluetooth LE peer operation event informs about operation that is performed on Bluetooth
 * peers related to a given Bluetooth local identity (and application local identity).
 *
 * For detailed information related to local identities, peer and bond management, see documentation
 * of the Bluetooth LE bond module in nRF Desktop. The Bluetooth LE bond module is an example of
 * application-specific module that implements Bluetooth peer and bond management.
 */
struct ble_peer_operation_event {
	/** Event header. */
	struct app_event_header header;

	/** Bluetooth LE peer operation. */
	enum peer_operation op;

	/** Local identity related to the peer operation (used by the application). */
	uint8_t bt_app_id;

	/** Bluetooth local identity related to the peer operation (used by the Bluetooth stack). */
	uint8_t bt_stack_id;
};

/** @brief Bluetooth LE connection parameters event.
 *
 * The Bluetooth LE connection parameters event is submitted to inform either about received
 * Bluetooth connection parameter update request or fact that connection parameters were already
 * updated.
 *
 * If the event is related to fact that the connection parameters were already updated, values
 * of interval_min and interval_max are the same. They are equal to the connection interval that
 * is in use.
 */
struct ble_peer_conn_params_event {
	/** Event header. */
	struct app_event_header header;

	/** ID used to identify Bluetooth connection - pointer to the bt_conn. */
	void *id;

	/** Minimum Connection Interval (N * 1.25 ms). */
	uint16_t interval_min;

	/** Maximum Connection Interval (N * 1.25 ms). */
	uint16_t interval_max;

	/** Connection Latency. */
	uint16_t latency;

	/** Supervision Timeout (N * 10 ms). */
	uint16_t timeout;

	/** Information if the event is related to the fact that connection parameters were already
	 *  updated or connection parameter update request was received.
	 */
	bool updated;
};

/** @brief Bluetooth LE peer search event.
 *
 * The Bluetooth LE peer search event is submitted to inform if application is currently looking for
 * a Bluetooth peer. The event can be related either to Bluetooth scanning or advertising.
 */
struct ble_peer_search_event {
	/** Event header. */
	struct app_event_header header;

	/** Information if application is currently looking for a Bluetooth peer. */
	bool active;
};

/** @brief Bluetooth LE advertising data update event.
 *
 * The Bluetooth LE advertising data update event is submitted to trigger update of advertising data
 * and scan response data.
 */
struct ble_adv_data_update_event {
	/** Event header. */
	struct app_event_header header;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

APP_EVENT_TYPE_DECLARE(ble_peer_event);
APP_EVENT_TYPE_DECLARE(ble_peer_operation_event);
APP_EVENT_TYPE_DECLARE(ble_peer_conn_params_event);
APP_EVENT_TYPE_DECLARE(ble_peer_search_event);
APP_EVENT_TYPE_DECLARE(ble_adv_data_update_event);

#ifdef __cplusplus
}
#endif

#endif /* _BLE_COMMON_EVENT_H_ */
