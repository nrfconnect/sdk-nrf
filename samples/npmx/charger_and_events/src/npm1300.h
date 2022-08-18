/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NPM1300_H__
#define NPM1300_H__

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include "mdk/npm1300.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NPM1300_REG_TO_ADDR(x) ((uint16_t)(uint32_t)(&(x)))

/**
 * @brief TODO: Macro for placing a runtime assertion. Just dummy expression
 *
 * @param expression Expression to evaluate.
 */
#define NPMX_ASSERT(expression) ((void)(expression))

/**
 * @brief TODO: Macro for placing a compile time assertion.
 *
 * @param expression Expression to be evaluated.  Just dummy expression
 */
#define NPMX_STATIC_ASSERT(expression)

/**
 * @brief Error log for internal reset causes.
 * Cleared with TASK_CLR_ERRLOG
 */
typedef enum
{
    NPM1300_ERRLOG_RSTCAUSE_SHIPMODEEXIT_MASK       = ERRLOG_RSTCAUSE_SHIPMODEEXIT_Msk,       ///< Internal reset caused by shipmode exit
    NPM1300_ERRLOG_RSTCAUSE_BOOTMONITORTIMEOUT_MASK = ERRLOG_RSTCAUSE_BOOTMONITORTIMEOUT_Msk, ///< Internal reset caused by boot monitor timeout
    NPM1300_ERRLOG_RSTCAUSE_WATCHDOGTIMEOUT_MASK    = ERRLOG_RSTCAUSE_WATCHDOGTIMEOUT_Msk,    ///< Internal reset caused by watchdog timeout
    NPM1300_ERRLOG_RSTCAUSE_LONGPRESSTIMEOUT_MASK   = ERRLOG_RSTCAUSE_LONGPRESSTIMEOUT_Msk,   ///< Internal reset caused by shphld long press
    NPM1300_ERRLOG_RSTCAUSE_THERMALSHUTDOWN_MASK    = ERRLOG_RSTCAUSE_THERMALSHUTDOWN_Msk,    ///< Internal reset caused by TSD
    NPM1300_ERRLOG_RSTCAUSE_VSYSLOW_MASK            = ERRLOG_RSTCAUSE_VSYSLOW_Msk,            ///< Internal reset caused by POF, VSYS low
    NPM1300_ERRLOG_RSTCAUSE_SWRESET_MASK            = ERRLOG_RSTCAUSE_SWRESET_Msk,            ///< Internal reset caused by soft reset
} npm1300_errlog_rstcause_mask_t;

/**
 * @brief Error log for slowDomain.
 * Can be cleared with:
 *      TASK_CLR_ERRLOG  <- info from documentation v0.5 point 7.9.1.8, should be verified
 *      or
 *      TASKCLEARCHGERR <- info from documentation v0.5 point 6.2.14, should be verified
 */
typedef enum
{
    NPM1300_ERRLOG_CHARGER_NTC_SENS_ERR_MASK    = ERRLOG_CHARGERERRREASON_NTCSENSORERR_Msk,   ///< NTC sensor error
    NPM1300_ERRLOG_CHARGER_VBAT_SENS_ERR_MASK   = ERRLOG_CHARGERERRREASON_VBATSENSORERR_Msk,  ///< VBAT Sensor Error
    NPM1300_ERRLOG_CHARGER_VBATLOW_MASK         = ERRLOG_CHARGERERRREASON_VBATLOW_Msk,        ///< VBAT Low Error
    NPM1300_ERRLOG_CHARGER_VTRICKLE_MASK        = ERRLOG_CHARGERERRREASON_VTRICKLE_Msk,       ///< Vtrickle Error
    NPM1300_ERRLOG_CHARGER_MEAS_TIMEOUT_MASK    = ERRLOG_CHARGERERRREASON_MEASTIMEOUT_Msk,    ///< Measurement Timeout Error
    NPM1300_ERRLOG_CHARGER_CHARGE_TIMEOUT_MASK  = ERRLOG_CHARGERERRREASON_CHARGETIMEOUT_Msk,  ///< Charge Timeout Error
    NPM1300_ERRLOG_CHARGER_TRICKLE_TIMEOUT_MASK = ERRLOG_CHARGERERRREASON_TRICKLETIMEOUT_Msk, ///< Trickle Timeout Error
} npm1300_errlog_charger_mask_t;

/**
 * @brief Bcharger Fsm sensor error.
 * Can be cleared with:
 *      TASK_CLR_ERRLOG  <- info from documentation v0.5 point 7.9.1.8, should be verified
 *      or
 *      TASKCLEARCHGERR <- info from documentation v0.5 point 6.2.14, should be verified
 */
typedef enum
{
    NPM1300_ERRLOG_CHARGESENS_NTC_COLD_MASK = ERRLOG_CHARGERERRSENSOR_SENSORNTCCOLD_Msk,  ///< Ntc Cold sensor value during error
    NPM1300_ERRLOG_CHARGESENS_NTC_COOL_MASK = ERRLOG_CHARGERERRSENSOR_SENSORNTCCOOL_Msk,  ///< Ntc Cool sensor value during error
    NPM1300_ERRLOG_CHARGESENS_NTC_WARM_MASK = ERRLOG_CHARGERERRSENSOR_SENSORNTCWARM_Msk,  ///< Ntc Warm sensor value during error
    NPM1300_ERRLOG_CHARGESENS_NTC_HOT_MASK  = ERRLOG_CHARGERERRSENSOR_SENSORNTCHOT_Msk,   ///< Ntc Hot sensor value during error
    NPM1300_ERRLOG_CHARGESENS_VTERM_MASK    = ERRLOG_CHARGERERRSENSOR_SENSORVTERM_Msk,    ///< Vterm sensor value during error
    NPM1300_ERRLOG_CHARGESENS_RECHARGE_MASK = ERRLOG_CHARGERERRSENSOR_SENSORRECHARGE_Msk, ///< Recharge sensor value during error
    NPM1300_ERRLOG_CHARGESENS_VTRICKLE_MASK = ERRLOG_CHARGERERRSENSOR_SENSORVTRICKLE_Msk, ///< Vtrickle sensor value during error
    NPM1300_ERRLOG_CHARGESENS_VBATLOW_MASK  = ERRLOG_CHARGERERRSENSOR_SENSORVBATLOW_Msk,  ///< VbatLow sensor value during error
} npm1300_errlog_chargesens_mask_t;

