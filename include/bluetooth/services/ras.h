/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RAS_H_
#define BT_RAS_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci_types.h>
#include <bluetooth/gatt_dm.h>

/** @file
 *  @defgroup bt_ras Ranging Service API
 *  @{
 *  @brief API for the Ranging Service (RAS).
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the Ranging Service. **/
#define BT_UUID_RANGING_SERVICE_VAL (0x185B)

/** @brief UUID of the RAS Features Characteristic. **/
#define BT_UUID_RAS_FEATURES_VAL (0x2C14)

/** @brief UUID of the Real-time Ranging Data Characteristic. **/
#define BT_UUID_RAS_REALTIME_RD_VAL (0x2C15)

/** @brief UUID of the On-demand Ranging Data Characteristic. **/
#define BT_UUID_RAS_ONDEMAND_RD_VAL (0x2C16)

/** @brief UUID of the RAS Control Point Characteristic. **/
#define BT_UUID_RAS_CP_VAL (0x2C17)

/** @brief UUID of the Ranging Data Ready Characteristic. **/
#define BT_UUID_RAS_RD_READY_VAL (0x2C18)

/** @brief UUID of the Ranging Data Overwritten Characteristic. **/
#define BT_UUID_RAS_RD_OVERWRITTEN_VAL (0x2C19)

#define BT_UUID_RANGING_SERVICE    BT_UUID_DECLARE_16(BT_UUID_RANGING_SERVICE_VAL)
#define BT_UUID_RAS_FEATURES       BT_UUID_DECLARE_16(BT_UUID_RAS_FEATURES_VAL)
#define BT_UUID_RAS_REALTIME_RD    BT_UUID_DECLARE_16(BT_UUID_RAS_REALTIME_RD_VAL)
#define BT_UUID_RAS_ONDEMAND_RD    BT_UUID_DECLARE_16(BT_UUID_RAS_ONDEMAND_RD_VAL)
#define BT_UUID_RAS_CP             BT_UUID_DECLARE_16(BT_UUID_RAS_CP_VAL)
#define BT_UUID_RAS_RD_READY       BT_UUID_DECLARE_16(BT_UUID_RAS_RD_READY_VAL)
#define BT_UUID_RAS_RD_OVERWRITTEN BT_UUID_DECLARE_16(BT_UUID_RAS_RD_OVERWRITTEN_VAL)

#define BT_RAS_RANGING_HEADER_LEN  4
#define BT_RAS_SUBEVENT_HEADER_LEN 8
#define BT_RAS_STEP_MODE_LEN       1

#define BT_RAS_MAX_SUBEVENTS_PER_PROCEDURE 32
#define BT_RAS_MAX_STEPS_PER_PROCEDURE     256

#define BT_RAS_STEP_MODE_2_3_ANT_DEPENDENT_LEN(antenna_paths)                                      \
	((antenna_paths + 1) * sizeof(struct bt_hci_le_cs_step_data_tone_info))

#define BT_RAS_STEP_MODE_0_MAX_LEN                                                                 \
	MAX(sizeof(struct bt_hci_le_cs_step_data_mode_0_initiator),                                \
	    sizeof(struct bt_hci_le_cs_step_data_mode_0_reflector))
#define BT_RAS_STEP_MODE_1_MAX_LEN (sizeof(struct bt_hci_le_cs_step_data_mode_1))
#define BT_RAS_STEP_MODE_2_MAX_LEN                                                                 \
	(sizeof(struct bt_hci_le_cs_step_data_mode_2) +                                            \
	 BT_RAS_STEP_MODE_2_3_ANT_DEPENDENT_LEN(CONFIG_BT_RAS_MAX_ANTENNA_PATHS))
#define BT_RAS_STEP_MODE_3_MAX_LEN                                                                 \
	(sizeof(struct bt_hci_le_cs_step_data_mode_3) +                                            \
	 BT_RAS_STEP_MODE_2_3_ANT_DEPENDENT_LEN(CONFIG_BT_RAS_MAX_ANTENNA_PATHS))

