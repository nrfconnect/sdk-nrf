/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_NSMS_H_
#define BT_NSMS_H_

/**
 * @file
 * @defgroup bt_nsms Nordic Status Message (NSMS) GATT Service
 * @{
 * @brief Nordic Status Message (NSMS) GATT Service API.
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the NSMS Service. **/
#define BT_UUID_NSMS_VAL \
	BT_UUID_128_ENCODE(0x57a70000, 0x9350, 0x11ed, 0xa1eb, 0x0242ac120002)

/** @brief UUID of the Status characteristic. **/
#define BT_UUID_NSMS_STATUS_VAL \
	BT_UUID_128_ENCODE(0x57a70001, 0x9350, 0x11ed, 0xa1eb, 0x0242ac120002)

#define BT_UUID_NSMS_SERVICE   BT_UUID_DECLARE_128(BT_UUID_NSMS_VAL)
#define BT_UUID_NSMS_STATUS    BT_UUID_DECLARE_128(BT_UUID_NSMS_STATUS_VAL)


/** @brief The status */
struct bt_nsms_status_str {
	/** Data access mutex */
	struct k_mutex *mtx;
	/** The size of the buffer */
	size_t size;
	/** Pointer to the buffer */
	char *buf;
};

/** @brief Nordic Status Message Service structure
 *
 * The structure that allows multi-instance implementation of the NSMS.
 */
struct bt_nsms {
	/** Pointer to the status attribute */
	const struct bt_gatt_attr *status_attr;
};

/** @brief Nordic Status Message Service reading command
 *
 * This is implementation used by BLE stack.
 *
 * @note
 * Do not use it from the code.
 */
ssize_t bt_nsms_status_read(struct bt_conn *conn,
			    struct bt_gatt_attr const *attr,
			    void *buf, uint16_t len, uint16_t offset);

#define BT_NSMS_SECURITY_LEVEL_NONE	0
#define BT_NSMS_SECURITY_LEVEL_ENCRYPT	1
#define BT_NSMS_SECURITY_LEVEL_AUTHEN	2

#define _BT_NSMS_CH_READ_PERM(_slevel) ((_slevel == BT_NSMS_SECURITY_LEVEL_AUTHEN) ?  \
					(BT_GATT_PERM_READ_AUTHEN) : \
					(_slevel == BT_NSMS_SECURITY_LEVEL_ENCRYPT) ? \
					(BT_GATT_PERM_READ_ENCRYPT) : \
					(BT_GATT_PERM_READ) \
					)

#define _BT_NSMS_CH_WRITE_PERM(_slevel) ((_slevel == BT_NSMS_SECURITY_LEVEL_AUTHEN) ? \
					(BT_GATT_PERM_WRITE_AUTHEN) : \
					(_slevel == BT_NSMS_SECURITY_LEVEL_ENCRYPT) ? \
					(BT_GATT_PERM_WRITE_ENCRYPT) : \
					(BT_GATT_PERM_WRITE) \
					)

/* @note:
 * The macro is required to provide proper arguments expansion as we are using name concatenation.
 * This allow proper double argument expansion.
 */
#define BT_NSMS_SERVICE_DEF(_name, ...) \
	BT_GATT_SERVICE_DEFINE(_name, __VA_ARGS__)

/** @brief Create the Nordic Status Message Service instance
 *
 * This macro creates the Nordic Status Message Service instance and prepares it to work.
 * Note that it allows to implement multiple instances and the only difference between them would be
 * the name.
 *
 * @param _nsms        Name of Status Message Service instance.
 * @param _name        NULL terminated string used as a CUD for Status Message Characteristic.
 * @param _slevel      Security level to use the characteristic.
 *			- BT_NSMS_SECURITY_LEVEL_AUTHEN			- requires authentication
 *			- BT_NSMS_SECURITY_LEVEL_ENCRYPT		- encryption is sufficient
 *			- BT_NSMS_SECURITY_LEVEL_NONE and other values	- none
 * @param _status_init Initial message.
 * @param _len_max     Maximal size of the message. The size of the message buffer.
 */
#define BT_NSMS_DEF(_nsms, _name, _slevel, _status_init, _len_max)                      \
	static K_MUTEX_DEFINE(CONCAT(_nsms, _status_mtx));                              \
	static char CONCAT(_nsms, _status_buf)[_len_max] = _status_init;                \
	static const struct bt_nsms_status_str CONCAT(_nsms, _status_str) = {           \
		.mtx = &CONCAT(_nsms, _status_mtx),                                     \
		.size = (_len_max),                                                     \
		.buf = CONCAT(_nsms, _status_buf)                                       \
	};                                                                              \
	BT_NSMS_SERVICE_DEF(CONCAT(_nsms, _svc),                                        \
		BT_GATT_PRIMARY_SERVICE(BT_UUID_NSMS_SERVICE),                          \
			BT_GATT_CHARACTERISTIC(BT_UUID_NSMS_STATUS,                     \
					       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
					       _BT_NSMS_CH_READ_PERM(_slevel),          \
					       bt_nsms_status_read,                     \
					       NULL,                                    \
					       (void *)&CONCAT(_nsms, _status_str)),    \
			BT_GATT_CCC(NULL, _BT_NSMS_CH_READ_PERM(_slevel) |              \
					  _BT_NSMS_CH_WRITE_PERM(_slevel)),             \
			BT_GATT_CUD(_name, _BT_NSMS_CH_READ_PERM(_slevel)),             \
	);                                                                              \
	static const struct bt_nsms _nsms = {                                           \
		.status_attr = &CONCAT(_nsms, _svc).attrs[2],                           \
	}

/**@brief Set status
 *
 * This function sets status message and send notification to a connected peer,
 * or all connected peers.
 *
 * @param[in] nsms_obj Pointer to Nordic Status Message instance.
 * @param[in] status   Pointer to a status string.
 *
 * @retval 0 If the data is sent.
 *           Otherwise, a negative value is returned.
 */
int bt_nsms_set_status(const struct bt_nsms *nsms_obj, const char *status);


#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* BT_NSMS_H_ */
