/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DRV_NPM6001_H_
#define _DRV_NPM6001_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "npm6001.h"

/**
 * @file
 * @defgroup drv_npm6001 nPM6001
 * @{
 * @brief nPM6001 sample driver.
 *
 * @details This sample driver contains high level logic for control and event processing
 *          for the nPM6001 power management IC, as well as nRF Connect SDK platform-specific
 *          functions for two-wire (TWI) communication and GPIO handling.
 *
 *          To use this sample driver on a new hardware platform, the platform-specific
 *          functions in @ref drv_npm6001_platform must be ported.
 *          Additionally, if interrupts are enabled, the @ref drv_npm6001_int_read function
 *          should be called when the N_INT signal is asserted by the nPM6001.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DRV_NPM6001_WRITE_SWREADY
/* This define can be used to override SWREADY behavior. By default 'READY' value is written to
 * SWREADY register in @ref drv_npm6001_init, which enables usef of DCDC_MODE pins.
 */
#define DRV_NPM6001_WRITE_SWREADY true
#endif

#ifndef DRV_NPM6001_C3_WORKAROUND
/* This define can be used to override C3 workaround behavior.
 * Should be set to 'true' if nPM6001 pin C3 is connected to low instead of high level.
 */
#define DRV_NPM6001_C3_WORKAROUND false
#endif

/**@brief nPM6001 TWI address. */
#define DRV_NPM6001_TWI_ADDR 0x70

/**@brief DCDC0 minimum voltage [mV]. */
#define DRV_NPM6001_DCDC0_MINV 1800
/**@brief DCDC0 maximum voltage [mV]. */
#define DRV_NPM6001_DCDC0_MAXV 3300
/**@brief DCDC0 voltage selection resolution [mV]. */
#define DRV_NPM6001_DCDC0_RES 100

/**@brief DCDC1 minimum voltage [mV]. */
#define DRV_NPM6001_DCDC1_MINV 700
/**@brief DCDC1 maximum voltage [mV]. */
#define DRV_NPM6001_DCDC1_MAXV 1400
/**@brief DCDC1 voltage selection resolution [mV]. */
#define DRV_NPM6001_DCDC1_RES 50

/**@brief DCDC2 minimum voltage [mV]. */
#define DRV_NPM6001_DCDC2_MINV 1200
/**@brief DCDC2 maximum voltage [mV]. */
#define DRV_NPM6001_DCDC2_MAXV 1400
/**@brief DCDC2 voltage selection resolution [mV]. */
#define DRV_NPM6001_DCDC2_RES 50

/**@brief DCDC3 minimum voltage [mV]. */
#define DRV_NPM6001_DCDC3_MINV 500
/**@brief DCDC3 maximum voltage [mV]. */
#define DRV_NPM6001_DCDC3_MAXV 3300
/**@brief DCDC3 voltage selection resolution [mV]. */
#define DRV_NPM6001_DCDC3_RES 25

/**@brief LDO0 minimum voltage [mV]. */
#define DRV_NPM6001_LDO0_MINV 1800
/**@brief LDO0 maximum voltage [mV]. */
#define DRV_NPM6001_LDO0_MAXV 3300
/**@brief LDO0 voltage selection resolution [mV]. */
#define DRV_NPM6001_LDO0_RES 300

/**@brief LDO1 minimum voltage [mV]. */
#define DRV_NPM6001_LDO1_MINV 1800
/**@brief LDO1 maximum voltage [mV]. */
#define DRV_NPM6001_LDO1_MAXV 1800

/**
 * @brief nPM6001 regulators.
 */
enum drv_npm6001_vreg {
	DRV_NPM6001_DCDC0,
	DRV_NPM6001_DCDC1,
	DRV_NPM6001_DCDC2,
	DRV_NPM6001_DCDC3,
	DRV_NPM6001_LDO0,
	DRV_NPM6001_LDO1,
};

/**
 * @brief nPM6001 regulator mode.
 */
enum drv_npm6001_vreg_mode {
	DRV_NPM6001_MODE_ULP,
	DRV_NPM6001_MODE_PWM,
};

/**
 * @brief nPM6001 interrupt type.
 */
enum drv_npm6001_int {
	DRV_NPM6001_INT_THERMAL_WARNING,
	DRV_NPM6001_INT_DCDC0_OVERCURRENT,
	DRV_NPM6001_INT_DCDC1_OVERCURRENT,
	DRV_NPM6001_INT_DCDC2_OVERCURRENT,
	DRV_NPM6001_INT_DCDC3_OVERCURRENT,
};

/**
 * @brief nPM6001 regulator mode pins.
 */
