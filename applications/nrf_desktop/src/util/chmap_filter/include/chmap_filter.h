/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _CHMAP_FILTER_H_
#define _CHMAP_FILTER_H_

/**
 * @brief Utility used to filter QoS information (per-channel CRC status)
 *        with the purpose of generating a suitable BLE channel map.
 *
 * @details Input to the library is CRC information.
 *          This information is used to assign ratings to each BLE channel.
 *          Single channels with poor ratings, or blocks of channels
 *          affected by wifi are removed from the channel map recommendation.
 *
 * @note The library is not thread-safe. Apart from the "get" functions,
 * functions must not be called while @ref chmap_filter_process is running
 *
 * @defgroup chmap_filter Channel map filtering
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zephyr/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WLAN characteristics */
/* Note: Some channels may be omitted to reduce state memory usage */
#define CHMAP_WLAN_802_11GN_CENTER_FREQS \
	{12, 17, 22, 27, 32, 37, 42, 47, 52, 57, 62, 67}
#define CHMAP_WLAN_802_11GN_CENTER_FREQ_COUNT  12
#define CHMAP_WLAN_802_11GN_CHANNEL_WIDTH_MHz  20

/* BLE characteristics */
#define CHMAP_BLE_CHANNEL_COUNT 37
#define CHMAP_BLE_BITMASK_SIZE 5
#define CHMAP_BLE_BITMASK_DEFAULT {0xFF, 0xFF, 0xFF, 0xFF, 0x1F}

/* Filter instance size [bytes] */
#define CHMAP_FILTER_INST_SIZE 580

/* Filter instance alignment [bytes] */
#define CHMAP_FILTER_INST_ALIGN 4

/* Static parameters */
/* Target evaluation keepout for minimum size channel map */
#define CHMAP_PARAM_DYN_EVAL_TARGET 10
/* Channel block duration factor */
/* Duration will increase based on previous channel history */
#define CHMAP_PARAM_DYN_BLOCK_INCREASE 2

/* Default parameter values */
#define DEFAULT_PARAM_MAINTENANCE_SAMPLE_COUNT 2000
#define DEFAULT_PARAM_INITIAL_RATING           0
#define DEFAULT_PARAM_BLE_MIN_CHANNEL_COUNT    4
#define DEFAULT_PARAM_BLE_WIFI_KEEPOUT_DURATION 2
#define DEFAULT_PARAM_EVAL_MAX_COUNT           1
#define DEFAULT_PARAM_EVAL_DURATION            5
#define DEFAULT_PARAM_EVAL_KEEPOUT_DURATION    300
#define DEFAULT_PARAM_DYN_DURATIONS            true
/* The below parameters are fixed point values, with scaling factor of 1/100 */
#define DEFAULT_PARAM_BLE_WEIGHT_CRC_OK        100  /* = 1.0 */
#define DEFAULT_PARAM_BLE_WEIGHT_CRC_ERROR     -100 /* = -1.0 */
#define DEFAULT_PARAM_BLE_RATING_TRIM          49   /* = 0.49 */
#define DEFAULT_PARAM_BLE_BLOCK_THRESHOLD      25   /* = 0.25 */
#define DEFAULT_PARAM_WIFI_RATING_INC          400  /* = 4.0 */
#define DEFAULT_PARAM_WIFI_PRESENT_THRESHOLD   90   /* = 0.9 */
#define DEFAULT_PARAM_WIFI_ACTIVE_THRESHOLD    75   /* = 0.75 */
#define DEFAULT_PARAM_WIFI_RATING_TRIM         50   /* = 0.5 */
#define DEFAULT_PARAM_EVAL_SUCCESS_THRESHOLD   85   /* = 0.85 */

/**@brief Channel map filter parameters that can be changed at runtime
 *
 * @note Type 'fix' is used to indicate a fixed point value with a scaling
 * factor of 1/100. E.g. value 20 = 20/100 = 0.2
 * @note Threshold values are ratios of average channel rating.
 * E.g. PARAM_BLE_BLOCK_THRESHOLD = 20 will block a BLE channel if its rating
 * falls below 20% of the average rating.
 * @note The time unit is based on how often @ref chmap_filter_process is run.
 * Recommend running this function at ~1 second interval
 */
struct chmap_filter_params {
/** Maintenance algorithm needs these many samples to run [int] */
	uint16_t maintenance_sample_count;
/** Initial channel rating [int] */
	int32_t initial_rating;
/** Min num of channels in channel map [int] */
	uint8_t  min_channel_count;
/** Channel rating weight of CRC OK [fix] */
	int16_t ble_weight_crc_ok;
/** Channel rating weight of CRC ERR [fix] */
	int16_t ble_weight_crc_error;
/** Factor for BLE channel rating trim. Must be < 100.
 *  Note: "high" value can lead to rating overflow [fix]
 */
	int16_t ble_rating_trim;
/** Threshold of average rating required to block of single channel [fix] */
	int16_t ble_block_threshold;
/** BLE channel suspected of being affected by wifi will not be blocked as
 *  single channel until this time has passed [int]
 */
	uint16_t ble_wifi_keepout_duration;
/** Wifi strength increase factor. Higher value blocks wifi faster [fix] */
	int16_t wifi_rating_inc;
/** Threshold of average rating that indicates Wifi presence [fix] */
	int16_t wifi_present_threshold;
/** Threshold of average rating required to block wifi channel [fix] */
	int16_t wifi_active_threshold;
/** Factor for Wifi channel rating trim. Must be < 100. [fix] */
	int16_t wifi_rating_trim;
/** Maximum number of channels that can be evaluated at a time [int] */
	uint8_t  eval_max_count;
/** Evaluated channels will be evaluated for these many time units [int] */
	uint16_t eval_duration;
/** Blocked channels will not be evaluated for these many time units [int] */
	uint16_t eval_keepout_duration;
/** Rating of evaluated channel must be above this threshold [fix] */
	int16_t eval_success_threshold;
/** Dynamic block and evaluation durations. [bool]
 *  Fewer channels in channel map = shorter time until evaluation.
 *  Channel blocked in the past = longer time until evaluation
 */
	bool dynamic_durations;
};

