/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_DM_H_
#define BT_GATT_DM_H_

/**
 * @file
 * @defgroup bt_gatt_dm GATT Discovery Manager API
 * @{
 * @brief Module for GATT Discovery Manager.
 */

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Discovery Manager instance
 *
 * The instance of the manager used by most of the functions here.
 */
struct bt_gatt_dm;

/** @brief Discovery Manager GATT attribute
 *
 * This structure is used to hold GATT attribute information.
 * The Discovery Manager attribute descriptor is a reduced version of
 * @ref bt_gatt_attr. The new definition is used to save memory that is used
 * by this module.
 */
struct bt_gatt_dm_attr {
	/** Attribute UUID */
	struct bt_uuid	*uuid;
	/** Attribute handle */
	u16_t		handle;
	/** Attribute permissions */
	u8_t		perm;
};

/** @brief Discovery callback structure.
 *
 *  This structure is used for tracking the result of a discovery.
 */
struct bt_gatt_dm_cb {
	/** @brief Discovery completed callback.
	 *
	 * The discovery procedure has completed successfully.
	 *
	 * @note You must release the discovery data with
	 * @ref bt_gatt_dm_data_release if you want to start another
	 * discovery.
	 *
	 * @param[in,out] dm Discovery Manager instance
	 * @param[in,out] context The value passed to
	 *                @ref gatt_db_discovery_start
	 */
	void (*completed)(struct bt_gatt_dm *dm,
			  void *context);

	/** @brief Service not found callback.
	 *
	 * The targeted service could not be found during the discovery.
	 *
	 * @param[in,out] conn Connection object.
	 * @param[in,out] context The value passed to
	 *                @ref gatt_db_discovery_start
	 */
	void (*service_not_found)(struct bt_conn *conn,
				  void *context);

	/** @brief Discovery error found callback.
	 *
	 * The discovery procedure has failed.
	 *
	 * @param[in,out] conn Connection object.
	 * @param[in]     err The error code.
	 * @param[in,out] context The value passed to
	 *                @ref gatt_db_discovery_start
	 */
	void (*error_found)(struct bt_conn *conn,
			    int err,
			    void *context);
};

/** @brief Access service value saved with service attribute
 *
 * This function access the service value parsed and saved previously
 * in the user_data attribute field.
 *
 * @note Use it only on the attribute parsed in this module.
 *       To access service attribute use @ref bt_gatt_dm_service_get function.
 *
 * @param[in] attr Service attribute
 *
 * @return The service value from the parsed attribute
 *         or NULL when attribute UUID value is unexpected.
 */
struct bt_gatt_service_val *bt_gatt_dm_attr_service_val(
	const struct bt_gatt_dm_attr *attr);

/** @brief Access characteristic value saved with characteristic attribute
 *
 * This function access the characteristic value parsed
 * and saved previously in the user_data attribute field.
 *
 * @note Use it only on the attribute parsed in this module.
 *       To access characteristic attribute use
 *       @ref bt_gatt_dm_char_next function.
 *
 * @param[in] attr Characteristic attribute
 *
 * @return The characteristic value from parser attribute
 *         or NULL when attribute UUID value is unexpected.
 */
struct bt_gatt_chrc *bt_gatt_dm_attr_chrc_val(
	const struct bt_gatt_dm_attr *attr);

/** @brief Get the connection object
 *
 * Function returns connection object that is used by given
 * discovery manager instance.
 *
 * @param[in] dm Discovery Manager instance
 *
 * @return Connection object.
 */
struct bt_conn *bt_gatt_dm_conn_get(struct bt_gatt_dm *dm);

/** @brief Get total number of attributes decoded
 *
 * The number of attributes including the service attribute.
 * It means that service without any attribute would return 1 here.
 *
 * @param[in] dm Discovery Manager instance.
 *
 * @return Total number of attributes parsed.
 */
size_t bt_gatt_dm_attr_cnt(const struct bt_gatt_dm *dm);