enum drv_npm6001_mode_pin {
	DRV_NPM6001_DCDC_PIN_MODE0,
	DRV_NPM6001_DCDC_PIN_MODE1,
	DRV_NPM6001_DCDC_PIN_MODE2,
};

/**
 * @brief nPM6001 thermal sensor.
 */
enum drv_npm6001_thermal_sensor {
	DRV_NPM6001_THERMAL_SENSOR_WARNING,
	DRV_NPM6001_THERMAL_SENSOR_SHUTDOWN,
};

/**
 * @brief nPM6001 DC/DC mode pin configuration.
 */
struct drv_npm6001_mode_pin_cfg {
	enum {
		DRV_NPM6001_MODE_PIN_CFG_PAD_TYPE_SCHMITT,
		DRV_NPM6001_MODE_PIN_CFG_PAD_TYPE_CMOS,
	} pad_type;

	enum {
		DRV_NPM6001_MODE_PIN_CFG_PULLDOWN_ENABLED,
		DRV_NPM6001_MODE_PIN_CFG_PULLDOWN_DISABLED,
	} pulldown;
};


/**
 * @brief nPM6001 general purpose I/O (GENIO).
 */
enum drv_npm6001_genio {
	DRV_NPM6001_GENIO0,
	DRV_NPM6001_GENIO1,
	DRV_NPM6001_GENIO2,
};

/**
 * @brief nPM6001 general purpose I/O (GENIO) configuration.
 */
struct drv_npm6001_genio_cfg {
	enum {
		DRV_NPM6001_GENIO_CFG_DIRECTION_INPUT,
		DRV_NPM6001_GENIO_CFG_DIRECTION_OUTPUT,
	} direction;

	enum {
		DRV_NPM6001_GENIO_CFG_INPUT_ENABLED,
		DRV_NPM6001_GENIO_CFG_INPUT_DISABLED,
	} input;

	enum {
		DRV_NPM6001_GENIO_CFG_PULLDOWN_ENABLED,
		DRV_NPM6001_GENIO_CFG_PULLDOWN_DISABLED,
	} pulldown;

	enum {
		DRV_NPM6001_GENIO_CFG_DRIVE_NORMAL,
		DRV_NPM6001_GENIO_CFG_DRIVE_HIGH,
	} drive;

	enum {
		DRV_NPM6001_GENIO_CFG_SENSE_LOW,
		DRV_NPM6001_GENIO_CFG_SENSE_HIGH,
	} sense;
};

/**
 * @brief Platform-specific functions for TWI and GPIO control.
 */
struct drv_npm6001_platform {
	/**@brief Initialize platform-specifics. Can be NULL if not needed.
	 *
	 * @return 0 if successful. Otherwise an error code.
	 */
	int (*drv_npm6001_platform_init)(void);

	/**@brief Read bytes from register.
	 *
	 * @param[out] buf Buffer to hold read values.
	 * @param[in] len Number of bytes to read.
	 * @param[in] reg_addr Register address.
	 *
	 * @return Number of bytes read.
	 */
	int (*drv_npm6001_twi_read)(uint8_t *buf, uint8_t len, uint8_t reg_addr);

	/**@brief Write bytes to register.
	 *
	 * @param[in] buf Values to write.
	 * @param[in] len Number of bytes to write.
	 * @param[in] reg_addr Register address.
	 *
	 * @return Number of bytes written.
	 */
	int (*drv_npm6001_twi_write)(const uint8_t *buf, uint8_t len, uint8_t reg_addr);
};

