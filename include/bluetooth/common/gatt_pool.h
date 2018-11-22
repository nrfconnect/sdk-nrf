/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_POOL_
#define BT_GATT_POOL_

/**
 * @file
 * @defgroup bt_gatt_pool  BLE GATT attribute pools API
 * @{
 * @brief BLE GATT attribute pools.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

/**
 *  @brief Register a primary service descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _svc_uuid_init Service UUID.
 */
#define BT_GATT_POOL_SVC_GET(_svc, _svc_uuid_init)                             \
	{                                                                      \
		struct bt_uuid *_svc_uuid = _svc_uuid_init;                    \
		bt_gatt_pool_svc_get(_svc, _svc_uuid);                         \
	}

/**
 *  @brief Register a characteristic descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _chrc_uuid_init Characteristic UUID.
 *  @param _char_props Properties of the characteristic.
 */
#define BT_GATT_POOL_CHRC_GET(_svc, _chrc_uuid_init, _char_props)              \
	{                                                                      \
		struct bt_gatt_chrc _chrc = {                                  \
		    .uuid = _chrc_uuid_init,                                   \
		    .properties = _char_props,                                 \
		};                                                             \
		bt_gatt_pool_chrc_get(_svc, &_chrc);                           \
	}

/**
 *  @brief Register an attribute descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _descriptor_init Attribute descriptor.
 */
#define BT_GATT_POOL_DESC_GET(_svc, _descriptor_init)                          \
	{                                                                      \
		struct bt_gatt_attr _descriptor = _descriptor_init;            \
		bt_gatt_pool_desc_get(_svc, &_descriptor);                     \
	}

/**
 *  @brief Register a CCC descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _ccc_cfg CCC descriptor configuration.
 *  @param _ccc_cb CCC descriptor callback.
 */
#define BT_GATT_POOL_CCC_GET(_svc, _ccc_cfg, _ccc_cb)                          \
	{                                                                      \
		struct _bt_gatt_ccc _ccc = {                                   \
		    .cfg = _ccc_cfg,                                           \
		    .cfg_len = ARRAY_SIZE(_ccc_cfg),                           \
		    .cfg_changed = _ccc_cb,                                    \
		};                                                             \
		bt_gatt_pool_ccc_get(_svc, &_ccc);                             \
	}

/** @brief Take a primary service descriptor from the pool.
 *
 *  @param svc GATT service descriptor.
 *  @param svc_uuid Service UUID.
 */
void bt_gatt_pool_svc_get(struct bt_gatt_service *svc,
			  struct bt_uuid const *svc_uuid);

/** @brief Return a primary service descriptor to the pool.
 *
 *  @param attr Attribute describing the primary service to be returned.
 */
void bt_gatt_pool_svc_put(struct bt_gatt_attr const *attr);

/** @brief Take a characteristic descriptor from the pool.
 *
 *  @param svc GATT service descriptor.
 *  @param chrc Characteristic descriptor.
 */
void bt_gatt_pool_chrc_get(struct bt_gatt_service *svc,
			   struct bt_gatt_chrc const *chrc);

/** @brief Return a characteristic descriptor to the pool.
 *
 *  @param attr Attribute describing the characteristic to be returned.
 */
void bt_gatt_pool_chrc_put(struct bt_gatt_attr const *attr);

/** @brief Take an attribute descriptor from the pool.
 *
 *  @param svc GATT service descriptor.
 *  @param descriptor Attribute descriptor.
 */
void bt_gatt_pool_desc_get(struct bt_gatt_service *svc,
			   struct bt_gatt_attr const *descriptor);

/** @brief Return an attribute descriptor to the pool.
 *
 *  @param attr Attribute describing the descriptor to be returned.
 */
void bt_gatt_pool_desc_put(struct bt_gatt_attr const *attr);

/** @brief Take a CCC descriptor from the pool.
 *
 *  @param svc GATT service descriptor.
 *  @param ccc CCC descriptor.
 */
void bt_gatt_pool_ccc_get(struct bt_gatt_service *svc,
			  struct _bt_gatt_ccc const *ccc);

/** @brief Return a CCC descriptor to the pool.
 *
 *  @param attr Attribute describing the CCC descriptor to be returned.
 */
void bt_gatt_pool_ccc_put(struct bt_gatt_attr const *attr);

#if CONFIG_BT_GATT_POOL_STATS != 0
/** @brief Print basic module statistics (containing pool size usage).
 */
void bt_gatt_pool_stats_print(void);
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_POOL_ */