#define BT_RAS_STEP_MODE_0_1_MAX_LEN   MAX(BT_RAS_STEP_MODE_0_MAX_LEN, BT_RAS_STEP_MODE_1_MAX_LEN)
#define BT_RAS_STEP_MODE_0_1_2_MAX_LEN MAX(BT_RAS_STEP_MODE_0_1_MAX_LEN, BT_RAS_STEP_MODE_2_MAX_LEN)

#if defined(CONFIG_BT_RAS_MODE_3_SUPPORTED)
#define BT_RAS_MAX_STEP_DATA_LEN MAX(BT_RAS_STEP_MODE_0_1_2_MAX_LEN, BT_RAS_STEP_MODE_3_MAX_LEN)
#else
#define BT_RAS_MAX_STEP_DATA_LEN BT_RAS_STEP_MODE_0_1_2_MAX_LEN
#endif

#define BT_RAS_PROCEDURE_MEM                                                                       \
	(BT_RAS_RANGING_HEADER_LEN +                                                               \
	 (BT_RAS_MAX_SUBEVENTS_PER_PROCEDURE * BT_RAS_SUBEVENT_HEADER_LEN) +                       \
	 (BT_RAS_MAX_STEPS_PER_PROCEDURE * BT_RAS_STEP_MODE_LEN) +                                 \
	 (BT_RAS_MAX_STEPS_PER_PROCEDURE * BT_RAS_MAX_STEP_DATA_LEN))

/** @brief Ranging Header structure as defined in RAS Specification, Table 3.7. */
struct ras_ranging_header {
	/** Ranging Counter is lower 12-bits of CS Procedure_Counter provided by the Core Controller
	 *  (Core Specification, Volume 4, Part E, Section 7.7.65.44).
	 */
	uint16_t ranging_counter : 12;
	/** CS configuration identifier. Range: 0 to 3. */
	uint8_t  config_id       : 4;
	/** Transmit power level used for the CS Procedure. Range: -127 to 20. Units: dBm. */
	int8_t   selected_tx_power;
	/** Antenna paths that are reported:
	 *  Bit0: 1 if Antenna Path_1 included; 0 if not.
	 *  Bit1: 1 if Antenna Path_2 included; 0 if not.
	 *  Bit2: 1 if Antenna Path_3 included; 0 if not.
	 *  Bit3: 1 if Antenna Path_4 included; 0 if not.
	 *  Bits 4-7: RFU
	 */
	uint8_t  antenna_paths_mask;
} __packed;
BUILD_ASSERT(sizeof(struct ras_ranging_header) == BT_RAS_RANGING_HEADER_LEN);

