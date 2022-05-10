/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_GATT_POOL_
#define BT_GATT_POOL_

/**
 * @file
 * @defgroup bt_gatt_pool  Bluetooth LE GATT attribute pools API
 * @{
 * @brief Bluetooth LE GATT attribute pools.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>


/** @brief Initialization of the GATT attribute pool variable.
 *
 *  This macro creates the initializer for the new attribute pool.
 *
 *  @param _attr_array_size Size of the attribute array.
 */
#define BT_GATT_POOL_INIT(_attr_array_size)                                    \
	{                                                                      \
		.svc =                                                         \
		{                                                              \
			.attrs = (struct bt_gatt_attr[_attr_array_size])       \
				{ {0} }                                        \
		},                                                             \
		.attr_array_size = _attr_array_size                            \
	}

/** @brief Define a new GATT attribute pool.
 *
 *  This macro creates a new attribute pool.
 *
 *  @param _name            Name of the pool created.
 *  @param _attr_array_size Size of the attribute array.
 */
#define BT_GATT_POOL_DEF(_name, _attr_array_size) \
	const static struct bt_gatt_pool _name =  \
		BT_GATT_POOL_INIT(_attr_array_size)

/** @brief Register a primary service descriptor.
 *
 *  @param _gp GATT service object with dynamic attribute allocation.
 *  @param _svc_uuid_init Service UUID.
 */
#define BT_GATT_POOL_SVC(_gp, _svc_uuid_init)                                  \
	do {                                                                   \
		int _ret;                                                      \
		const struct bt_uuid *_svc_uuid = _svc_uuid_init;              \
		_ret = bt_gatt_pool_svc_alloc(_gp, _svc_uuid);                 \
		__ASSERT_NO_MSG(!_ret);                                        \
		(void)_ret;                                                    \
	} while (0)

/** @brief Register a characteristic descriptor.
 *
 *  @param _gp GATT service object with dynamic attribute allocation.
 *  @param _uuid Characteristic UUID.
 *  @param _props Characteristic properties.
 *  @param _perm Characteristic access permissions.
 *  @param _read Characteristic read callback.
 *  @param _write Characteristic write callback.
 *  @param _value Characteristic value.
 */
#define BT_GATT_POOL_CHRC(_gp, _uuid, _props, _perm, _read, _write, _value)    \
	do {                                                                   \
		int _ret;                                                      \
		const struct bt_gatt_attr _attr = BT_GATT_ATTRIBUTE(           \
			_uuid, _perm, _read, _write, _value);                  \
		_ret = bt_gatt_pool_chrc_alloc(_gp, _props, &_attr);           \
		__ASSERT_NO_MSG(!_ret);                                        \
		(void)_ret;                                                    \
	} while (0)

/** @brief Register an attribute descriptor.
 *
 *  @param _gp GATT service object with dynamic attribute allocation.
 *  @param _uuid Descriptor UUID.
 *  @param _perm Descriptor access permissions.
 *  @param _read Descriptor read callback.
 *  @param _write Descriptor write callback.
 *  @param _value Descriptor value.
 */
#define BT_GATT_POOL_DESC(_gp, _uuid, _perm, _read, _write, _value)            \
	do {                                                                   \
		int _ret;                                                      \
		const struct bt_gatt_attr _descriptor = BT_GATT_DESCRIPTOR(    \
			_uuid, _perm, _read, _write, _value);                  \
		_ret = bt_gatt_pool_desc_alloc(_gp, &_descriptor);             \
		__ASSERT_NO_MSG(!_ret);                                        \
		(void)_ret;                                                    \
	} while (0)

/** @brief Register a CCC descriptor.
 *
 *  @param _gp GATT service object with dynamic attribute allocation.
 *  @param _ccc CCC descriptor configuration.
 *  @param _ccc_changed CCC value changed callback.
 *  @param _perm CCC descriptor permissions.
 */
#define BT_GATT_POOL_CCC(_gp, _ccc, _ccc_changed, _perm)                       \
	do {                                                                   \
		int _ret;                                                      \
		_ccc = (struct _bt_gatt_ccc)BT_GATT_CCC_INITIALIZER(\
			_ccc_changed, NULL, NULL);      \
		_ret = bt_gatt_pool_ccc_alloc(_gp, &_ccc, _perm);              \
		__ASSERT_NO_MSG(!_ret);                                        \
		(void)_ret;                                                    \
	} while (0)


/** @brief The GATT service object that uses dynamic attribute allocation.
 *
 * This structure contains the SVC object together with the number of the
 * attributes available.
 */
struct bt_gatt_pool {
	/** GATT service descriptor. */
	struct bt_gatt_service svc;
	/** Maximum number of attributes supported. */
	size_t attr_array_size;
};

/** @brief Take a primary service descriptor from the pool.
 *
 *  @param gp GATT service object with dynamic attribute allocation.
 *  @param svc_uuid Service UUID.
 *
 *  @retval 0 Operation finished successfully.
 *  @retval -EINVAL Invalid input value.
 *  @retval -ENOSPC Number of arguments in @c svc would exceed @c array_size.
 *  @retval -ENOMEM No internal memory in the gatt_poll module.
 */
int bt_gatt_pool_svc_alloc(struct bt_gatt_pool *gp,
			   struct bt_uuid const *svc_uuid);

/** @brief Take a characteristic descriptor from the pool.
 *
 *  @param gp    GATT service object with dynamic attribute allocation.
 *  @param props Properties of the characteristic.
 *  @param attr  Characteristic value attribute.
 *
 *  @return 0 or a negative error code.
 */
int bt_gatt_pool_chrc_alloc(struct bt_gatt_pool *gp, uint8_t props,
			    struct bt_gatt_attr const *attr);

/** @brief Take an attribute descriptor from the pool.
 *
 *  @param gp GATT service object with dynamic attribute allocation.
 *  @param descriptor Attribute descriptor.
 *
 *  @return 0 or negative error code.
 */
int bt_gatt_pool_desc_alloc(struct bt_gatt_pool *gp,
			    struct bt_gatt_attr const *descriptor);

/** @brief Take a CCC descriptor from the pool.
 *
 *  @param gp GATT service object with dynamic attribute allocation.
 *  @param ccc CCC descriptor.
 *  @param perm Permissions to access CCC.
 *              Use the GATT attribute permission bit field values.
 *
 *  @return 0 or negative error code.
 */
int bt_gatt_pool_ccc_alloc(struct bt_gatt_pool *gp,
			   struct _bt_gatt_ccc *ccc,
			   uint8_t perm);

/** @brief Free the whole dynamically created GATT service.
 *
 *  @param gp GATT service object with dynamic attribute allocation.
 */
void bt_gatt_pool_free(struct bt_gatt_pool *gp);

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
