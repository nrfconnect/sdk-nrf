/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @def PRIMARY_SVC_REGISTER
 *  @brief Macro for Primary Service descriptor registration.
 *
 *  @param _svc GATT service descriptor
 *  @param _svc_uuid_init service UUID
 */
#define PRIMARY_SVC_REGISTER(_svc, _svc_uuid_init)	    \
	{						    \
		struct bt_uuid *_svc_uuid = _svc_uuid_init; \
		primary_svc_register(_svc, _svc_uuid);	    \
	}


/** @def CHRC_REGISTER
 *  @brief Macro for characteristic descriptor registration.
 *
 *  @param _svc GATT service descriptor
 *  @param _chrc_uuid_init characteristic uuid
 *  @param _char_props characteristic properties
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

/** @def DESCRIPTOR_REGISTER
 *  @brief Macro for attribute descriptor registration.
 *
 *  @param _svc GATT service descriptor
 *  @param _descriptor_init attribute descriptor
 */
#define DESCRIPTOR_REGISTER(_svc, _descriptor_init)		    \
	{							    \
		struct bt_gatt_attr _descriptor = _descriptor_init; \
		descriptor_register(_svc, &_descriptor);	    \
	}

/** @def CCC_REGISTER
 *  @brief Macro for CCC descriptor registration.
 *
 *  @param _svc GATT service descriptor
 *  @param _ccc_cfg CCC descriptor configuration
 *  @param _ccc_cb CCC descriptor callback
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

/** @brief Registers Primary service descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param svc_uuid_init service UUID
 */
void primary_svc_register(struct bt_gatt_service *svc,
			  struct bt_uuid const *svc_uuid);

/** @brief Unregisters Primary service descriptor.
 *
 *  @param attr attribute describing previously registered primary service
 */
void primary_svc_unregister(struct bt_gatt_attr const *attr);

/** @brief Registers characteristic descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param chrc characteristic descriptor
 */
void chrc_register(struct bt_gatt_service *svc,
		   struct bt_gatt_chrc const *chrc);

/** @brief Unregisters characteristic descriptor.
 *
 *  @param attr attribute describing previously registered characteristic
 */
void chrc_unregister(struct bt_gatt_attr const *attr);

/** @brief Registers attribute descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param descriptor attribute descriptor
 */
void descriptor_register(struct bt_gatt_service *svc,
			 struct bt_gatt_attr const *descriptor);

/** @brief Unregisters attribute descriptor.
 *
 *  @param attr attribute describing previously registered descriptor
 */
void descriptor_unregister(struct bt_gatt_attr const *attr);

/** @brief Registers CCC descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param ccc CCC descriptor
 */
void ccc_register(struct bt_gatt_service *svc, struct _bt_gatt_ccc const *ccc);

/** @brief Unregisters CCC descriptor.
 *
 *  @param attr attribute describing previously registered CCC descriptor
 */
void ccc_unregister(struct bt_gatt_attr const *attr);

#if CONFIG_NRF_BT_STATISTICS_PRINT != 0
/** @brief Prints basic module statistics containing pool size usage.
 */
void statistics_print(void);
#endif

#ifdef __cplusplus
}
#endif