/** @brief Subevent Header structure as defined in RAS Specification, Table 3.8. */
struct ras_subevent_header {
	/** Starting ACL connection event count for the results reported in the event */
	uint16_t start_acl_conn_event;
	/** Frequency compensation value in units of 0.01 ppm (15-bit signed integer).
	 *  Note this value can be BT_HCI_LE_CS_SUBEVENT_RESULT_FREQ_COMPENSATION_NOT_AVAILABLE
	 *  if the role is not the initiator, or the frequency compensation value is unavailable.
	 */
	uint16_t freq_compensation;
	/** Ranging Done Status:
	 *  0x0: All results complete for the CS Procedure
	 *  0x1: Partial results with more to follow for the CS procedure
	 *  0xF: All subsequent CS Procedures aborted
	 *  All other values: RFU
	 */
	uint8_t ranging_done_status   : 4;
	/** Subevent Done Status:
	 *  0x0: All results complete for the CS Subevent
	 *  0xF: Current CS Subevent aborted.
	 *  All other values: RFU
	 */
	uint8_t subevent_done_status  : 4;
	/** Indicates the abort reason when Procedure_Done Status received from the Core Controller
	 *  (Core Specification, Volume 4, Part 4, Section 7.7.65.44) is set to 0xF,
	 *  otherwise the value is set to zero.
	 *  0x0: Report with no abort
	 *  0x1: Abort because of local Host or remote request
	 *  0x2: Abort because filtered channel map has less than 15 channels
	 *  0x3: Abort because the channel map update instant has passed
	 *  0xF: Abort because of unspecified reasons
	 *  All other values: RFU
	 */
	uint8_t ranging_abort_reason  : 4;
	/** Indicates the abort reason when Subevent_Done_Status received from the Core Controller
	 * (Core Specification, Volume 4, Part 4, Section 7.7.65.44) is set to 0xF,
	 * otherwise the default value is set to zero.
	 * 0x0: Report with no abort
	 * 0x1: Abort because of local Host or remote request
	 * 0x2: Abort because no CS_SYNC (mode 0) received
	 * 0x3: Abort because of scheduling conflicts or limited resources
	 * 0xF: Abort because of unspecified reasons
	 * All other values: RFU
	 */
	uint8_t subevent_abort_reason : 4;
	/** Reference power level. Range: -127 to 20. Units: dBm */
	int8_t  ref_power_level;
	/** Number of steps in the CS Subevent for which results are reported.
	 *  If the Subevent is aborted, then the Number Of Steps Reported can be set to zero
	 */
	uint8_t num_steps_reported;
} __packed;
BUILD_ASSERT(sizeof(struct ras_subevent_header) == BT_RAS_SUBEVENT_HEADER_LEN);

/** @brief RAS Ranging Data Buffer callback structure. */
struct bt_ras_rd_buffer_cb {
	/** @brief New ranging data has been received from the local controller.
	 *
	 *  This callback notifies the application that the ranging data buffer
	 *  has reassembled a complete ranging procedure from the local controller.
	 *
	 *  @param conn Connection object.
	 *  @param ranging_counter Ranging counter of the stored procedure.
	 */
	void (*new_ranging_data_received)(struct bt_conn *conn, uint16_t ranging_counter);

	/** @brief Ranging data has been overwritten.
	 *
	 *  This callback notifies the application that the ranging data buffer
	 *  has overwritten a stored procedure due to running out of buffers
	 *  to store a newer procedure from the local controller.
	 *
	 *  @param conn Connection object.
	 *  @param ranging_counter Ranging counter of the overwritten procedure.
	 */
	void (*ranging_data_overwritten)(struct bt_conn *conn, uint16_t ranging_counter);

	sys_snode_t node;
};

/** @brief RAS Ranging Data buffer structure.
 *
 *  Provides storage and metadata to store a complete Ranging Data body
 *  as defined in RAS Specification, Section 3.2.1.2.
 *  Buffers can be accessed by the application and RRSP concurrently, and will not
 *  be overwritten while any references are held via @ref bt_ras_rd_buffer_claim.
 *
 *  @note The following CS subevent fields are not included by specification:
 *        subevent count, step channel, step length.
 */
struct ras_rd_buffer {
	/** Connection with an RRSP instance owning this buffer. */
	struct bt_conn *conn;
	/** CS Procedure Ranging Counter stored in this buffer. */
	uint16_t ranging_counter;
	/** Write cursor into the procedure subevent buffer. */
	uint16_t subevent_cursor;
	/** Reference counter for buffer.
	 *  The buffer will not be overwritten with active references.
	 */
	atomic_t refcount;
	/** All ranging data has been written, buffer is ready to send. */
	bool ready;
	/** Ranging data is being written to this buffer. */
	bool busy;
	/** The peer has ACKed this buffer, the overwritten callback will not be called. */
	bool acked;
	/** Complete ranging data procedure buffer. */
	union {
		uint8_t buf[BT_RAS_PROCEDURE_MEM];
		struct {
			struct ras_ranging_header ranging_header;
			uint8_t subevents[];
		} __packed;
	} procedure;
};

