/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Bluetooth LE advertising providers subsystem header.
 */

#ifndef BT_ADV_PROV_H_
#define BT_ADV_PROV_H_

#include <zephyr/bluetooth/bluetooth.h>

/**
 * @defgroup bt_le_adv_prov Bluetooth LE advertising providers subsystem
 * @brief The subsystem manages advertising packets and scan response packets.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Structure describing Bluetooth advertising state. */
struct bt_le_adv_prov_adv_state {
	/** Information if the advertising device is looking for a new peer. */
	bool pairing_mode;

	/** Instead of instantly stopping Bluetooth advertising, the advertising may enter grace
	 * period (if requested by at least one of the providers). During the grace period
	 * advertising is continued for the requested time, but the advertising data is modified.
	 * The boolean informs about advertising in grace period.
	 */
	bool in_grace_period;

	/** Information if RPA (Resolvable Private Address) was rotated since the last advertising
	 * data update. Advertising data that contain random values should be re-generated together
	 * with RPA rotation to prevent compromising privacy.
	 */
	bool rpa_rotated;

	/** Information if new advertising session is about to start. If set to false, the
	 * previously started advertising session is continued.
	 */
	bool new_adv_session;
};

/** Structure describing feedback reported by advertising providers. */
struct bt_le_adv_prov_feedback {
	/** Instead of instantly stopping Bluetooth advertising, the advertising may enter grace
	 * period (if requested by at least one of the providers). During the grace period
	 * advertising is continued for the requested time, but the advertising data is modified.
	 * The value is used by providers to inform about required minimal time of grace period.
	 * The time of grace period advertising is equal to maximum time requested by providers.
	 */
	size_t grace_period_s;
};

/**
 * @typedef bt_le_adv_prov_data_get
 * Callback used to get provider's data.
 *
 * @param[out] d	Pointer to structure to be filled with data.
 * @param[in]  state	Pointer to structure describing Bluetooth advertising state.
 * @param[out] fb	Pointer to structure describing provider's feedback.
 *
 * @retval 0		If the operation was successful.
 * @retval (-ENOENT)	If provider does not provide data.
 * @return		Other negative value denotes error specific to provider.
 */
typedef int (*bt_le_adv_prov_data_get)(struct bt_data *d,
				       const struct bt_le_adv_prov_adv_state *state,
				       struct bt_le_adv_prov_feedback *fb);

/** Structure describing advertising data provider. */
struct bt_le_adv_prov_provider {
	/** Function used to get provider's data. */
	bt_le_adv_prov_data_get get_data;
};

/** Register advertising data provider.
 *
 * The macro statically registers an advertising data provider. The provider appends data to
 * advertising packet managed by the Bluetooth LE advertising providers subsystem.
 *
 * @param pname		Provider name.
 * @param get_data_fn	Function used to get provider's advertising data.
 */
#define BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(pname, get_data_fn)					 \
	STRUCT_SECTION_ITERABLE_ALTERNATE(bt_le_adv_prov_ad, bt_le_adv_prov_provider, pname) = { \
		.get_data = get_data_fn,							 \
	}

/** Register scan response data provider.
 *
 * The macro statically registers a scan response data provider. The provider appends data to
 * scan response packet managed by the Bluetooth LE advertising providers subsystem.
 *
 * @param pname		Provider name.
 * @param get_data_fn	Function used to get provider's scan response data.
 */
#define BT_LE_ADV_PROV_SD_PROVIDER_REGISTER(pname, get_data_fn)					 \
	STRUCT_SECTION_ITERABLE_ALTERNATE(bt_le_adv_prov_sd, bt_le_adv_prov_provider, pname) = { \
		.get_data = get_data_fn,							 \
	}

/** Get number of advertising data packet providers.
 *
 * The number of advertising data packet providers defines maximum number of elements in advertising
 * packet that can be provided by providers. An advertising data provider may not provide data.
 *
 * @return Number of advertising data packet providers.
 */
size_t bt_le_adv_prov_get_ad_prov_cnt(void);

/** Get number of scan response data packet providers.
 *
 * The number of scan response data packet providers defines maximum number of elements in scan
 * response packet that can be provided by providers. A scan response data provider may not provide
 * data.
 *
 * @return Number of scan response data packet providers.
 */
size_t bt_le_adv_prov_get_sd_prov_cnt(void);

/** Fill advertising data.
 *
 * Number of elements in array pointed by ad must be at least equal to @ref
 * bt_le_adv_prov_get_ad_prov_cnt.
 *
 * @param[out]    ad		Pointer to array to be filled with advertising data.
 * @param[in,out] ad_len        Value describing number of elements in the array pointed by ad.
 *				The value is then set by the function to number of filled elements.
 * @param[in]     state		Structure describing advertising state.
 * @param[out]    fb		Structure filled with feedback from advertising data providers.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_le_adv_prov_get_ad(struct bt_data *ad, size_t *ad_len,
			  const struct bt_le_adv_prov_adv_state *state,
			  struct bt_le_adv_prov_feedback *fb);

/** Fill scan response data.
 *
 * Number of elements in array pointed by sd must be at least equal to @ref
 * bt_le_adv_prov_get_sd_prov_cnt.
 *
 * @param[out]    sd		Pointer to array to be filled with scan response data.
 * @param[in,out] sd_len        Value describing number of elements in the array pointed by sd.
 *				The value is then set by the function to number of filled elements.
 * @param[in]     state		Structure describing advertising state.
 * @param[out]    fb		Structure filled with feedback from scan response data providers.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_le_adv_prov_get_sd(struct bt_data *sd, size_t *sd_len,
			  const struct bt_le_adv_prov_adv_state *state,
			  struct bt_le_adv_prov_feedback *fb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ADV_PROV_H_ */