/** @brief Possible Events groups */
typedef enum
{
    NPM1300_EVENT_GROUP_ADC             = 0, ///< ADC Events
    NPM1300_EVENT_GROUP_BAT_CHAR_TEMP   = 1, ///< Battery Charger Temperature Events
    NPM1300_EVENT_GROUP_BAT_CHAR_STATUS = 2, ///< Battery Charger Status Events
    NPM1300_EVENT_GROUP_BAT_CHAR_BAT    = 3, ///< Battery Charger Battery Events
    NPM1300_EVENT_GROUP_SHIPHOLD        = 4, ///< ShipHold pin Events
    NPM1300_EVENT_GROUP_VBUSIN_VOLTAGE  = 5, ///< VBUSIN Voltage Detection Events
    NPM1300_EVENT_GROUP_VBUSIN_THERMAL  = 6, ///< VBUSIN Thermal and USB C Events
    NPM1300_EVENT_GROUP_USB_B           = 7, ///< USB-B Event
    NPM1300_EVENT_GROUP_COUNT,               ///< Events groups count
} npm1300_event_group_t;

/** @brief VBUSIN Voltage Detection Events fields */
typedef enum
{
    NPM1300_EVENT_GROUP_VBUSIN_DETECTED_MASK           =
        MAIN_EVENTSVBUSIN0CLR_EVENTVBUSDETECTED_Msk,                                                 ///< Event VBUS input detected
    NPM1300_EVENT_GROUP_VBUSIN_REMOVED_MASK            = MAIN_EVENTSVBUSIN0CLR_EVENTVBUSREMOVED_Msk, ///< Event VBUS input removed
    NPM1300_EVENT_GROUP_VBUSIN_OVRVOLT_DETECTED_MASK   =
        MAIN_EVENTSVBUSIN0CLR_EVENTVBUSOVRVOLTDETECTED_Msk,                                          ///< Event VBUS Over Voltage Detected
    NPM1300_EVENT_GROUP_VBUSIN_OVRVOLT_REMOVED_MASK    =
        MAIN_EVENTSVBUSIN0CLR_EVENTVBUSOVRVOLTREMOVED_Msk,                                           ///< Event VBUS Over Removed
    NPM1300_EVENT_GROUP_VBUSIN_UNDERVOLT_DETECTED_MASK =
        MAIN_EVENTSVBUSIN0CLR_EVENTUNDERVOLTDETECTED_Msk,                                            ///< Event VBUS Under Voltage Detected.
    NPM1300_EVENT_GROUP_VBUSIN_UNDERVOLT_REMOVED_MASK  =
        MAIN_EVENTSVBUSIN0CLR_EVENTUNDERVOLTREMOVED_Msk,                                             ///< Event VBUS Under Removed
} npm1300_event_group_vbusin_mask_t;

/** @brief Battery Charger Battery Events fields */
typedef enum
{
    NPM1300_EVENT_GROUP_BATTERY_DETECTED_MASK = MAIN_EVENTSBCHARGER2SET_EVENTBATDETECTED_Msk, ///< Event Battery Detected
    NPM1300_EVENT_GROUP_BATTERY_REMOVED_MASK  = MAIN_EVENTSBCHARGER2SET_EVENTBATLOST_Msk,     ///< Event Battery Lost
    NPM1300_EVENT_GROUP_BATTERY_RECHARGE_MASK = MAIN_EVENTSBCHARGER2SET_EVENTBATRECHARGE_Msk, ///< Event Battery re-charge needed
} npm1300_event_group_battery_mask_t;

/** @brief Battery Charger Status Events fields */
typedef enum
{
    NPM1300_EVENT_GROUP_CHARGER_SUPPLEMENT_MASK = MAIN_EVENTSBCHARGER1SET_EVENTSUPPLEMENT_Msk,   ///< Event supplement mode activated
    NPM1300_EVENT_GROUP_CHARGER_TRICKLE_MASK    = MAIN_EVENTSBCHARGER1SET_EVENTCHGTRICKLE_Msk,   ///< Event Trickle Charge started
    NPM1300_EVENT_GROUP_CHARGER_CC_MASK         = MAIN_EVENTSBCHARGER1SET_EVENTCHGCC_Msk,        ///< Event Constant Current charging started
    NPM1300_EVENT_GROUP_CHARGER_CV_MASK         = MAIN_EVENTSBCHARGER1SET_EVENTCHGCV_Msk,        ///< Event Constant Voltage charging started
    NPM1300_EVENT_GROUP_CHARGER_COMPLETED_MASK  = MAIN_EVENTSBCHARGER1SET_EVENTCHGCOMPLETED_Msk, ///< Event charging completed (Battery Full)
    NPM1300_EVENT_GROUP_CHARGER_ERROR_MASK      = MAIN_EVENTSBCHARGER1SET_EVENTCHGERROR_Msk,     ///< Event charging error
} npm1300_event_group_charger_mask_t;

