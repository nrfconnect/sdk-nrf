/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup audio_app_bt_stream
 * @{
 * @defgroup le_audio_tx Functions for LE Audio TX functionality.
 * @{
 * @brief Helper functions to manage LE Audio TX functionality.
 */

#ifndef _LE_AUDIO_TX_H_
#define _LE_AUDIO_TX_H_

#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "zbus_common.h"
#include "le_audio.h"
#include "audio_defines.h"

struct le_audio_tx_info {
	struct stream_index idx;
	struct bt_cap_stream *cap_stream;
	uint8_t audio_location;
};

struct bt_le_audio_tx_ctx;

#define HCI_ISO_BUF_PER_CHAN 2

#if defined(CONFIG_BT_ISO_MAX_CIG) && defined(CONFIG_BT_ISO_MAX_BIG)
#define GROUP_MAX (CONFIG_BT_ISO_MAX_BIG + CONFIG_BT_ISO_MAX_CIG)
#elif defined(CONFIG_BT_ISO_MAX_CIG)
#define GROUP_MAX CONFIG_BT_ISO_MAX_CIG
#elif defined(CONFIG_BT_ISO_MAX_BIG)
#define GROUP_MAX CONFIG_BT_ISO_MAX_BIG
#else
#error Neither CIG nor BIG defined
#endif

#if (defined(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) && defined(CONFIG_BT_BAP_UNICAST))
#define SUBGROUP_MAX (1 + CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT)
#elif (defined(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) && !defined(CONFIG_BT_BAP_UNICAST))
#define SUBGROUP_MAX CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT
#else
#define SUBGROUP_MAX 1
#endif

#define TX_STREAMS_MAX CONFIG_BT_ISO_MAX_CHAN

#define TX_BUF_NUM (TX_STREAMS_MAX * HCI_ISO_BUF_PER_CHAN)

struct tx_inf {
	uint16_t iso_conn_handle;
	struct bt_iso_tx_info iso_tx;
	struct bt_iso_tx_info iso_tx_readback;
	atomic_t iso_tx_pool_alloc;
	bool hci_wrn_printed;
};

struct bt_le_audio_tx_ctx {
	struct net_buf_pool *iso_tx_pool;
	struct tx_inf tx_info_arr[TX_STREAMS_MAX];
	/* Estimated timestamp from the controller and validity */
	uint32_t ts_ctlr_esti_us;
	bool ts_ctlr_esti_us_valid;
	/* Real timestamp from the controller */
	uint32_t ts_ctlr_real_us_last;
	/* Timestamp - when the last correction was applied */
	uint32_t ts_last_correction;
	/* Counter to track subsequent rapid corrections. Indicates an error state */
	uint32_t subsequent_rapid_corrections;
	/* Correction between estimated and real timestamps */
	int64_t corr_diff_us;
	/* The previous "now" timestamp and validity */
	uint32_t ts_last_us;
	bool ts_last_us_valid;
	/* This device is Bluetooth clock master (central/unicast client or broadcast source) */
	bool is_ble_clock_master;
	/* Status of the last submitted data */
	enum tx_data_status last_data_status;
};

#define BT_LE_AUDIO_TX_DEFINE(name)                                                                \
	NET_BUF_POOL_FIXED_DEFINE(name##_cfg_buf, TX_BUF_NUM,                                      \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),                       \
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);                         \
	static struct bt_le_audio_tx_ctx name##_ctx = {                                            \
		.iso_tx_pool = &name##_cfg_buf,                                                    \
	};                                                                                         \
	static struct bt_le_audio_tx_ctx *name = &name##_ctx

#define BT_LE_AUDIO_TX_DEFINE_INDEXED(name, group_idx, subgroup_idx)                               \
	NET_BUF_POOL_FIXED_DEFINE(name##_##group_idx##_##subgroup_idx##_buf, TX_BUF_NUM,           \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),                       \
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);                         \
	static struct bt_le_audio_tx_ctx name##_##group_idx##_##subgroup_idx##_ctx = {             \
		.iso_tx_pool = &name##_##group_idx##_##subgroup_idx##_buf,                         \
	};                                                                                         \
	__unused static struct bt_le_audio_tx_ctx *name##_##group_idx##_##subgroup_idx =           \
		&name##_##group_idx##_##subgroup_idx##_ctx

