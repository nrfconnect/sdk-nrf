/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef __SVC_COMMON_H
#define __SVC_COMMON_H

/**
 * @file
 * @defgroup nrf_bt_svc_common Common BLE GATT service utilities API
 * @{
 * @brief Common utilities for BLE GATT services.
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
#define PRIMARY_SVC_REGISTER(_svc, _svc_uuid_init)	    \
	{						    \
		struct bt_uuid *_svc_uuid = _svc_uuid_init; \
		primary_svc_register(_svc, _svc_uuid);	    \
	}


/**
 *  @brief Register a characteristic descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _chrc_uuid_init Characteristic UUID.
 *  @param _char_props Properties of the characteristic.
 */
#define CHRC_REGISTER(_svc, _chrc_uuid_init, _char_props) \
	{						  \
		struct bt_gatt_chrc _chrc =		  \
		{					  \
			.uuid = _chrc_uuid_init,	  \
			.properties = _char_props,	  \
		};					  \
		chrc_register(_svc, &_chrc);		  \
	}

/**
 *  @brief Register an attribute descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _descriptor_init Attribute descriptor.
 */
#define DESCRIPTOR_REGISTER(_svc, _descriptor_init)		    \
	{							    \
		struct bt_gatt_attr _descriptor = _descriptor_init; \
		descriptor_register(_svc, &_descriptor);	    \
	}

/**
 *  @brief Register a CCC descriptor.
 *
 *  @param _svc GATT service descriptor.
 *  @param _ccc_cfg CCC descriptor configuration.
 *  @param _ccc_cb CCC descriptor callback.
 */
#define CCC_REGISTER(_svc, _ccc_cfg, _ccc_cb)		 \
	{						 \
		struct _bt_gatt_ccc _ccc =		 \
		{					 \
			.cfg = _ccc_cfg,		 \
			.cfg_len = ARRAY_SIZE(_ccc_cfg), \
			.cfg_changed = _ccc_cb,		 \
		};					 \
		ccc_register(_svc, &_ccc);		 \
	}

/** @brief Register a primary service descriptor.
 *
 *  @param svc GATT service descriptor.
 *  @param svc_uuid Service UUID.
 */
void primary_svc_register(struct bt_gatt_service *svc,
			  struct bt_uuid const *svc_uuid);

/** @brief Unregister a primary service descriptor.
 *
 *  @param attr Attribute describing a previously registered primary service.
 */
void primary_svc_unregister(struct bt_gatt_attr const *attr);

/** @brief Register a characteristic descriptor.
 *
 *  @param svc GATT service descriptor.
 *  @param chrc Characteristic descriptor.
 */
void chrc_register(struct bt_gatt_service *svc,
		   struct bt_gatt_chrc const *chrc);

/** @brief Unregister a characteristic descriptor.
 *
 *  @param attr Attribute describing a previously registered characteristic.
 */
void chrc_unregister(struct bt_gatt_attr const *attr);

/** @brief Register an attribute descriptor.
 *
 *  @param svc GATT service descriptor.
 *  @param descriptor Attribute descriptor.
 */
void descriptor_register(struct bt_gatt_service *svc,
			 struct bt_gatt_attr const *descriptor);

/** @brief Unregister an attribute descriptor.
 *
 *  @param attr Attribute describing a previously registered descriptor.
 */
void descriptor_unregister(struct bt_gatt_attr const *attr);

/** @brief Register a CCC descriptor.
 *
 *  @param svc GATT service descriptor.
 *  @param ccc CCC descriptor.
 */
void ccc_register(struct bt_gatt_service *svc, struct _bt_gatt_ccc const *ccc);

/** @brief Unregister a CCC descriptor.
 *
 *  @param attr Attribute describing a previously registered CCC descriptor.
 */
void ccc_unregister(struct bt_gatt_attr const *attr);

#if CONFIG_NRF_BT_STATISTICS_PRINT != 0
/** @brief Print basic module statistics (containing pool size usage).
 */
void statistics_print(void);
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __SVC_COMMON_H */
