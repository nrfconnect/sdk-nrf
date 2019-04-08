/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ADP536X_H_
#define ADP536X_H_

/**@file adp536x.h
 *
 * @brief Driver for ADP536X.
 * @defgroup adp536x Driver for ADP536X
 * @{
 */

#include <zephyr.h>

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

/**
 * @brief Initialize ADP536X.
 *
 * @param[in] dev_name Pointer to the device name.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_init(const char *dev_name);

/**
 * @brief Set the VBUS current limit.
 *
 * @param[in] value The upper current threshold in LSB.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_vbus_current_set(u8_t value);

/**
 * @brief Set the charger current.
 *
 * @param[in] value The charger current in LSB.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charger_current_set(u8_t value);

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
int adp536x_charger_termination_voltage_set(u8_t value);

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
int adp536x_charger_status_1_read(u8_t *buf);

/**
 * @brief Read the STATUS2 register.
 *
 * @param[out] buf The read value of the STATUS2 register.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_charger_status_2_read(u8_t *buf);

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
 * @brief Enable the buck/boost regulator.
 *
 * @param[in] enable The requested regulator operation state.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_buckbst_enable(bool enable);

/**
 * @brief Set the buck regulator to 1.8 V.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_buck_1v8_set(void);

/**
 * @brief Set the buck/boost regulator to 3.3 V.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_buckbst_3v3_set(void);

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
int adp536x_oc_chg_current_set(u8_t value);

/**
 * @brief Set the buck discharge resistor status.
 *
 * @param[in] enable Boolean value to enable or disable the discharge resistor.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int adp536x_buck_discharge_set(bool enable);

#endif /* ADP536X_H_ */