/** @brief Allocate Ranging Responder instance for connection.
 *
 *  This will allocate an instance of the Ranging Responder service for the given connection.
 *
 *  @note This method must not be called if CONFIG_BT_RAS_RRSP_AUTO_ALLOC_INSTANCE is enabled.
 *  @note The number of supported instances can be set using CONFIG_BT_RAS_RRSP_MAX_ACTIVE_CONN.
 *
 *  @param conn Connection instance.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_ras_rrsp_alloc(struct bt_conn *conn);

/** @brief Free Ranging Responder instance for connection.
 *
 *  This will free an allocated instance of the Ranging Responder service for
 *  the given connection, if one has been allocated.
 *  If the connection has no instance allocated, this method has no effect.
 *
 *  @note This method must not be called if CONFIG_BT_RAS_RRSP_AUTO_ALLOC_INSTANCE is enabled.
 *
 *  @param conn Connection instance.
 */
void bt_ras_rrsp_free(struct bt_conn *conn);

/** @brief Register ranging data buffer callbacks.
 *
 *  Register callbacks to monitor ranging data buffer state.
 *
 *  @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_ras_rd_buffer_cb_register(struct bt_ras_rd_buffer_cb *cb);

/** @brief Check if a given ranging counter is available.
 *
 *  Checks if the given ranging counter is stored in the buffer and
 *  has a valid complete ranging data body stored.
 *
 *  @param conn Connection instance.
 *  @param ranging_counter CS procedure ranging counter.
 *
 *  @retval true A buffer storing this ranging counter exists and can be claimed.
 *  @retval false A buffer storing this ranging counter does not exist.
 */
bool bt_ras_rd_buffer_ready_check(struct bt_conn *conn, uint16_t ranging_counter);

/** @brief Claim a buffer with a given ranging counter.
 *
 *  Returns a pointer to a buffer storing a valid complete ranging data body with
 *  the requested procedure counter, and increments its reference counter.
 *
 *  @param conn Connection instance.
 *  @param ranging_counter CS procedure ranging counter.
 *
 *  @return Pointer to ranging data buffer structure or NULL if no such buffer exists.
 */
struct ras_rd_buffer *bt_ras_rd_buffer_claim(struct bt_conn *conn, uint16_t ranging_counter);

/** @brief Release a claimed ranging data buffer.
 *
 *  Returns a buffer and decrements its reference counter.
 *  The buffer will stay available until overwritten by newer ranging data, if
 *  it has no remaining references.
 *
 *  @param buf Pointer to claimed ranging data buffer.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL Invalid buffer provided.
 */
int bt_ras_rd_buffer_release(struct ras_rd_buffer *buf);

/** @brief Pull bytes from a ranging data buffer.
 *
 *  Utility method to consume up to max_data_len bytes from a buffer.
 *  The provided read_cursor will be used as the initial offset and updated.
 *
 *  @param buf Pointer to claimed ranging data buffer.
 *  @param out_buf Destination to copy up to max_data_len bytes to.
 *  @param max_data_len Maximum amount of bytes to copy from the buffer.
 *  @param read_cursor Current offset into procedure subevent buffer, will be read and written to.
 *  @param empty Set to true if all data has been read from the ranging data buffer.
 *
 *  @return Number of bytes written into out_buf.
 */
int bt_ras_rd_buffer_bytes_pull(struct ras_rd_buffer *buf, uint8_t *out_buf, uint16_t max_data_len,
				uint16_t *read_cursor, bool *empty);

/** @brief Ranging data ready callback. Called when peer has ranging data available.
 *
 * @param[in] conn            Connection Object.
 * @param[in] ranging_counter Ranging counter ready to be requested.
 */
typedef void (*bt_ras_rreq_rd_ready_cb_t)(struct bt_conn *conn, uint16_t ranging_counter);