/** @brief ADC Events fields */
typedef enum
{
    NPM1300_EVENT_GROUP_ADC_BAT_READY_MASK      = MAIN_EVENTSADCSET_EVENTADCVBATRDY_Msk,    ///< VBAT measurement finished
    NPM1300_EVENT_GROUP_ADC_NTC_READY_MASK      = MAIN_EVENTSADCSET_EVENTADCNTCRDY_Msk,     ///< Battery NTC measurement finished
    NPM1300_EVENT_GROUP_ADC_DIE_TEMP_READY_MASK = MAIN_EVENTSADCSET_EVENTADCTEMPRDY_Msk,    ///< Internal Die Temperature measurement finished
    NPM1300_EVENT_GROUP_ADC_VSYS_READY_MASK     = MAIN_EVENTSADCSET_EVENTADCVSYSRDY_Msk,    ///< VSYS Voltage measurement measurement finished
    NPM1300_EVENT_GROUP_ADC_VSET1_READY_MASK    = MAIN_EVENTSADCSET_EVENTADCVSET1RDY_Msk,   ///< DCDC VSET1 pin measurement finished
    NPM1300_EVENT_GROUP_ADC_VSET2_READY_MASK    = MAIN_EVENTSADCSET_EVENTADCVSET2RDY_Msk,   ///< DCDC VSET2 pin measurement finished
    NPM1300_EVENT_GROUP_ADC_IBAT_READY_MASK     = MAIN_EVENTSADCSET_EVENTADCIBATRDY_Msk,    ///< IBAT measurement finished
    NPM1300_EVENT_GROUP_ADC_VBUS_READY_MASK     = MAIN_EVENTSADCSET_EVENTADCVBUS7V0RDY_Msk, ///< VBUS (7Volt range) measurement finished
} npm1300_event_group_adc_mask_t;

/** @brief Request ADC measurement */
typedef enum
{
    NPM1300_ADC_REQUEST_BAT      = 0, ///< VBAT measurement
    NPM1300_ADC_REQUEST_NTC      = 1, ///< Battery NTC measurement
    NPM1300_ADC_REQUEST_DIE_TEMP = 2, ///< Internal Die Temperature measurement
    NPM1300_ADC_REQUEST_VSYS     = 3, ///< VSYS Voltage measurement measurement
    NPM1300_ADC_REQUEST_VSET1    = 4, ///< DCDC VSET1 pin measurement
    NPM1300_ADC_REQUEST_VSET2    = 5, ///< DCDC VSET2 pin measurement
    NPM1300_ADC_REQUEST_IBAT     = 6, ///< IBAT measurement
    NPM1300_ADC_REQUEST_VBUS     = 7, ///< VBUS (7Volt range) measurement
    NPM1300_ADC_REQUEST_COUNT,        ///< ADC requests maximum count
} npm1300_adc_request_t;

/** @brief Selected LED type */
typedef enum
{
    NPM1300_LED_INSTANCE_0 = 0, ///< Select LED_0
    NPM1300_LED_INSTANCE_1 = 1, ///< Select LED_1
    NPM1300_LED_INSTANCE_2 = 2, ///< Select LED_2
} nmp1300_led_instance_t;

/** @brief LED modes */
typedef enum
{
    NPM1300_LED_MODE_ERROR    = LEDDRV_LEDDRV1MODESEL_LEDDRV1MODESEL_ERROR,    ///< Error condition from Charger
    NPM1300_LED_MODE_CHARGING = LEDDRV_LEDDRV1MODESEL_LEDDRV1MODESEL_CHARGING, ///< Charging indicator (On during charging)
    NPM1300_LED_MODE_HOST     = LEDDRV_LEDDRV1MODESEL_LEDDRV1MODESEL_HOST,     ///< Driven from register LEDDRV_x_SET/CLR
    NPM1300_LED_MODE_NOTUSED  = LEDDRV_LEDDRV1MODESEL_LEDDRV1MODESEL_NOTUSED,  ///< Not used
} npm1300_led_mode_t;

/** @brief Config for GPIO mode selection */
typedef enum
{
    NPM1300_GPIO_MODE_INPUT              = GPIOS_GPIOMODE0_GPIOMODE_GPIINPUT,     ///< GPI Input
    NPM1300_GPIO_MODE_INPUT_OVERRIDE_1   = GPIOS_GPIOMODE0_GPIOMODE_GPILOGIC1,    ///< GPI Logic1
    NPM1300_GPIO_MODE_INPUT_OVERRIDE_0   = GPIOS_GPIOMODE0_GPIOMODE_GPILOGIC0,    ///< GPI Logic0
    NPM1300_GPIO_MODE_INPUT_RISING_EDGE  = GPIOS_GPIOMODE0_GPIOMODE_GPIEVENTRISE, ///< GPI Rising Edge Event
    NPM1300_GPIO_MODE_INPUT_FALLING_EDGE = GPIOS_GPIOMODE0_GPIOMODE_GPIEVENTFALL, ///< GPI Falling Edge Event
    NPM1300_GPIO_MODE_OUTPUT_IRQ         = GPIOS_GPIOMODE0_GPIOMODE_GPOIRQ,       ///< GPO Interrupt
    NPM1300_GPIO_MODE_OUTPUT_RESET       = GPIOS_GPIOMODE0_GPIOMODE_GPORESET,     ///< GPO Reset
    NPM1300_GPIO_MODE_OUTPUT_PLW         = GPIOS_GPIOMODE0_GPIOMODE_GPOPLW,       ///< GPO PwrLossWarn
    NPM1300_GPIO_MODE_OUTPUT_OVERRIDE_1  = GPIOS_GPIOMODE0_GPIOMODE_GPOLOGIC1,    ///< GPO Logic1
    NPM1300_GPIO_MODE_OUTPUT_OVERRIDE_0  = GPIOS_GPIOMODE0_GPIOMODE_GPOLOGIC0,    ///< GPO Logic0
} nmp1300_gpio_mode_t;

/** @brief Select GPIO type */
typedef enum
{
    NPM1300_GPIO_INSTANCE_0 = 0, ///< GPIO instance 0
    NPM1300_GPIO_INSTANCE_1 = 1, ///< GPIO instance 1
    NPM1300_GPIO_INSTANCE_2 = 2, ///< GPIO instance 2
    NPM1300_GPIO_INSTANCE_3 = 3, ///< GPIO instance 3
    NPM1300_GPIO_INSTANCE_4 = 4, ///< GPIO instance 4
} npm1300_gpio_instance_t;

