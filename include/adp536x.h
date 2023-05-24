/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ADP536X_H_
#define ADP536X_H_

/**@file adp536x.h
 *
 * @brief Driver for ADP536X.
 * @defgroup adp536x Driver for ADP536X
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>

/* Definition of VBUS current limit values. */
#define ADP536X_VBUS_ILIM_50mA		0x00
#define ADP536X_VBUS_ILIM_100mA		0x01
#define ADP536X_VBUS_ILIM_150mA		0x02
#define ADP536X_VBUS_ILIM_200mA		0x03
#define ADP536X_VBUS_ILIM_250mA		0x04
#define ADP536X_VBUS_ILIM_300mA		0x05
#define ADP536X_VBUS_ILIM_400mA		0x06
#define ADP536X_VBUS_ILIM_500mA		0x07

/* Definition of charging current values. */
#define ADP536X_CHG_CURRENT_10mA        0x00
#define ADP536X_CHG_CURRENT_50mA        0x04
#define ADP536X_CHG_CURRENT_100mA	0x09
#define ADP536X_CHG_CURRENT_150mA	0x0E
#define ADP536X_CHG_CURRENT_200mA	0x13
#define ADP536X_CHG_CURRENT_250mA	0x18
#define ADP536X_CHG_CURRENT_300mA	0x1D
#define ADP536X_CHG_CURRENT_320mA	0x1F

/* Definition of overcharge protection threshold values. */
#define ADP536X_OC_CHG_THRESHOLD_25mA	0x00
#define ADP536X_OC_CHG_THRESHOLD_50mA	0x01
#define ADP536X_OC_CHG_THRESHOLD_100mA	0x02
#define ADP536X_OC_CHG_THRESHOLD_150mA	0x03
#define ADP536X_OC_CHG_THRESHOLD_200mA	0x04
#define ADP536X_OC_CHG_THRESHOLD_250mA	0x05
#define ADP536X_OC_CHG_THRESHOLD_300mA	0x06
#define ADP536X_OC_CHG_THRESHOLD_400mA	0x07

/** Fuel gauge state */
enum adp536x_fg_enabled {
	ADP566X_FG_DISABLED = 0,        /**< Disable fuel gauge */
	ADP566X_FG_ENABLED = 1          /**< Enable fuel gauge */
};

/** Fuel gauge mode */
enum adp536x_fg_mode {
	ADP566X_FG_MODE_ACTIVE = 0,     /** Fuel gauge is in active mode */
	ADP566X_FG_MODE_SLEEP = 1       /** Fuel gauge is in sleep mode */
};

/**
 * @brief Initialize ADP536X.
 *
 * @param[in] dev Pointer to the I2C bus device.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_init(const struct device *dev);

/**
 * @brief Set the VBUS current limit.
 *
 * @param[in] value The upper current threshold in LSB.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_vbus_current_set(uint8_t value);

/**
 * @brief Set the charger current.
 *
 * @param[in] value The charger current in LSB.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charger_current_set(uint8_t value);

/**
 * @brief Set the charger termination voltage.
 *
 * This function configures the maximum battery voltage where
 * the charger remains active.
 *
 * @param[in] value The charger termination voltage.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charger_termination_voltage_set(uint8_t value);

/**
 * @brief Enable charging.
 *
 * @param[in] enable The requested charger operation state.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charging_enable(bool enable);

/**
 * @brief Read the STATUS1 register.
 *
 * @param[out] buf The read value of the STATUS1 register.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charger_status_1_read(uint8_t *buf);

/**
 * @brief Read the STATUS2 register.
 *
 * @param[out] buf The read value of the STATUS2 register.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charger_status_2_read(uint8_t *buf);

/**
 * @brief Enable charge hiccup protection mode.
 *
 * @param[in] enable The requested hiccup protection state.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_oc_chg_hiccup_set(bool enable);

/**
 * @brief Enable discharge hiccup protection mode.
 *
 * @param[in] enable The requested hiccup protection state.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_oc_dis_hiccup_set(bool enable);

/**
 * @brief Reset the device to its default values.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_factory_reset(void);

/**
 * @brief Set the charge over-current threshold.
 *
 * @param[in] value The over-current threshold.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_oc_chg_current_set(uint8_t value);

/**
 * @brief Set Fuel Gauge operating mode
 *
 * @param[in] en Enable or disable the fuel gauge.
 * @param[in] mode Active or sleep mode.
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_fg_set_mode(enum adp536x_fg_enabled en, enum adp536x_fg_mode mode);

/**
 * @brief Read battery state of charge
 *
 * @param[out] percentage Percentage of battery remaining
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_fg_soc(uint8_t *percentage);

/**
 * @brief Read battery voltage
 *
 * @param[out] millivolts Battery voltage in millivolts
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_fg_volts(uint16_t *millivolts);

/** @} */

#endif /* ADP536X_H_ */
