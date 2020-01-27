/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @defgroup bt_enocean Bluetooth EnOcean library API
 * @{
 * @brief API for interacting with EnOcean Bluetooth wall switches and sensors
 */

#ifndef BT_ENOCEAN_H__
#define BT_ENOCEAN_H__

#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Double rocker switch top left button (OA) */
#define BT_ENOCEAN_SWITCH_OA BIT(0)
/** Double rocker switch bottom left button (IA) */
#define BT_ENOCEAN_SWITCH_IA BIT(1)
/** Double rocker switch top right button (OB) */
#define BT_ENOCEAN_SWITCH_OB BIT(2)
/** Double rocker switch bottom right button (IB) */
#define BT_ENOCEAN_SWITCH_IB BIT(3)

/** Single rocker switch top button (O) */
#define BT_ENOCEAN_SWITCH_O BT_ENOCEAN_SWITCH_OB
/** Single rocker switch bottom button (I) */
#define BT_ENOCEAN_SWITCH_I BT_ENOCEAN_SWITCH_IB

/** EnOcean sensor data message. */
struct bt_enocean_sensor_data {
	bool *occupancy; /**< Current occupancy status. */
	u16_t *light_sensor; /**< Light level in sensor (lx). */
	u16_t *battery_voltage; /**< Backup battery voltage (mV). */
	u16_t *light_solar_cell; /**< Light level in solar cell (lx). */
	u8_t *energy_lvl; /**< Energy level (percent). */
};

/** EnOcean device representation. */
struct bt_enocean_device {
	u32_t seq;  /**< Most recent sequence number. */
	u8_t flags; /**< Internal flags field. */
	s8_t rssi; /**< Most recent RSSI measurement. */
	bt_addr_le_t addr; /**< Device address. */
	u8_t key[16]; /**< Device key. */
};

/** Type of button event */
enum bt_enocean_button_action {
	BT_ENOCEAN_BUTTON_RELEASE, /**< Buttons were released. */
	BT_ENOCEAN_BUTTON_PRESS, /**< Buttons were pressed. */
};

/** EnOcean callback functions. All callbacks are optional. */
struct bt_enocean_callbacks {
	/** @brief Callback for EnOcean Switch button presses.
	 *
	 *  This callback is called for every new button message
	 *  from a commissioned EnOcean Switch. The set of changed button
	 *  states on the switch is represented as a bitfield, with 4 bits
	 *  representing the individual buttons.
	 *
	 *  See the @c BT_ENOCEAN_SWITCH_* defines for button mapping. Note that
	 *  the single rocker switches only engages the third and fourth
	 *  buttons.
	 *
	 *  @param device       EnOcean device generating the message.
	 *  @param action       Button action type.
	 *  @param changed      Bitfield of buttons affected by the action.
	 *  @param opt_data     Optional data array from the message, or NULL.
	 *  @param opt_data_len Length of the optional data.
	 */
	void (*button)(struct bt_enocean_device *device,
		       enum bt_enocean_button_action action, u8_t changed,
		       const u8_t *opt_data, size_t opt_data_len);

	/** @brief Callback for EnOcean Sensor reports.
	 *
	 *  This callback is called for every new sensor message from a
	 *  commissioned EnOcean Switch.
	 *
	 *  The @c data parameter contains a set of optional fields, represented
	 *  as pointers. Unknown fields are NULL pointers, and must be ignored.
	 *
	 *  @param device       EnOcean device generating the message.
	 *  @param data         Sensor data.
	 *  @param opt_data     Optional data array from the message, or NULL.
	 *  @param opt_data_len Length of the optional data.
	 */
	void (*sensor)(struct bt_enocean_device *device,
		       const struct bt_enocean_sensor_data *data,
		       const u8_t *opt_data, size_t opt_data_len);

	/** @brief Callback for EnOcean commissioning.
	 *
	 *  This callback is called for every new EnOcean device being
	 *  commissioned.
	 *
	 *  All devices must be commissioned before they can be observed.
	 *  See the EnOcean device's user guide for details.
	 *
	 *  A device can be rejected inside the callback by calling
	 *  @ref bt_enocean_decommission with the @c device's address.
	 *
	 *  @param device Device that got commissioned.
	 */
	void (*commissioned)(struct bt_enocean_device *device);

	/** @brief Callback for EnOcean devices being loaded from
	 *         persistent storage.
	 *
	 *  This callback is called for every commissioned EnOcean device
	 *  recovered from persistent storage.
	 *
	 *  @param device Device that was loaded.
	 */
	void (*loaded)(struct bt_enocean_device *device);
};

/** @brief Visitor callback for @ref bt_enocean_foreach.
 *
 *  @param dev       EnOcean device
 *  @param user_data User data passed to @ref bt_enocean_foreach.
 */
typedef void (*bt_enocean_foreach_cb_t)(struct bt_enocean_device *dev,
				     void *user_data);

/** @brief Initialize the EnOcean subsystem.
 *
 *  @param cb Set of callbacks to receive events with.
 */
void bt_enocean_init(const struct bt_enocean_callbacks *cb);

/** @brief Enable automatic commissioning.
 *
 *  This function must be called for the module to start accepting
 *  commissioning packets.
 */
void bt_enocean_commissioning_enable(void);

/** @brief Disable automatic commissioning.
 *
 *  Automatic commissioning will be disabled until
 *  @ref bt_enocean_commissioning_enable is called. Manual commissioning
 *  through @ref bt_enocean_commission is unaffected.
 */
void bt_enocean_commissioning_disable(void);

/** @brief Manually commission an EnOcean device.
 *
 *  Allows devices to be commissioned based on out of band data, like NFC or QR
 *  codes.
 *
 *  @param addr Address of the commissioned device.
 *  @param key  Encryption key for the commissioned device.
 *  @param seq  Most recent sequence number of the device.
 *
 *  @retval 0          The device has been successfully commissioned.
 *  @retval -EEXIST    This device has already been commissioned.
 *  @retval -ENOMEM    There's not enough space to commission a new device.
 *  @retval -ECANCELED The commissioning callback rejected the device.
 *  @retval -EBUSY     Failed allocating persistent storage data.
 */
int bt_enocean_commission(const bt_addr_le_t *addr, const u8_t key[16],
			  u32_t seq);

/** @brief Forget an EnOcean device.
 *
 *  Removes the EnOcean device's security information, freeing up space for new
 *  devices. May be used directly in the
 *  @ref bt_enocean_callbacks::commissioning callback to reject a device before
 *  it gets commissioned.
 *
 *  @param dev Device to forget.
 */
void bt_enocean_decommission(struct bt_enocean_device *dev);

/** @brief Call the given callback for each commissioned enocean device.
 *
 *  @param cb        Callback to call for each device
 *  @param user_data User data to pass to the callback.
 *
 *  @return The number of devices visited.
 */
u32_t bt_enocean_foreach(bt_enocean_foreach_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* BT_ENOCEAN_H__ */