struct chmap_instance;

/**@brief Get library version number
 *
 * @return Pointer to null-terminated version string
 */
const char *chmap_filter_version(void);

/**@brief Initialize channel map filter library
 */
void chmap_filter_init(void);

/**@brief Initialize channel map filter instance.
 *
 * @details Sufficient memory should be supplied to hold filter state.
 * Allocate @ref CHMAP_FILTER_INST_SIZE bytes for this purpose. Make sure that
 * used memory is aligned to @ref CHMAP_FILTER_INST_ALIGN.
 *
 * @param[in] p_inst Pointer to instance buffer
 * @param[in] size Size of instance buffer
 *
 * @retval 0 when successful
 * @retval -ENOMEM if buffer is too small
 */
int chmap_filter_instance_init(struct chmap_instance *p_inst, size_t size);

/**@brief Update channel CRC information
 *
 * @note Should not preempt @ref chmap_filter_process
 *
 * @param[in] p_inst    Channel map instance pointer
 * @param[in] ch_idx    BLE channel index
 * @param[in] crc_ok    CRC ok count
 * @param[in] crc_error CRC error count
 */
void chmap_filter_crc_update(
	struct chmap_instance *p_inst,
	uint8_t ch_idx,
	uint16_t crc_ok,
	uint8_t crc_error);

/**@brief Process channel map filter
 *
 * @param[in] p_inst Channel map instance pointer
 *
 * @return true when channel map update is recommended
 */
bool chmap_filter_process(struct chmap_instance *p_inst);

/**@brief Get suggested channel map
 *
 * @param[in] p_inst Channel map instance pointer
 *
 * @return pointer to @em CHMAP_BLE_BITMAP_SIZE sized channel map array
 */
uint8_t *chmap_filter_suggested_map_get(struct chmap_instance *p_inst);

/**@brief Confirm that suggested channel map has been applied
 *
 * @details This function is used to keep internal state consistent with actual
 *          channel map used. There will always be a brief period of
 *          inconsistency when a new channel map is applied,
 *          which is to be expected.
 *
 * @param[in] p_inst Channel map instance pointer
 */
void chmap_filter_suggested_map_confirm(struct chmap_instance *p_inst);

/**@brief Get dynamic parameters
 *
 * @param[in]  p_inst   Channel map instance pointer
 * @param[out] p_params Buffer to hold parameter values
 */
void chmap_filter_params_get(
	struct chmap_instance *p_inst,
	struct chmap_filter_params *p_params);

/**@brief Set dynamic parameters
 *
 * @details Parameter update follow the read-modify-write pattern.
 *          Use @ref chmap_filter_params_get to get the full set of parameters,
 *          make desired adjustments,
 *          then @ref chmap_filter_params_set to set updated parameters.
 *
 * @note Parameters will be applied when @ref chmap_filter_process is called.
 *
 * @param[in] p_inst   Channel map instance pointer
 * @param[in] p_params Buffer that holds parameter values
 *
 * @retval 0 when successful.
 * @retval -EINVAL if one or more parameters are invalid.
 */
int chmap_filter_params_set(
	struct chmap_instance *p_inst,
	struct chmap_filter_params *p_params);

/**@brief Get bitmask of currently blacklisted wifi channels
 *
 * @note As blacklist updates happen asynchronously,
 *       @ref chmap_filter_process needs to run after
 *       @ref chmap_filter_blacklist_set
 *       for the information returned from this function to be valid.
 *
 * @return Bitmask of wifi channels.
 *         E.g. channel 6 and channel 8 = (1 << 6) | (1 << 8) = 320
 */
uint16_t chmap_filter_wifi_blacklist_get(void);

/**@brief Set wifi channels blacklist
 *
 * @details One or more channels can be blacklisted.
 *          It does not matter if channels overlap.
 *          The blacklist will not be accepted if the total number of
 *          blacklisted BLE channels exceeds the minimum channel count param.
 *
 * @note    Blacklist can be partially added if more than one channel is
 *          blacklisted and function return is non-zero.
 *          Use @ref chmap_filter_wifi_blacklist_get to confirm.
 *
 * @param[in] p_inst    Channel map instance pointer
 * @param[in] blacklist Bitfield of blacklisted wifi channels.
 *                      E.g. wifi channel 6 = (1 << 6)
 *
 * @retval 0 when successful,
 * @retval -EINVAL if blacklist exceeds minimum channel count.
 */
int chmap_filter_blacklist_set(struct chmap_instance *p_inst, uint16_t blacklist);

/**@brief Get BLE channel information
 *
 * @param[in]  p_inst   Channel map instance pointer
 * @param[in]  chn_idx  Bluetooth channel index (0-36)
 * @param[out] p_state  Pointer to hold state value
 * @param[out] p_rating Pointer to hold rating value
 * @param[out] p_freq   Pointer to hold frequency value [2400 + x MHz]
 *
 * @retval 0 when successful
 * @retval -EINVAL if p_inst or chn_idx is invalid
 */
int chmap_filter_chn_info_get(
	struct chmap_instance *p_inst,
	uint8_t chn_idx,
	uint8_t *p_state,
	int16_t *p_rating,
	uint8_t *p_freq);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CHMAP_FILTER_H_ */