/** @brief Get service value
 *
 * Function returns the value that contains UUID and attribute
 * end handler of the service found.
 *
 * @param[in] dm Discovery Manager instance.
 *
 * @return The pointer service value structure.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_service_get(
	const struct bt_gatt_dm *dm);

/** @brief Get next characteristic
 *
 * @param[in] dm Discovery Manager instance.
 * @param[in] prev An attribute where start to search.
 *            If NULL - the first characteristic in the service would be found.
 *            Note: It can be the previous characteristic attribute or the
 *            last descriptor inside the previous attribute.
 *            Function would start searching for the next characteristic
 *            from that point.
 *
 * @return The pointer for an attribute that describes the characteristic
 *         or NULL if no more characteristic is present.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_char_next(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_dm_attr *prev);

/** @brief Get the characteristic by its UUID
 *
 * Function finds characteristic attribute by the UUID stored in its
 * characteristic value.
 * If the selected characteristic is not found in parsed service
 * it returns NULL.
 *
 * @param[in] dm Discovery instance
 * @param[in] uuid The UUID of the characteristic
 *
 * @return The characteristic attribute
 *         (the one with UUID set to @ref BT_UUID_GATT_CHRC)
 *         with the selected UUID inside the characteristic value.
 *         Returns NULL if no such characteristic is present in the
 *         current service.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_char_by_uuid(
	const struct bt_gatt_dm *dm,
	const struct bt_uuid *uuid);

/** @brief Get attribute by handle
 *
 * Function returns any type of the attribute using its handle.
 *
 * @param[in] dm Discovery Manager instance.
 * @param[in] handle The handle to find
 *
 * @return The pointer to the attribute or NULL if there is no
 *         attribute with such a pointer.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_attr_by_handle(
	const struct bt_gatt_dm *dm, u16_t handle);

/** @brief Get next attribute
 *
 * Function returns the attribute next to the given one.
 * It returns any type of the attribute.
 *
 * @param[in] dm Discovery Manager instance.
 * @param[in] prev Previous attribute or NULL if we wish to get
 *            first attribute (just after service).
 *
 * @return Attribute next to the @c prev
 *         or the first attribute if NULL is given.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_attr_next(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_dm_attr *prev);


/** @brief Search the descriptor by UUID
 *
 * Function searches for the descriptor with given UUID
 * inside given characteristic.
 *
 * @param[in] dm Discovery Manager instance.
 * @param[in] attr_chrc The characteristic attribute where to search
 * @param[in] uuid The UUID of the searched descriptor.
 *
 * @return Pointer to the attribute or NULL if the attribute cannot be found.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_desc_by_uuid(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_dm_attr *attr_chrc,
	const struct bt_uuid *uuid);

/** @brief Get next descriptor
 *
 * Function returns next descriptor.
 * The difference between this function and @ref bt_gatt_dm_attr_next is that
 * it returns NULL also when returned attribute appears to be next
 * characteristic.
 *
 * @param[in] dm Discovery Manager instance.
 * @param[in] prev Previous attribute.
 *            The characteristic if we wish to get first descriptor
 *            or previous descriptor.
 *            If NULL or pointer to service attribute is given
 *            the result is undefined.
 *
 * @return The pointer to the descriptor attribute
 *         or NULL if there is no more descriptors in the characteristic.
 */
const struct bt_gatt_dm_attr *bt_gatt_dm_desc_next(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_dm_attr *prev);

/** @brief Start service discovery.
 *
 * This function is asynchronous. Discovery results are passed through
 * the supplied callback.
 *
 * @note Only one discovery procedure can be started simultaneously. To start
 * another one, wait for the result of the previous procedure to finish
 * and call @ref bt_gatt_dm_data_release if it was successful.
 *
 * @param[in]     conn Connection object.
 * @param[in]     svc_uuid UUID of target service
 *                or NULL if any service should be discovered.
 * @param[in]     cb Callback structure.
  * @param[in,out] context Context argument to be passed to
  *                callback functions.
 *
 * @note
 * If @p svc_uuid is set to NULL, all services may be discovered.
 * To process the next service, call @ref bt_gatt_dm_continue.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dm_start(struct bt_conn *conn,
		     const struct bt_uuid *svc_uuid,
		     const struct bt_gatt_dm_cb *cb,
		     void *context);

/** @brief Continue service discovery.
 *
 * This function continues service discovery.
 * Call it after the previous data was released by @ref bt_gatt_dm_data_release.
 *
 * @param[in,out] dm Discovery Manager instance.
 * @param[in]     context Context argument to
 *                be passed to callback functions.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dm_continue(struct bt_gatt_dm *dm, void *context);

/** @brief Release data associated with service discovery.
 *
 * After calling this function, you cannot rely on the discovery data that was
 * passed with the discovery completed callback (see @ref bt_gatt_dm_cb).
 *
 * @param[in] dm Discovery Manager instance
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dm_data_release(struct bt_gatt_dm *dm);

/** @brief Print service discovery data.
 *
 * This function prints GATT attributes that belong to the discovered service.
 *
 * @param[in] dm Discovery Manager instance
 */
#ifdef CONFIG_BT_GATT_DM_DATA_PRINT
void bt_gatt_dm_data_print(const struct bt_gatt_dm *dm);
#else
static inline void bt_gatt_dm_data_print(const struct bt_gatt_dm *dm)
{
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_DM_H_ */