/** @brief Config for GPIO drive strength */
typedef enum
{
    NPM1300_GPIO_DRIVE_1_MA = GPIOS_GPIODRIVE1_GPIODRIVE_1MA, ///< 1 mA
    NPM1300_GPIO_DRIVE_6_MA = GPIOS_GPIODRIVE1_GPIODRIVE_6MA, ///< 6 mA
} npm1300_gpio_drive_t;

/** @brief Configuration structure for GPIO */
typedef struct
{
    npm1300_gpio_instance_t type;              ///< Select GPIO type
    nmp1300_gpio_mode_t     mode;              ///< Config for GPIO mode selection
    npm1300_gpio_drive_t    drive;             ///< GPIO Drive strength
    bool                    pull_up_enable;    ///< Config for GPIO pull-up enable
    bool                    pull_down_enable;  ///< Config for GPIO pull-down enable
    bool                    open_drain_enable; ///< Config for GPIO open drain
    bool                    debounce_enable;   ///< Config for GPIO debounce
} npm1300_gpio_conf_t;

/** @brief All possible callback types to be registered */
typedef enum
{
    NPM1300_CALLBACK_TYPE_EVENT_ADC                = 0,  ///< Callback run when EVENTSADCSET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_TEMP      = 1,  ///< Callback run when EVENTSBCHARGER0SET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS    = 2,  ///< Callback run when EVENTSBCHARGER1SET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT       = 3,  ///< Callback run when EVENTSBCHARGER2SET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_SHIPHOLD           = 4,  ///< Callback run when EVENTSSHPHLDSET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE     = 5,  ///< Callback run when EVENTSVBUSIN0SET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_THERMAL_USB = 6,  ///< Callback run when EVENTSVBUSIN1SET register not empty
    NPM1300_CALLBACK_TYPE_EVENT_USB_B              = 7,  ///< Callback run when EVENTSUSBBSET register not empty
    NPM1300_CALLBACK_TYPE_RSTCAUSE                 = 8,  ///< Callback run when RSTCAUSE register is not empty
    NPM1300_CALLBACK_TYPE_CHARGERERRREASON         = 9,  ///< Callback run when CHARGERERRREASON register is not empty
    NPM1300_CALLBACK_TYPE_CHARGERERRSENSOR         = 10, ///< Callback run when CHARGERERRREASON register is not empty
    NPM1300_CALLBACK_TYPE_COUNT,                         ///< Callbacks count
} npm1300_callback_type_t;

/** @brief Ntc Comparator Status */
typedef enum
{
    NPM1300_NTC_STATUS_COLD_MASK = BCHARGER_NTCSTATUS_NTCCOLD_Msk, ///< Ntc Cold
    NPM1300_NTC_STATUS_COOL_MASK = BCHARGER_NTCSTATUS_NTCCOOL_Msk, ///< Ntc Cool
    NPM1300_NTC_STATUS_WARM_MASK = BCHARGER_NTCSTATUS_NTCWARM_Msk, ///< Ntc Warm
    NPM1300_NTC_STATUS_HOT_MASK  = BCHARGER_NTCSTATUS_NTCHOT_Msk,  ///< Ntc Hot
} npm1300_ntc_status_mask_t;

/** @brief Charger modules to be enabled and disabled */
typedef enum
{
    NPM1300_CHARGER_MODULE_CHARGER_MASK    = (1U << 0U), ///< Battery Charger Module
    /* Default: charger not set */
    NPM1300_CHARGER_MODULE_FULL_COOL_MASK  = (1U << 1U), ///< Battery Charger Module Full Charge in Cool temp
    /* When:    not set:     50% charge current value of BCHGISETMSB and BCHGISETLSB registers,
     *          set:         100% charge current value of BCHGISETMSB and BCHGISETLSB registers
     * Default: not set */
    NPM1300_CHARGER_MODULE_RECHARGE_MASK   = (1U << 2U), ///< Battery Charger Module Recharge
    /* When:    set:         The charger waits until the battery voltage is below VRECHARGE before starting a new charging cycle
     *          not set:     Optional automatic re-charge is disabled
     * Default: set */
    NPM1300_CHARGER_MODULE_NTC_LIMITS_MASK = (1U << 3U), ///< Battery Charger Module NTC temperature limits
    /* When:    set:         Battery Charger does not ignore NTC temperature limits
     *          not set:     Battery Charger ignore NTC temperature limits
     * Default: set */
} npm1300_charger_module_mask_t;

/** @brief Battery Charger termination voltage Normal and Warm*/
typedef enum
{
    NPM1300_CHARGER_VOLTAGE_3V50    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_3V50, ///< 3.50V
    NPM1300_CHARGER_VOLTAGE_3V55    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_3V55, ///< 3.55V
    NPM1300_CHARGER_VOLTAGE_3V60    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_3V60, ///< 3.60V
    NPM1300_CHARGER_VOLTAGE_3V65    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_3V65, ///< 3.65V
    NPM1300_CHARGER_VOLTAGE_4V00    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V00, ///< 4.00V
    NPM1300_CHARGER_VOLTAGE_4V05    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V05, ///< 4.05V
    NPM1300_CHARGER_VOLTAGE_4V10    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V10, ///< 4.10V
    NPM1300_CHARGER_VOLTAGE_4V15    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V15, ///< 4.15V
    NPM1300_CHARGER_VOLTAGE_4V20    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V20, ///< 4.20V
    NPM1300_CHARGER_VOLTAGE_4V25    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V25, ///< 4.25V
    NPM1300_CHARGER_VOLTAGE_4V30    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V30, ///< 4.30V
    NPM1300_CHARGER_VOLTAGE_4V35    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V35, ///< 4.35V
    NPM1300_CHARGER_VOLTAGE_4V40    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V40, ///< 4.40V
    NPM1300_CHARGER_VOLTAGE_4V45    = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_4V45, ///< 4.45V
    NPM1300_CHARGER_VOLTAGE_DEFAULT = BCHARGER_BCHGVTERMR_BCHGVTERMREDUCED_3V60, ///< Default 3.60V
} npm1300_charger_voltage_t;