/**
 * @brief	Sends N channels of synchronized audio data to the controller.
 *
 * @details
 * This function is the main LE Audio TX scheduling point. One call corresponds to one
 * frame interval for the configured streams. All streams which shall be sent together
 * (synchronized) are sent at the same time.
 * This implies calling once for every subgroup when doing broadcast and once for each
 * CIG in unicast. If 0 is returned, it publishes a reference message on @c sdu_ref_chan.
 * This ZBUS message contains the current time, and when the SDU(s) are expected on air.
 * Calling too late/slow will cause empty data on air, whilst calling too early/fast will
 * trigger this module to flush reduce latency and avoid buffer overruns.
 *
 * Summary:
 * - Supply data which shall be sent synchronized in a  CIS/subgroup. (E.g,
 *  2 streams every 10 ms).
 * - Call @ref bt_le_audio_tx_stream_sent for each stream after TX completion to keep
 *   pool accounting correct.
 *
 * @note Not tested for ISO interval not equal to frame length.
 *
 * @param[in]	ctx		Pointer to an initialized TX context pr CIG for unicast and pr
 *				subgroup for broadcast.
 * @param[in]	audio_frame	Pointer to encoded audio data and metadata.
 * @param[in]	tx		Pointer to an array of @ref le_audio_tx_info elements.
 * @param[in]	num_tx		Number of elements in @p tx.
 *
 * @retval	0		Success (including controlled pacing cases where no SDU is sent,
 *				but finalize/publish succeeds).
 * @retval	-EINVAL		Invalid arguments, invalid stream index tuple, invalid frame
 *				metadata, or inconsistent stream/audio configuration.
 * @retval	-EIO		No stream was sent successfully.
 *
 * @return	Other negative error codes may be propagated from lower layers (for example
 *		timestamp synchronization/HCI readback or SDU reference publish failures).
 */
int bt_le_audio_tx_send(struct bt_le_audio_tx_ctx *ctx, struct net_buf const *const audio_frame,
			struct le_audio_tx_info *tx, uint8_t num_tx);

/**
 * @brief	Initializes per-stream TX bookkeeping when a stream enters streaming state.
 *
 * @param[in]	ctx		Pointer to TX context.
 * @param[in]	stream_idx	Stream index.
 *
 * @retval	0		Success.
 * @retval	-EINVAL		ctx is NULL or the module has not been initialized.
 * @retval      -ESPIPE		stream_idx is out of bounds.
 */
int bt_le_audio_tx_stream_started(struct bt_le_audio_tx_ctx *ctx, struct stream_index stream_idx);

/**
 * @brief	Releases one outstanding TX pool allocation for the provided stream.
 *
 * @details
 * Call this after TX completion notification for each stream that consumed one TX buffer.
 * This keeps the module's per-stream pool accounting aligned with actual controller
 * completions.
 *
 * @param[in]	ctx		Pointer to TX context.
 * @param[in]	stream_idx	Stream index.
 *
 * @retval	0		Success.
 * @retval	-EINVAL		ctx is NULL or the module has not been initialized.
 * @retval      -ESPIPE		stream_idx is out of bounds.
 */
int bt_le_audio_tx_stream_sent(struct bt_le_audio_tx_ctx *ctx, struct stream_index stream_idx);

/**
 * @brief	Initializes the TX path context and shared ISO flush worker.
 *
 * @param[in]	ctx		Pointer to TX context.
 * @param[in]	is_clock_master	Set to true if this device is a Bluetooth central,
 *				(Unicast client) or Broadcast source,
 *				false = peripheral (unicast server) or Broadcast sink.
 *
 * @details
 * The context memory is cleared and re-initialized while preserving @p ctx->iso_tx_pool.
 * A shared internal work queue used for deferred ISO flush handling is started once on the
 * first successful call.
 *
 * @retval	0		Success.
 * @retval	-EINVAL		@p ctx is NULL or required configuration is missing.
 */
int bt_le_audio_tx_init(struct bt_le_audio_tx_ctx *ctx, bool is_clock_master);

/**
 * @}
 * @}
 */


#endif /* _LE_AUDIO_TX_H_ */