/**@brief Initialize nPM6001 sample driver.
 *
 * @param[in] hw_funcs Platform-specific functions.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_init(const struct drv_npm6001_platform *hw_funcs);

/**@brief Read nPM6001 interrupt source.
 *
 * @details Should be called while N_INT pin is low, as interrupt sources are cleared one at a time.
 *
 * @param[out] interrupt Interrupt source.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_int_read(enum drv_npm6001_int *interrupt);

/**@brief Enable interrupt.
 *
 * @param[in] interrupt Interrupt to enable.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_int_enable(enum drv_npm6001_int interrupt);

/**@brief Disable interrupt.
 *
 * @param[in] interrupt Interrupt to disable.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_int_disable(enum drv_npm6001_int interrupt);

/**@brief Enable voltage regulator.
 *
 * @param[in] regulator Regulator to enable.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_vreg_enable(enum drv_npm6001_vreg regulator);

/**@brief Disable voltage regulator.
 *
 * @note Regulators DCDC0, DCDC1, and DCDC2 are always on.
 *
 * @param[in] regulator Regulator to disable.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_vreg_disable(enum drv_npm6001_vreg regulator);

/**@brief Set mode for DC/DC regulator.
 *
 * @param[in] regulator Regulator to adjust.
 * @param[in] mode DCDC mode to set.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_vreg_dcdc_mode_set(enum drv_npm6001_vreg regulator,
	enum drv_npm6001_vreg_mode mode);

/**@brief Enable mode pin control for DC/DC regulator.
 *
 * @details Mode pins can be used to force PWM mode for one or more of the DC/DC regulators.
 *
 * @param[in] regulator Regulator to enable pin mode control for.
 * @param[in] pin Mode pin.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_vreg_mode_pin_enable(
	enum drv_npm6001_vreg regulator,
	enum drv_npm6001_mode_pin pin);

/**@brief Configure mode pin.
 *
 * @param[in] pin Mode pin.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_mode_pin_cfg(enum drv_npm6001_mode_pin pin,
	const struct drv_npm6001_mode_pin_cfg *cfg);

/**@brief Set regulator voltage.
 *
 * @details An error is returned if target voltage is outside of allowed range.
 *          Within the allowed range, the target value is rounded to the closest configurable value.
 *
 * @param[in] regulator Regulator to adjust.
 * @param[in] voltage Desired voltage in mV.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_vreg_voltage_set(enum drv_npm6001_vreg regulator, uint16_t voltage);


/**@brief Enable thermal sensor.
 *
 * @param[in] sensor Sensor function to enable.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_thermal_sensor_enable(enum drv_npm6001_thermal_sensor sensor);

/**@brief Disable thermal sensor.
 *
 * @param[in] sensor Sensor function to disable.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_thermal_sensor_disable(enum drv_npm6001_thermal_sensor sensor);

/**@brief Enable thermal sensor dynamic powerup.
 *
 * @details Power on thermal sensor dynamically function when designated DCDC regulator(s)
 *          enter PWM mode.
 *
 * @param[in] sensor Sensor function to enable dynamic powerup for.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_thermal_sensor_dyn_pwrup_enable(enum drv_npm6001_thermal_sensor sensor);

/**@brief Set thermal sensor dynamic powerup trigger.
 *
 * @param[in] vreg DCDC regulator(s) that should trigger dynamic powerup.
 * @param[in] len Number of DCDC regulators.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_thermal_sensor_dyn_pwrup_trig(const enum drv_npm6001_vreg *vreg, uint8_t len);


/**@brief Disable thermal sensor dynamic powerup.
 *
 * @param[in] sensor Sensor function to disable dynamic powerup for.
 * @param[in] vreg Regulator that will cause sensor start.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_thermal_sensor_dyn_pwrup_disable(enum drv_npm6001_thermal_sensor sensor);

/**@brief Configure nPM6001 general purpose I/O.
 *
 * @param[in] pin GENIO pin to configure.
 * @param[in] cfg Configuration parameters.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_genio_cfg(enum drv_npm6001_genio pin, const struct drv_npm6001_genio_cfg *cfg);

/**@brief Set general purpose I/O (high level).
 *
 * @param[in] pin GENIO pin to set.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_genio_set(enum drv_npm6001_genio pin);

/**@brief Clear general purpose I/O (low level).
 *
 * @param[in] pin GENIO pin to clear.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_genio_clr(enum drv_npm6001_genio pin);

/**@brief Read general purpose I/O level.
 *
 * @param[in] pin GENIO pin to read.
 * @param[out] set True if level is high.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_genio_read(enum drv_npm6001_genio pin, bool *set);

/**@brief Enable watchdog.
 *
 * @details The watchdog uses a 24-bit timer to generate a system reset when the desired
 *          timer value is reached. Once started, it must be kicked/reset before
 *          the timeout expires, otherwise a reset is triggered.
 *          The timer value is in 4 second ticks.
 *
 * @param[in] timer_val 24-bit watchdog timer value [4s units].
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_watchdog_enable(uint32_t timer_val);

/**@brief Disable watchdog.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_watchdog_disable(void);

/**@brief Kick (reset) the watchdog counter.
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_watchdog_kick(void);

/**@brief Enter hibernation.
 *
 * @details In hibernation mode, all functionality except the wakeup timer is disabled.
 *          Once the timeout expires, the device wakes up again.
 *          The timer value is in 4 second ticks.
 *
 * @param[in] timer_val 24-bit hibernate timer value [4s units].
 *
 * @return 0 if successful. Otherwise a negative error code.
 */
int drv_npm6001_hibernate(uint32_t timer_val);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DRV_NPM6001_H_ */