/** @brief Battery Charger Status */
typedef enum
{
    NPM1300_CHARGER_STATUS_BATTERY_DETECTED_MASK  =
        BCHARGER_BCHGCHARGESTATUS_BATTERYDETECTED_Msk,                                              ///< Battery is connected
    NPM1300_CHARGER_STATUS_COMPLETED_MASK         = BCHARGER_BCHGCHARGESTATUS_COMPLETED_Msk,        ///< Charging completed (Battery Full)
    NPM1300_CHARGER_STATUS_TRICKLE_CHARGE_MASK    = BCHARGER_BCHGCHARGESTATUS_TRICKLECHARGE_Msk,    ///< Trickle charge
    NPM1300_CHARGER_STATUS_CONSTANT_CURRENT_MASK  =
        BCHARGER_BCHGCHARGESTATUS_CONSTANTCURRENT_Msk,                                              ///< Constant Current charging
    NPM1300_CHARGER_STATUS_CONSTANT_VOLTAGE_MASK  =
        BCHARGER_BCHGCHARGESTATUS_CONSTANTVOLTAGE_Msk,                                              ///< Constant Voltage charging
    NPM1300_CHARGER_STATUS_RECHARGE_MASK          = BCHARGER_BCHGCHARGESTATUS_RECHARGE_Msk,         ///< Battery re-charge is needed
    NPM1300_CHARGER_STATUS_DIE_TEMP_HIGH_MASK     =
        BCHARGER_BCHGCHARGESTATUS_DIETEMPHIGHCHGPAUSED_Msk,                                         ///< Charging stopped due Die Temp high
    NPM1300_CHARGER_STATUS_SUPPLEMENT_ACTIVE_MASK =
        BCHARGER_BCHGCHARGESTATUS_SUPPLEMENTACTIVE_Msk,                                             ///< Supplement Mode Active
} npm1300_charger_status_mask_t;

/** @brief Battery Charger Trickle Level Select */
typedef enum
{
    NPM1300_CHARGER_TRICKLE_2V9     = BCHARGER_BCHGVTRICKLESEL_BCHGVTRICKLESEL_2V9, ///< Trickle voltage level 2.9 V
    NPM1300_CHARGER_TRICKLE_2V5     = BCHARGER_BCHGVTRICKLESEL_BCHGVTRICKLESEL_2V5, ///< Trickle voltage level 2.5 V
    NPM1300_CHARGER_TRICKLE_DEFAULT = BCHARGER_BCHGVTRICKLESEL_BCHGVTRICKLESEL_2V9, ///< Default trickle voltage level 2.9 V
} npm1300_charger_trickle_t;

/** @brief Battery Charger ITERM select */
typedef enum
{
    NPM1300_CHARGER_ITERM_10      = BCHARGER_BCHGITERMSEL_BCHGITERMSEL_SEL10, ///< ITERM current set to 10 percent of charging current
    NPM1300_CHARGER_ITERM_20      = BCHARGER_BCHGITERMSEL_BCHGITERMSEL_SEL20, ///< ITERM current set to 20 percent of charging current
    NPM1300_CHARGER_ITERM_DEFAULT = BCHARGER_BCHGITERMSEL_BCHGITERMSEL_SEL10, ///< Default ITERM current set to 10 percent of charging current
} npm1300_charger_iterm_t;

/**
 * @brief Battery NTC type.
 * Should be checked in battery documentation.
 */
typedef enum
{
    NPM1300_BATTERY_NTC_TYPE_HI_Z  = ADC_ADCNTCRSEL_ADCNTCRSEL_Hi_Z,  ///< No resistor
    NPM1300_BATTERY_NTC_TYPE_10_K  = ADC_ADCNTCRSEL_ADCNTCRSEL_10K,   ///< NTC10K
    NPM1300_BATTERY_NTC_TYPE_47_K  = ADC_ADCNTCRSEL_ADCNTCRSEL_47K,   ///< NTC47K
    NPM1300_BATTERY_NTC_TYPE_100_K = ADC_ADCNTCRSEL_ADCNTCRSEL_100K,  ///< NTC100K
} npm1300_battery_ntc_type_t;

/** @brief Power Failure Warning polarity */
typedef enum
{
    NTC1300_POF_POLARITY_HI = POF_POFCONFIG_POFWARNPOLARITY_HIACTIVE, ///< HiActive
    NTC1300_POF_POLARITY_LO = POF_POFCONFIG_POFWARNPOLARITY_LOACTIVE, ///< LoActive
} ntc1300_pof_polarity_t;

/** @brief Vsys Comparator Threshold */
typedef enum
{
    NTC1300_POF_THRESHOLD_2V8 = POF_POFCONFIG_POFVSYSTHRESHSEL_2V8, ///< 2.8V
    NTC1300_POF_THRESHOLD_2V6 = POF_POFCONFIG_POFVSYSTHRESHSEL_2V6, ///< 2.6V
    NTC1300_POF_THRESHOLD_2V7 = POF_POFCONFIG_POFVSYSTHRESHSEL_2V7, ///< 2.7V
    NTC1300_POF_THRESHOLD_2V9 = POF_POFCONFIG_POFVSYSTHRESHSEL_2V9, ///< 2.9V
    NTC1300_POF_THRESHOLD_3V0 = POF_POFCONFIG_POFVSYSTHRESHSEL_3V0, ///< 3.0V
    NTC1300_POF_THRESHOLD_3V1 = POF_POFCONFIG_POFVSYSTHRESHSEL_3V1, ///< 3.1V
    NTC1300_POF_THRESHOLD_3V2 = POF_POFCONFIG_POFVSYSTHRESHSEL_3V2, ///< 3.2V
    NTC1300_POF_THRESHOLD_3V3 = POF_POFCONFIG_POFVSYSTHRESHSEL_3V3, ///< 3.3V
    NTC1300_POF_THRESHOLD_3V4 = POF_POFCONFIG_POFVSYSTHRESHSEL_3V4, ///< 3.4V
    NTC1300_POF_THRESHOLD_3V5 = POF_POFCONFIG_POFVSYSTHRESHSEL_3V5, ///< 3.5V
} ntc1300_pof_threshold_t;