/** @brief Ranging data overwritten callback. Called when peer has overwritten previously available
 * ranging data.
 *
 * @param[in] conn            Connection Object.
 * @param[in] ranging_counter Ranging counter which has been overwritten.
 */
typedef void (*bt_ras_rreq_rd_overwritten_cb_t)(struct bt_conn *conn, uint16_t ranging_counter);

/** @brief Ranging data get complete callback. Called when ranging data get procedure has completed.
 *
 * @param[in] conn            Connection Object.
 * @param[in] ranging_counter Ranging counter which has been completed.
 * @param[in] err             Error code, 0 if the ranging data get was successful. Otherwise a
 * negative error code.
 */
typedef void (*bt_ras_rreq_ranging_data_get_complete_t)(struct bt_conn *conn,
							uint16_t ranging_counter, int err);

/** @brief RAS features read callback.
 *
 * @param[in] conn         Connection Object.
 * @param[in] feature_bits Bit 0 set if Real-time Ranging Data supported
 *                         Bit 1 set if Retrieve Lost Ranging Data Segments supported
 *                         Bit 2 set if Abort Operation supported
 *                         Bit 3 set if Filter Ranging Data supported
 *                         All other bits are RFU.
 * @param[in] err          Error code, 0 if the features were read successfully. Otherwise a
 * negative error code.
 */
typedef void (*bt_ras_rreq_features_read_cb_t)(struct bt_conn *conn, uint32_t feature_bits,
					       int err);

/** @brief Allocate a RREQ context and assign GATT handles. Takes a reference to the connection.
 *
 * @note RREQ context will be freed automatically on disconnect.
 *
 * @param[in] dm   Discovery Object.
 * @param[in] conn Connection Object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned. No RREQ context will be allocated if
 * there is an error.
 */
int bt_ras_rreq_alloc_and_assign_handles(struct bt_gatt_dm *dm, struct bt_conn *conn);

/** @brief Get ranging data for given ranging counter.
 *
 * @note This should only be called after receiving a ranging data ready callback and
 * when subscribed to ondemand ranging data and RAS-CP.
 *
 * @param[in] conn                 Connection Object.
 * @param[in] ranging_data_out     Simple buffer to store received ranging data.
 * @param[in] ranging_counter      Ranging counter to get.
 * @param[in] data_get_complete_cb Callback called when get ranging data completes.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_cp_get_ranging_data(struct bt_conn *conn, struct net_buf_simple *ranging_data_out,
				    uint16_t ranging_counter,
				    bt_ras_rreq_ranging_data_get_complete_t data_get_complete_cb);

/** @brief Free RREQ context for connection. This will unsubscribe from any remaining subscriptions.
 *
 * @note RREQ context will be freed automatically on disconnect.
 *
 * @param[in] conn Connection Object.
 */
void bt_ras_rreq_free(struct bt_conn *conn);

