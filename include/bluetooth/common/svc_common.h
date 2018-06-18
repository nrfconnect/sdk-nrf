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
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int primary_svc_register(struct bt_gatt_service *const svc,
			 struct bt_uuid const *const svc_uuid);

/** @brief Registers characteristic descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param chrc characteristic descriptor
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int chrc_register(struct bt_gatt_service *const svc,
		  struct bt_gatt_chrc const *const chrc);

/** @brief Registers attribute descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param descriptor attribute descriptor
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int descriptor_register(struct bt_gatt_service *const svc,
			struct bt_gatt_attr const *const descriptor);

/** @brief Registers CCC descriptor.
 *
 *  @param svc GATT service descriptor
 *  @param ccc CCC descriptor
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int ccc_register(struct bt_gatt_service *const svc,
		 struct _bt_gatt_ccc const *const ccc);

#ifdef __cplusplus
}
#endif