typedef struct npm1300_instance npm1300_instance_t;

typedef void (*npm1300_callback_t)(npm1300_instance_t *    pm,
                                   npm1300_callback_type_t type,
                                   uint32_t                arg);

/** @brief nPM1300 struct definition */
typedef struct npm1300_instance
{
    void *             user_handler;                                                                ///< User context handler used in @ref write_registers and @ref read_registers functions
    int (*write_registers)(void * user_handler, uint16_t addr, uint8_t * data, uint32_t num_bytes); ///< Function used by nPM device instance to write data to nPM device
    int (*read_registers)(void * user_handler, uint16_t addr, uint8_t * data, uint32_t num_bytes);  ///< Function used by nPM device instance to read data from nPM device

    npm1300_callback_t generic_cb;                                                                  ///< Function used when @ref registered_cb for @ref npm1300_callback_type_t is not set
    npm1300_callback_t registered_cb[(uint32_t)NPM1300_CALLBACK_TYPE_COUNT];                        ///< Table of registered callbacks

    uint8_t            event_group_enable_mask[NPM1300_EVENT_GROUP_COUNT];                          ///< Enabled events in specified events group, see npm1300_event_group_xxx_t for selected event

    npm1300_led_mode_t led_mode[3];                                                                 ///< Table of LEDs modes
    bool               interrupt;                                                                   ///< Flag set in interrupt to inform npm1300_proc about incoming GPIO interrupt
    bool               charger_error;                                                               ///< Flag set when charger error event occurs
    uint32_t           ntc_resistance;                                                              ///< NTC resistor resistance in Ohms
    uint8_t            adc_config;                                                                  ///< Copy of ADCCONFIG register to avoid reading via I2C bus
} npm1300_instance_t;

/**
 * @brief Get the callback name
 *
 * @param [in] type The specfied callback type
 * @return const char* Pointer to the callback name
 */
const char * npm1300_callback_to_str(npm1300_callback_type_t type);

/**
 * @brief Get the name of callback bit
 *
 * @param [in] type The specfied callback type
 * @param [in] bit The bit in callback register
 * @return const char* Pointer to the bit name
 */
const char * npm1300_callback_bit_to_str(npm1300_callback_type_t type, uint8_t bit);

/**
 * @brief Get the name of battery charger status bit
 *
 * @param [in] bit The bit in battery charger status register
 * @return const char* Pointer to the bit name
 */
const char * npm1300_charger_status_bit_to_str(uint8_t bit);
/**
 * @brief Stop watchdog boot timer
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully stopped boot timer
 * @return false Communication error
 */
bool npm1300_stop_boot_timer(npm1300_instance_t * pm);

/**
 * @brief Check if nPM device is connected
 *
 * @param [in] pm The instance of nPM device
 * @return true The nPM device is connected
 * @return false The nPM device is not connected
 */
bool npm1300_connection_check(npm1300_instance_t * pm);

/**
 * @brief Check possible errors in nPM device and run callbacks
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully checked all errors
 * @return false Communication error
 */
bool npm1300_check_errors(npm1300_instance_t * pm);

/**
 * @brief Set the mode of LED instance
 *
 * @param [in] pm The instance of nPM device
 * @param [in] led The instance of the LED driver
 * @param [in] mode The mode of the LED
 * @return true Successfully set the LED mode
 * @return false Communication error
 */
bool npm1300_led_mode_set(npm1300_instance_t *   pm,
                          nmp1300_led_instance_t led,
                          npm1300_led_mode_t     mode);

/**
 * @brief Set the state of LED instance
 *
 * @note This function can be called only after @ref npm1300_led_mode_set with NPM1300_LED_MODE_HOST
 *
 * @param [in] pm The instance of nPM device
 * @param [in] led The instance of LED driver
 * @param [in] state The state of LED
 * @return true Successfully set the LED state
 * @return false Communication error or specified LED not in host mode
 */
bool npm1300_led_state_set(npm1300_instance_t * pm, nmp1300_led_instance_t led, bool state);

/**
 * @brief Set GPIO mode
 *
 * @param [in] pm The instance of nPM device
 * @param [in] gpio The instance of GPIO driver
 * @param [in] mode THe mode of GPIO
 * @return true Successfully set the GPIO mode
 * @return false Communication error
 */
bool npm1300_gpio_mode_set(npm1300_instance_t *    pm,
                           npm1300_gpio_instance_t gpio,
                           nmp1300_gpio_mode_t     mode);

/**
 * @brief Register callback handler to specified callback type event/error
 *        If no callback is registered, pm->generic_cb is called
 *
 * @param [in] pm The instance of nPM device
 * @param [in] cb The function callback pointer to be registered
 * @param [in] type The type of registered function callback pointer
 */
void npm1300_register_cb(npm1300_instance_t *    pm,
                         npm1300_callback_t      cb,
                         npm1300_callback_type_t type);

/**
 * @brief Initialize the instance of nPM device with defaults values
 *        Should be called in setup function to avoid runtime errors
 *
 * @param [in] pm The instance of nPM device
 */
void npm1300_init(npm1300_instance_t * pm);