/** @brief Subscribe to RAS-CP. Required to be called before @ref bt_ras_rreq_cp_get_ranging_data.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_subscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_cp_subscribe(struct bt_conn *conn);

/** @brief Unsubscribe from RAS-CP.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_unsubscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_cp_unsubscribe(struct bt_conn *conn);

/** @brief Subscribe to on-demand ranging data notifications. Required to be called before @ref
 * bt_ras_rreq_cp_get_ranging_data.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_subscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_on_demand_rd_subscribe(struct bt_conn *conn);

/** @brief Unsubscribe from on-demand ranging data notifications.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_unsubscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_on_demand_rd_unsubscribe(struct bt_conn *conn);

/** @brief Subscribe to ranging data ready notifications. These notify when on-demand ranging data
 * is available for a given CS procedure counter.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_subscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 * @param[in] cb   Ranging data ready callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_rd_ready_subscribe(struct bt_conn *conn, bt_ras_rreq_rd_ready_cb_t cb);

/** @brief Unsubscribe from ranging data ready notifications.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_unsubscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_rd_ready_unsubscribe(struct bt_conn *conn);

/** @brief Subscribe to ranging data overwritten notifications. These notify when on-demand ranging
 * data is no longer available for a given CS procedure counter.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_subscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 * @param[in] cb   Ranging data overwritten callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_rd_overwritten_subscribe(struct bt_conn *conn, bt_ras_rreq_rd_overwritten_cb_t cb);

/** @brief Unsubscribe from ranging data overwritten notifications.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_unsubscribe will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_rd_overwritten_unsubscribe(struct bt_conn *conn);

/** @brief Read supported RAS features from peer.
 *
 * @note Calling from BT RX thread may return an error as bt_gatt_read will not block if
 * there are no available TX buffers.
 *
 * @param[in] conn Connection Object, which already has associated RREQ context.
 * @param[in] cb   Features read callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_ras_rreq_read_features(struct bt_conn *conn, bt_ras_rreq_features_read_cb_t cb);

/** @brief Provide ranging header for the ranging data back to the user.
 *
 * @param[in] ranging_header Ranging header data.
 * @param[in] user_data User data.
 *
 * @retval true if data parsing should continue.
 *         false if data parsing should be stopped.
 */
typedef bool (*bt_ras_rreq_ranging_header_cb_t)(struct ras_ranging_header *ranging_header,
						 void *user_data);

/** @brief Provide subevent header for each subevent back to the user.
 *
 * @param[in] subevent_header Subevent header data.
 * @param[in] user_data       User data.
 *
 * @retval true If should continue parsing data.
 *         false If data parsing should be stopped.
 */
typedef bool (*bt_ras_rreq_subevent_header_cb_t)(struct ras_subevent_header *subevent_header,
						 void *user_data);

/** @brief Provide step data for each step back to the user.
 *
 * @param[in] local_step Local step data.
 * @param[in] peer_step  Peer step data.
 * @param[in] user_data  User data.
 *
 * @retval true If should continue parsing data.
 *         false If data parsing should be stopped.
 */
typedef bool (*bt_ras_rreq_step_data_cb_t)(struct bt_le_cs_subevent_step *local_step,
					   struct bt_le_cs_subevent_step *peer_step,
					   void *user_data);

/** @brief Parse peer ranging data buffer and local step data buffer.
 *
 * A helper for parsing ranging data-formatted buffer, as populated by @ref
 * bt_ras_rreq_cp_get_ranging_data, and local step data buffer,
 * where step_data_buf from le_cs_subevent_data_available has been placed together in a struct
 * net_buf_simple.
 *
 * @note All data will be removed from the buffers in this function.
 *
 * @param[in] peer_ranging_data_buf Buffer to the peer ranging data to parse.
 * @param[in] local_step_data_buf   Buffer to the local step data to parse.
 * @param[in] cs_role               Channel sounding role of local device.
 * @param[in] ranging_header_cb     Callback called (once) for the ranging header.
 * @param[in] subevent_header_cb    Callback called with each subevent header.
 * @param[in] step_data_cb          Callback called with each peer and local step data.
 * @param[in] user_data             User data to be passed to the callbacks.
 */
void bt_ras_rreq_rd_subevent_data_parse(struct net_buf_simple *peer_ranging_data_buf,
					struct net_buf_simple *local_step_data_buf,
					enum bt_conn_le_cs_role cs_role,
					bt_ras_rreq_ranging_header_cb_t ranging_header_cb,
					bt_ras_rreq_subevent_header_cb_t subevent_header_cb,
					bt_ras_rreq_step_data_cb_t step_data_cb, void *user_data);

/** @brief Convert CS procedure counter to RAS ranging counter
 *
 * @param[in] procedure_counter Procedure counter
 *
 * @retval RAS ranging counter
 */
static inline uint16_t bt_ras_rreq_get_ranging_counter(uint16_t procedure_counter)
{
	return procedure_counter & 0xFFF;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RAS_H_ */