/**
 * @brief Enable event interrupt
 *
 * @param [in] pm The instance of nPM device
 * @param [in] event Specified event group type
 * @param [in] flags_mask Specified bits in event group, see npm1300_event_group_xxx_t for selected event
 * @return true Interrupt enabled successfully
 * @return false Interrupt not enabled
 */
bool npm1300_event_interrupt_enable(npm1300_instance_t *  pm,
                                    npm1300_event_group_t event,
                                    uint8_t               flags_mask);

/**
 * @brief Clear event fields in specified event group
 * @param [in] pm The instance of nPM device
 * @param [in] event Specified event group type
 * @param [in] flags_mask Specified bit flags for event group type
 * @return true Event cleared successfully
 * @return false Event not cleared
 */
bool npm1300_event_clear(npm1300_instance_t * pm, npm1300_event_group_t event, uint8_t flags_mask);

/**
 * @brief Get events field from specified event group
 *
 * @param [in] pm The instance of nPM device
 * @param [in] event Specified event group type
 * @param [out] p_flags_mask The pointer to the flags variable, npm1300_event_group_xxx_t for selected event group
 * @return true Successfully read the status event register
 * @return false Communication error
 */
bool npm1300_event_get(npm1300_instance_t *  pm,
                       npm1300_event_group_t event,
                       uint8_t *             p_flags_mask);

/**
 * @brief Should be run in loop task to handle all interrupts from device
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully communicate with nPM device
 * @return false Communication error
 */
bool npm1300_proc(npm1300_instance_t * pm);

/**
 * @brief Should be called when:
 *    - ISR function from GPIO
 *    - GPIO is in HIGH state
 *    to register event from device
 *
 * @param [in] pm The instance of nPM device
 */
void npm1300_interrupt(npm1300_instance_t * pm);

/**
 * @brief Enable specified charger modules
 *
 * @param [in] pm The instance of nPM device
 * @param [in] flags_mask Selected module to be enabled
 * @return true Successfully enabled all selected modules
 * @return false Communication error
 */
bool npm1300_charger_module_enable(npm1300_instance_t *          pm,
                                   npm1300_charger_module_mask_t flags_mask);

/**
 * @brief Disable specified charger modules
 *
 * @param [in] pm The instance of nPM device
 * @param [in] flags_mask Selected module to be disabled
 * @return true Successfully disabled all selected modules
 * @return false Communication error
 */
bool npm1300_charger_module_disable(npm1300_instance_t *          pm,
                                    npm1300_charger_module_mask_t flags_mask);

/**
 * @brief Check which charger's modules are enabled
 *
 * @param [in] pm The instance of nPM device
 * @param [out] p_modules_mask Pointer to modules status variable
 * @return true Successfully read modules status
 * @return false Communication error
 */
bool npm1300_charger_module_get(npm1300_instance_t *            pm,
                                npm1300_charger_module_mask_t * p_modules_mask);

/**
 * @brief Set charger current of nPM device. Default value after reset is 32 mA
 *
 * @note CHARGER has to be disabled before changing the current setting
 *       The setting takes effect when charging is (re-)enabled.
 *
 * @param [in] pm The instance of nPM device
 * @param [in] current in mA from range of 32 mA to 800 mA, if 0 function returns false,
 *              any value out of range is trimmed
 * @return true Successfully set the charger current
 * @return false Communication error or current value is 0
 */
bool npm1300_charger_charging_current_set(npm1300_instance_t * pm, uint16_t current);

/**
 * @brief Set maximum discharging current of nPM device. Default value after reset is 1000 mA
 *
 * @param [in] pm The instance of nPM device
 * @param [in] current in mA from the range of 270 mA to 1340 mA, if 0 function returns false,
 *              any value out of range is trimmed
 * @return true Successfully set the maximum discharging current
 * @return false Communication error or current value is 0
 */
bool npm1300_charger_discharging_current_set(npm1300_instance_t * pm, uint16_t current);

/**
 * @brief Set the nominal battery voltage
 *        Default value after reset is code 0x02 -> 3.6V (NPM1300_CHARGER_VOLTAGE_DEFAULT)
 *
 * @param [in] pm The instance of nPM device
 * @param [in] voltage from ranges 3.5 V...3.65 V or 4.0 V...4.45 V in 50 mV steps
 *                @ref npm1300_charger_voltage_t
 * @return true Successfully set the battery voltage
 * @return false Communication error or voltage out of range
 */
bool npm1300_charger_termination_volatge_normal_set(npm1300_instance_t *      pm,
                                                    npm1300_charger_voltage_t voltage);

/**
 * @brief Set the battery voltage used in the Warm temperature
 *        Default value after reset is code 0x02 -> 3.6V (NPM1300_CHARGER_VOLTAGE_DEFAULT)
 *
 * @param [in] pm The instance of nPM device
 * @param [in] voltage from ranges 3.5 V...3.65 V or 4.0 V...4.45 V in 50 mV steps
 *                @ref npm1300_charger_voltage_t
 * @return true Successfully set the battery voltage
 * @return false Communication error or voltage out of range
 */
bool npm1300_charger_termination_volatge_warm_set(npm1300_instance_t *      pm,
                                                  npm1300_charger_voltage_t voltage);

/**
 * @brief Set the trickle voltage
 *        Trickle charging is performed when VBAT < V_TRICKLE_FAST (default 2.9 V)
 *
 * @param [in] pm The instance of nPM device
 * @param [in] val trickle voltage value, see npm1300_charger_trickle_t (2.9 V or 2.5 V)
 * @return true Successfully set the trickle value
 * @return false Communication error
 */
bool npm1300_charger_trickle_level_set(npm1300_instance_t * pm, npm1300_charger_trickle_t val);

/**
 * @brief Set the termination current ITERM
 *          Trickle charging current I_TRICKLE is 10% or 20% of I_CHG
 *
 * @param [in] pm The instance of nPM device
 * @param [in] iterm value: 10 % or 20 %
 * @return true Successfully set the iterm value
 * @return false Communication error
 */
bool npm1300_charger_iterm_level_set(npm1300_instance_t * pm, npm1300_charger_iterm_t iterm);

/**
 * @brief Set the NTC cold resistance threshold
 *
 * @note resistance value should be read from NTC characteristic for selected temperature
 *
 * @param [in] pm The instance of nPM device
 * @param [in] resistance in Ohms, value should be greater then 0
 * @return true Successfully set the threshold
 * @return false Communication error or resistance is 0 value
 */
bool npm1300_charger_cold_resistance_set(npm1300_instance_t * pm, uint32_t resistance);

/**
 * @brief Set the NTC cool resistance threshold
 *
 * @note resistance value should be read from NTC characteristic for selected temperature
 *
 * @param [in] pm The instance of nPM device
 * @param [in] resistance in Ohms, value should be greater then 0
 * @return true Successfully set the threshold
 * @return false Communication error or resistance is 0 value
 */
bool npm1300_charger_cool_resistance_set(npm1300_instance_t * pm, uint32_t resistance);

/**
 * @brief Set the NTC warm resistance threshold
 *
 * @note resistance value should be read from NTC characteristic for selected temperature
 *
 * @param [in] pm The instance of nPM device
 * @param [in] resistance in Ohms, value should be greater then 0
 * @return true Successfully set the threshold
 * @return false Communication error or resistance is 0 value
 */
bool npm1300_charger_warm_resistance_set(npm1300_instance_t * pm, uint32_t resistance);

/**
 * @brief Set the NTC hot resistance threshold
 *
 * @note resistance value should be read from NTC characteristic for selected temperature
 *
 * @param [in] pm The instance of nPM device
 * @param [in] resistance in Ohms, value should be greater then 0
 * @return true Successfully set the threshold
 * @return false Communication error or resistance is 0 value
 */
bool npm1300_charger_hot_resistance_set(npm1300_instance_t * pm, uint32_t resistance);

/**
 * @brief Read the charger status register
 *
 * @param [in] pm The instance of nPM device
 * @param [out] p_status_mask The pointer to status variable
 * @return true Successfully read the status
 * @return false Communication error
 */
bool npm1300_charger_status_get(npm1300_instance_t *            pm,
                                npm1300_charger_status_mask_t * p_status_mask);

/**
 * @brief Start single shot ADC measurement
 *
 * @param [in] pm The instance of nPM device
 * @param [in] request The type of ADC request to be done
 * @return true Successfully triggered single shot measurement
 * @return false Communication error
 */
bool npm1300_adc_single_shot_start(npm1300_instance_t * pm, npm1300_adc_request_t request);

/**
 * @brief Check if ADC battery voltage measurement is ready to read.
 *
 * @param [in] pm The instance of nPM device
 * @param [out] p_status The pointer to the status variable, true - measurement is ready to read
 * @return true Successfully checked status
 * @return false Communication error
 */
bool npm1300_adc_vbat_ready_check(npm1300_instance_t * pm, bool * p_status);

/**
 * @brief Configure ADC with battery NTC type
 *
 * @param [in] pm The instance of nPM device
 * @param [in] battery_ntc The type of the NTC battery thermistor
 * @return true Configuration set successfully
 * @return false Communication error
 */
bool npm1300_adc_ntc_set(npm1300_instance_t * pm, npm1300_battery_ntc_type_t battery_ntc);

/**
 * @brief Read VBAT measurement
 *
 * @param [in] pm The instance of nPM device
 * @param [out] p_voltage Battery voltage in millivolts
 * @return true Successfully read the voltage
 * @return false Communication error
 */
bool npm1300_adc_vbat_get(npm1300_instance_t * pm, uint16_t * p_voltage);

/**
 * @brief Enable VBAT auto measurement every 1 second
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully enabled VBAT auto measurement
 * @return false Communication error
 */
bool npm1300_adc_vbat_auto_meas_enable(npm1300_instance_t * pm);

/**
 * @brief Disable VBAT auto measurement every 1 second
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully disabled VBAT auto measurement
 * @return false Communication error
 */
bool npm1300_adc_vbat_auto_meas_disable(npm1300_instance_t * pm);

/**
 * @brief Enable Power Failure Detection block and set configuration data
 *
 * @param [in] pm The instance of nPM device
 * @param [in] polarity Power Failure Warning polarity
 * @param [in] threshold Vsys Comparator Threshold Select
 * @return true Successfully enabled pof and set configuration data
 * @return false Communication error
 */
bool npm1300_pof_enable(npm1300_instance_t *    pm,
                        ntc1300_pof_polarity_t  polarity,
                        ntc1300_pof_threshold_t threshold);

/**
 * @brief Disable Power Failure Detection block
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully disabled pof
 * @return false Communication error
 */
bool npm1300_pof_disable(npm1300_instance_t * pm);

/**
 * @brief Suspend mode enable and disable
 *
 * In suspend mode the current consumption from VBUS has to be limited to 2.5 mA.
 * nPM1300 can not detect suspend mode. Instead the host has to inform the chip via TWI
 * to minimize current consumption from VBUS to ISUSP (note that current consumed via pin VBUSOUT
 * is not included). VBUS is then disconnected from VSYS but VBUSOUT remains active.
 * As a consequence charging is paused. The chip exits this mode only when the host tells it to do so
 * via TWI command (charging resumes automatically), or in case VBUS is disconnected in the meanwhile.
 *
 * @param [in] pm The instance of nPM device
 * @param [in] status true - suspend mode enable, false - suspend mode disable
 * @return true Successfully enabled or disabled suspend mode
 * @return false Communication error
 */
bool npm1300_suspend_mode_set(npm1300_instance_t * pm, bool status);

#ifdef __cplusplus
}
#endif

#endif
