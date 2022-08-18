/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "npm1300.h"

#define EVENTS_EVENT_CLEAR_OFFSET            0x01 ///< Clear register address offset from SET register, in bytes
#define EVENTS_INTERRUPT_ENABLE_SET_OFFSET   0x02 ///< Interrupt Enable Set register address offset from SET register, in bytes
#define EVENTS_INTERRUPT_ENABLE_CLEAR_OFFSET 0x03 ///< Interrupt Enable Clear register address offset from SET register, in bytes

/* Check if the mask for all NTC temperatures values are the same */
NPMX_STATIC_ASSERT(BCHARGER_NTCCOLDLSB_NTCCOLDLVLLSB_Msk == BCHARGER_NTCCOOLLSB_NTCCOOLLVLLSB_Msk);
NPMX_STATIC_ASSERT(BCHARGER_NTCCOLDLSB_NTCCOLDLVLLSB_Msk == BCHARGER_NTCWARMLSB_NTCWARMLVLLSB_Msk);
NPMX_STATIC_ASSERT(BCHARGER_NTCCOLDLSB_NTCCOLDLVLLSB_Msk == BCHARGER_NTCHOTLSB_NTCHOTLVLLSB_Msk);

/**
 * @brief Convert NTC uint16_t code to two bytes array representing nPM's registers
 *
 * @param [in] code uint16_t The code of expected resistance
 *
 * @return Initialised two bytes array
 */
#define NPM1300_NTC_CODE_TO_DATA(code)                                        \
{                                                                             \
    [0] = (uint8_t)((code) >> (uint16_t)2U),                                  \
    [1] = (uint8_t)((code) & (uint16_t)BCHARGER_NTCCOLDLSB_NTCCOLDLVLLSB_Msk) \
}

#define CHARGER_ENABLE_LOGIC_POSITIVE_Msk 0x3U  ///< Used to work with @ref npm1300_charger_module_mask_t, two bytes represent BCHGENABLECLR and BCHGENABLESET register
#define CHARGER_ENABLE_LOGIC_POSITIVE_Pos 0x0U  ///< Used to work with @ref npm1300_charger_module_mask_t, bytes position shift

#define CHARGER_ENABLE_LOGIC_NEGATIVE_Msk 0x3U  ///< Used to work with @ref npm1300_charger_module_mask_t, two bytes represent BCHGDISABLECLR and BCHGDISABLESET register
#define CHARGER_ENABLE_LOGIC_NEGATIVE_Pos 0x2U  ///< Used to work with @ref npm1300_charger_module_mask_t, bytes position shift

#define ADC_VFS_VBAT_MV                   5000U ///< Full scale voltage for VBAT measurement, from product specification
#define ADC_BITS_RESOLUTION               1024U ///< Bits resolution of 10-bit SAR ADC, from product specification
#define ADC_RESULT_MSB_SHIFT              0x2U  ///< ADC data from MSB's register needs to be shifted

/** @brief Possible ERRLOG registers */
typedef enum
{
    ERRLOG_REGISTER_RSTCAUSE         = 0, ///< Error log for internal reset causes
    ERRLOG_REGISTER_CHARGERERRREASON = 1, ///< Error log for slow domain
    ERRLOG_REGISTER_CHARGERERRSENSOR = 2, ///< Bcharger Fsm sensor error
    BCHARGER_REGISTER_BCHGERRREASON  = 3, ///< Charger Error log in normal mode
    BCHARGER_REGISTER_BCHGERRSENSOR  = 4, ///< Sensor Error log in normal mode
} npm1300_error_register_t;

/** @brief All possible errors addresses */
const static uint16_t errors_registers_addresses[] =
{
    [ERRLOG_REGISTER_RSTCAUSE]         = NPM1300_REG_TO_ADDR(NPM_ERRLOG->RSTCAUSE),         ///< Address of RSTCAUSE register
    [ERRLOG_REGISTER_CHARGERERRREASON] = NPM1300_REG_TO_ADDR(NPM_ERRLOG->CHARGERERRREASON), ///< Address of CHARGERERRREASON register
    [ERRLOG_REGISTER_CHARGERERRSENSOR] = NPM1300_REG_TO_ADDR(NPM_ERRLOG->CHARGERERRSENSOR), ///< Address of CHARGERERRSENSOR register
    [BCHARGER_REGISTER_BCHGERRREASON]  = NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGERRREASON),  ///< Address of BCHGERRREASON register
    [BCHARGER_REGISTER_BCHGERRSENSOR]  = NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGERRSENSOR)   ///< Address of BCHGERRSENSOR register
};

/** @brief All possible callback names */
const static char callbacks_register_name[NPM1300_CALLBACK_TYPE_COUNT][21] =
{
    [NPM1300_CALLBACK_TYPE_EVENT_ADC]                = { "ADC"               },
    [NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_TEMP]      = { "BAT_CHAR_TEMP"     },
    [NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS]    = { "BAT_CHAR_STATUS"   },
    [NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT]       = { "BAT_CHAR_BAT"      },
    [NPM1300_CALLBACK_TYPE_EVENT_SHIPHOLD]           = { "SHIPHOLD"          },
    [NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE]     = { "VBUSIN_VOLTAGE"    },
    [NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_THERMAL_USB] = { "VBUSIN_THERMAL_USB"},
    [NPM1300_CALLBACK_TYPE_EVENT_USB_B]              = { "USB_B"             },
    [NPM1300_CALLBACK_TYPE_RSTCAUSE]         =         { "RSTCAUSE"          },
    [NPM1300_CALLBACK_TYPE_CHARGERERRREASON] =         { "CHARGERERRREASON"  },
    [NPM1300_CALLBACK_TYPE_CHARGERERRSENSOR] =         { "CHARGERERRSENSOR"  },
};

/** @brief All possible callback bit names */
static const char callbacks_bits_names[NPM1300_CALLBACK_TYPE_COUNT][8][30] =
{
    [NPM1300_CALLBACK_TYPE_EVENT_ADC] =
    {
    [0] = "VBAT",              [1] = "Battery NTC",
    [2] = "Internal Die Temp", [3] = "VSYS Voltage",
    [4] = "DCDC VSET1 pin",    [5] = "DCDC VSET2 pin",
    [6] = "IBAT",              [7] = "VBUS"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_TEMP] =
    {
    [0] = "Cold Battery",  [1] = "Cool Battery",
    [2] = "Warm Battery",  [3] = "Hot Battery",
    [4] = "Die high temp", [5] = "Die resume temp"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS] =
    {
    [0] = "Supplement mode activated", [1] = "Trickle Charge started",
    [2] = "Constant Current started",  [3] = "Constant Voltage started",
    [4] = "Charging completed",        [5] = "Charging error"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT] =
    {
    [0] = "Battery Detected",
    [1] = "Battery Lost",
    [2] = "Battery re-charge needed"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_SHIPHOLD] =
    {
    [0] = "Pressed",      [1] = "Released",
    [2] = "Held to Exit", [3] = "Held for 10 s",
    [4] = "Watchdog Timeout Warn"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE] =
    {
    [0] = "VBUS input detected",    [1] = "VBUS input removed",
    [2] = "Over Voltage Detected",  [3] = "Over Voltage Removed",
    [4] = "Under Voltage Detected", [5] = "Under Voltage Removed"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_THERMAL_USB] =
    {
    [0] = "Warning detected",       [1] = "Warning removed",
    [2] = "Shutown detected",       [3] = "Shutdown removed",
    [4] = "Voltage on CC1 changes", [5] = "Voltage on CC2 changes"
    },
    [NPM1300_CALLBACK_TYPE_EVENT_USB_B] =
    {
    [0] = "USB-B DP/DN detection"
    },
    [NPM1300_CALLBACK_TYPE_RSTCAUSE] =
    {
    [0] = "Shipmode exit",    [1] = "Boot monitor timeout",
    [2] = "Watchdog timeout", [3] = "Shphld long press",
    [4] = "TSD",              [5] = "VSYS low",
    [6] = "Soft reset"
    },
    [NPM1300_CALLBACK_TYPE_CHARGERERRREASON] =
    {
    [0] = "NTC sensor error",     [1] = "VBAT Sensor Error",
    [2] = "VBAT Low Error",       [3] = "Measurement Timeout Error",
    [4] = "Charge Timeout Error", [5] = "Trickle Timeout Error"
    },
    [NPM1300_CALLBACK_TYPE_CHARGERERRSENSOR] =
    {
    [0] = "Ntc Cold sensor", [1] = "Ntc Cool sensor",
    [2] = "Ntc Warm sensor", [3] = "Ntc Hot sensor",
    [4] = "Vterm sensor",    [5] = "Recharge sensor",
    [6] = "Vtrickle sensor", [7] = "VbatLow sensor"
    },
};

static const char charger_status_bits_names[8][30] =
{
    [0] = "Battery is connected",          [1] = "Charging completed",
    [2] = "Trickle charge",                [3] = "Constant Current charging",
    [4] = "Constant Voltage charging",     [5] = "Battery re-charge is needed",
    [6] = "Charging stopped due Die Temp", [7] = "Supplement Mode Active"
};

/**
 * @brief Read specified error register from nPM device
 *
 * @param [in] pm The instance of nPM device
 * @param [in] error_register Specified error register to read
 * @param [out] p_error The pointer to error variable
 * @return true Successfully read error
 * @return false Communication error
 */
static bool read_error(npm1300_instance_t *     pm,
                       npm1300_error_register_t error_register,
                       uint8_t *                p_error)
{
    uint16_t reg = errors_registers_addresses[(uint8_t)error_register];

    return 0 == pm->read_registers(pm->user_handler, reg, p_error, 1);
}

/**
 * @brief Get the address of SET register for specified LED instance
 *
 * @param [in] led the LED instance
 * @return uint16_t SET register address of LED instance
 */
static uint16_t led_set_register_get(nmp1300_led_instance_t led)
{
    switch (led)
    {
        case NPM1300_LED_INSTANCE_0:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV0SET);
        case NPM1300_LED_INSTANCE_1:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV1SET);
        case NPM1300_LED_INSTANCE_2:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV2SET);
        default:
            NPMX_ASSERT(0);
            return 0;
    }
}

/**
 * @brief Get the address of CLR register for specified LED instance
 *
 * @param [in] led the LED instance
 * @return uint16_t CLR register address of LED instance
 */
static uint16_t led_clr_register_get(nmp1300_led_instance_t led)
{
    switch (led)
    {
        case NPM1300_LED_INSTANCE_0:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV0CLR);
        case NPM1300_LED_INSTANCE_1:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV1CLR);
        case NPM1300_LED_INSTANCE_2:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV2CLR);
        default:
            NPMX_ASSERT(0);
            return 0;
    }
}

/**
 * @brief Get the address of MODE register for specified LED instance
 *
 * @param [in] led the LED instance
 * @return uint16_t MODE register address of LED instance
 */
static uint16_t led_mode_register_get(nmp1300_led_instance_t led)
{
    switch (led)
    {
        case NPM1300_LED_INSTANCE_0:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV0MODESEL);
        case NPM1300_LED_INSTANCE_1:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV1MODESEL);
        case NPM1300_LED_INSTANCE_2:
            return NPM1300_REG_TO_ADDR(NPM_LEDDRV->LEDDRV2MODESEL);
        default:
            NPMX_ASSERT(0);
            return 0;
    }
}

/**
 * @brief Get the address of MODE register for specified GPIO
 *
 * @param [in] gpio the GPIO instance
 * @return uint16_t MODE register address of GPIO instance
 */
static uint16_t gpio_mode_register_get(npm1300_gpio_instance_t gpio)
{
    switch (gpio)
    {
        case NPM1300_GPIO_INSTANCE_0:
            return NPM1300_REG_TO_ADDR(NPM_GPIOS->GPIOMODE0);
        case NPM1300_GPIO_INSTANCE_1:
            return NPM1300_REG_TO_ADDR(NPM_GPIOS->GPIOMODE1);
        case NPM1300_GPIO_INSTANCE_2:
            return NPM1300_REG_TO_ADDR(NPM_GPIOS->GPIOMODE2);
        case NPM1300_GPIO_INSTANCE_3:
            return NPM1300_REG_TO_ADDR(NPM_GPIOS->GPIOMODE3);
        case NPM1300_GPIO_INSTANCE_4:
            return NPM1300_REG_TO_ADDR(NPM_GPIOS->GPIOMODE4);
        default:
            NPMX_ASSERT(0);
            return 0;
    }
}

/**
 * @brief Clear the Errlog registers
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully cleared error log
 * @return false Communication error
 */
static bool clear_error_log(npm1300_instance_t * pm)
{
    uint8_t data = ERRLOG_TASKCLRERRLOG_TASKCLRERRLOG_Msk;

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_ERRLOG->TASKCLRERRLOG),
                                    &data,
                                    1);
}

/**
 * @brief Get the address of Set register for specified event group
 *
 * @param [in] event Specified event group
 * @return uint16_t The address of Set register
 */
static uint16_t event_set_register_get(npm1300_event_group_t event)
{
    switch (event)
    {
        case NPM1300_EVENT_GROUP_ADC:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSADCSET);
        case NPM1300_EVENT_GROUP_BAT_CHAR_TEMP:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSBCHARGER0SET);
        case NPM1300_EVENT_GROUP_BAT_CHAR_STATUS:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSBCHARGER1SET);
        case NPM1300_EVENT_GROUP_BAT_CHAR_BAT:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSBCHARGER2SET);
        case NPM1300_EVENT_GROUP_SHIPHOLD:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSSHPHLDSET);
        case NPM1300_EVENT_GROUP_VBUSIN_VOLTAGE:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSVBUSIN0SET);
        case NPM1300_EVENT_GROUP_VBUSIN_THERMAL:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSVBUSIN1SET);
        case NPM1300_EVENT_GROUP_USB_B:
            return NPM1300_REG_TO_ADDR(NPM_MAIN->EVENTSUSBBSET);
        default:
            NPMX_ASSERT(0);
            return 0;
    }
}

/**
 * @brief Get the address of Clear register for specified event group
 *
 * @param [in] event Specified event group
 * @return uint16_t The address of Clear register
 */
static uint16_t event_clear_register_get(npm1300_event_group_t event)
{
    return event_set_register_get(event) + EVENTS_EVENT_CLEAR_OFFSET;
}

/**
 * @brief Get the address of Interrupt Enable register for specified event group
 *
 * @param [in] event Specified event group
 * @return uint16_t The address of Interrupt Enable register
 */
static uint16_t event_irq_enable_register_get(npm1300_event_group_t event)
{
    return event_set_register_get(event) + EVENTS_INTERRUPT_ENABLE_SET_OFFSET;
}

/**
 * @brief Get the address of Interrupt Clear (Disable) register for specified event group
 *
 * @param [in] event Specified event group
 * @return uint16_t The address of Interrupt Clear (Disable) register
 */
static uint16_t event_irq_disable_register_get(npm1300_event_group_t event)
{
    return event_set_register_get(event) + EVENTS_INTERRUPT_ENABLE_CLEAR_OFFSET;
}

/**
 * @brief Get event callback based on event group
 *
 * @param [in] event Specified event group
 * @return npm1300_callback_type_t callback
 */
static npm1300_callback_type_t event_callback_get(npm1300_event_group_t event)
{
    switch (event)
    {
        case NPM1300_EVENT_GROUP_ADC:
            return NPM1300_CALLBACK_TYPE_EVENT_ADC;
        case NPM1300_EVENT_GROUP_BAT_CHAR_TEMP:
            return NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_TEMP;
        case NPM1300_EVENT_GROUP_BAT_CHAR_STATUS:
            return NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS;
        case NPM1300_EVENT_GROUP_BAT_CHAR_BAT:
            return NPM1300_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT;
        case NPM1300_EVENT_GROUP_SHIPHOLD:
            return NPM1300_CALLBACK_TYPE_EVENT_SHIPHOLD;
        case NPM1300_EVENT_GROUP_VBUSIN_VOLTAGE:
            return NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE;
        case NPM1300_EVENT_GROUP_VBUSIN_THERMAL:
            return NPM1300_CALLBACK_TYPE_EVENT_VBUSIN_THERMAL_USB;
        case NPM1300_EVENT_GROUP_USB_B:
            return NPM1300_CALLBACK_TYPE_EVENT_USB_B;
        default:
            NPMX_ASSERT(0);
            return 0;
    }
}

/**
 * @brief Get events field from specified event group
 *
 * @param [in] pm The instance of nPM device
 * @param [in] event Specified event group type
 * @param [out] p_flags_mask The pointer to the flags variable, npm1300_event_group_xxx_t for selected event group
 * @return true Successfully read the status event register
 * @return false Communication error
 */
static bool event_get(npm1300_instance_t * pm, npm1300_event_group_t event, uint8_t * p_flags_mask)
{
    return 0 == pm->read_registers(pm->user_handler,
                                   event_set_register_get(event),
                                   p_flags_mask,
                                   1);
}

/**
 * @brief Clear event fields in specified event group
 * @param [in] pm The instance of nPM device
 * @param [in] event Specified event group type
 * @param [in] flags Specified bit flags for event group type
 * @return true Event cleared successfully
 * @return false Event not cleared
 */
static bool event_clear(npm1300_instance_t * pm, npm1300_event_group_t event, uint8_t flags_mask)
{
    uint16_t reg = event_clear_register_get(event);

    return 0 == pm->write_registers(pm->user_handler, reg, &flags_mask, 1);
}

/**
 * @brief Set GPIO mode
 *
 * @param [in] pm The instance of nPM device
 * @param [in] gpio The instance of GPIO driver
 * @param [in] mode THe mode of GPIO
 * @return true Successfully set the GPIO mode
 * @return false Communication error
 */
static bool gpio_mode_set(npm1300_instance_t *    pm,
                          npm1300_gpio_instance_t gpio,
                          nmp1300_gpio_mode_t     mode)
{
    uint16_t reg = gpio_mode_register_get(gpio);

    return 0 == pm->write_registers(pm->user_handler, reg, (uint8_t *)(&mode), 1);
}

/**
 * @brief Check if any registered event in event group happened on nPM device
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully checked all registered events
 * @return false Communication error
 */
static bool check_events(npm1300_instance_t * pm)
{
    uint8_t            flags_mask;
    npm1300_callback_t cb;

    for (uint8_t i = 0; i < NPM1300_EVENT_GROUP_COUNT; i++)
    {
        if (0 == pm->event_group_enable_mask[i])  /* skip checking unregistered event group */
        {
            continue;
        }

        if (false == event_get(pm, (npm1300_event_group_t)i, &flags_mask))
        {
            return false;
        }

        flags_mask &= pm->event_group_enable_mask[i];

        if (flags_mask)
        {
            if (false == event_clear(pm, (npm1300_event_group_t)i, flags_mask))
            {
                return false;
            }
            cb = pm->registered_cb[event_callback_get((npm1300_event_group_t)i)];
            if (cb)
            {
                cb(pm, event_callback_get((npm1300_event_group_t)i), flags_mask);
            }
        }
    }
    return true;
}

/**
 * @brief Check possible errors in nPM device and run callbacks
 *
 * @param [in] pm The instance of nPM device
 * @return true Successfully checked all errors
 * @return false Communication error
 */
static bool check_errors(npm1300_instance_t * pm)
{
    uint8_t rst_cause = 0;

    if (false == read_error(pm, ERRLOG_REGISTER_RSTCAUSE, &rst_cause))
    {
        return false;
    }

    /* in default mode 'Charger-FSM Error' is in BCHGERRREASON register */
    uint8_t charger_reason = 0;

    if (false == read_error(pm, BCHARGER_REGISTER_BCHGERRREASON, &charger_reason))
    {
        return false;
    }

    if (0 == charger_reason)
    {
        /** TODO: information about slow domain should be
         * in npm1300_instance_t to avoid multiple I2C errors reading
         * */
        /* In slowDomain error is in CHARGERERRREASON */
        if (false == read_error(pm, ERRLOG_REGISTER_CHARGERERRREASON, &charger_reason))
        {
            return false;
        }
    }

    /* in default mode CHARGERERRSENSOR is in BCHGERRSENSOR register */
    uint8_t charger_sensor = 0;

    if (false == read_error(pm, BCHARGER_REGISTER_BCHGERRSENSOR, &charger_sensor))
    {
        return false;
    }

    if (0 == charger_sensor)
    {
        /** TODO: information about slow domain should be
         * in npm1300_instance_t to avoid multiple I2C errors reading
         * */
        /* In slowDomain error is in CHARGERERRSENSOR */
        if (false == read_error(pm, ERRLOG_REGISTER_CHARGERERRSENSOR, &charger_sensor))
        {
            return false;
        }
    }

    npm1300_callback_t cb = pm->registered_cb[(uint32_t)NPM1300_CALLBACK_TYPE_RSTCAUSE];

    if (cb && rst_cause)
    {
        cb(pm, NPM1300_CALLBACK_TYPE_RSTCAUSE, rst_cause);
    }

    cb = pm->registered_cb[(uint32_t)NPM1300_CALLBACK_TYPE_CHARGERERRREASON];

    if (cb && charger_reason)
    {
        cb(pm, NPM1300_CALLBACK_TYPE_CHARGERERRREASON, charger_reason);
    }

    cb = pm->registered_cb[(uint32_t)NPM1300_CALLBACK_TYPE_CHARGERERRSENSOR];

    if (cb && charger_sensor)
    {
        cb(pm, NPM1300_CALLBACK_TYPE_CHARGERERRSENSOR, charger_sensor);
    }

    if (rst_cause || charger_reason || charger_sensor)
    {
        return clear_error_log(pm);
    }
    return true;
}

/**
 * @brief Calculate NTC code based on nominal and desired NTC resistance
 *
 * @param [in] desired_resistance The resistance value in Ohms at which NTC threshold event happen.
 * @param [in] nominal_resistance The nominal resistance value of NTC battery resistor in Ohms.
 * @return uint16_t Registers code
 */
static uint16_t ntc_code_get(uint32_t desired_resistance, uint32_t nominal_resistance)
{
    uint32_t code = 1024; /* 2 ^ 10 */

    code *= desired_resistance;
    code /= desired_resistance + nominal_resistance;
    return (uint16_t)(code & (uint32_t)0xFFFF);
}

const char * npm1300_callback_to_str(npm1300_callback_type_t type)
{
    NPMX_ASSERT(type < NPM1300_CALLBACK_TYPE_COUNT);
    if (type < NPM1300_CALLBACK_TYPE_COUNT)
    {
        return callbacks_register_name[type];
    }
    else
    {
        return NULL;
    }
}

const char * npm1300_callback_bit_to_str(npm1300_callback_type_t type, uint8_t bit)
{
    NPMX_ASSERT(type < NPM1300_CALLBACK_TYPE_COUNT);
    NPMX_ASSERT(bit < 8);
    if ((type < NPM1300_CALLBACK_TYPE_COUNT) && (bit < 8))
    {
        return callbacks_bits_names[type][bit];
    }
    else
    {
        return NULL;
    }
}

const char * npm1300_charger_status_bit_to_str(uint8_t bit)
{
    NPMX_ASSERT(bit < 8);
    if (bit < 8)
    {
        return charger_status_bits_names[bit];
    }
    else
    {
        return NULL;
    }
}

bool npm1300_stop_boot_timer(npm1300_instance_t * pm)
{
    uint8_t data = (TIMER_TIMERCLR_TASKTIMERDIS_SET << TIMER_TIMERCLR_TASKTIMERDIS_Pos);
    int     ret  = pm->write_registers(pm->user_handler,
                                       NPM1300_REG_TO_ADDR(NPM_TIMER->TIMERCLR),
                                       &data,
                                       1);

    return ret == 0;
}

bool npm1300_connection_check(npm1300_instance_t * pm)
{
    uint8_t data;
    int     ret = pm->read_registers(pm->user_handler,
                                     NPM1300_REG_TO_ADDR(NPM_GPIOS->GPIOPDEN0),
                                     &data,
                                     1);

    return (data == GPIOS_GPIOPDEN0_GPIOPDEN_PULLDOWN1) && ret == 0;
}

bool npm1300_check_errors(npm1300_instance_t * pm)
{
    return check_errors(pm);
}

bool npm1300_led_mode_set(npm1300_instance_t *   pm,
                          nmp1300_led_instance_t led,
                          npm1300_led_mode_t     mode)
{
    uint16_t reg = led_mode_register_get(led);
    int      ret = pm->write_registers(pm->user_handler, reg, (uint8_t *)(&mode), 1);

    if (0 == ret)
    {
        pm->led_mode[(uint8_t)led] = mode;
        return true;
    }
    return false;
}

bool npm1300_led_state_set(npm1300_instance_t * pm, nmp1300_led_instance_t led, bool state)
{
    if (pm->led_mode[(uint8_t)led] == NPM1300_LED_MODE_HOST)
    {
        uint16_t reg  = state ? led_set_register_get(led) : led_clr_register_get(led);
        uint8_t  data = state ?
                        (LEDDRV_LEDDRV0SET_LEDDRV0ON_SET << LEDDRV_LEDDRV0SET_LEDDRV0ON_Pos) :
                        (LEDDRV_LEDDRV0CLR_LEDDRV0OFF_CLR << LEDDRV_LEDDRV0CLR_LEDDRV0OFF_Pos);
        return 0 == pm->write_registers(pm->user_handler, reg, &data, 1);
    }
    return false;
}

bool npm1300_gpio_mode_set(npm1300_instance_t *    pm,
                           npm1300_gpio_instance_t gpio,
                           nmp1300_gpio_mode_t     mode)
{
    return gpio_mode_set(pm, gpio, mode);
}

bool npm1300_gpio_state_set(npm1300_instance_t * pm, npm1300_gpio_instance_t gpio, bool state)
{
    nmp1300_gpio_mode_t mode = state ?
                               NPM1300_GPIO_MODE_OUTPUT_OVERRIDE_1 :
                               NPM1300_GPIO_MODE_OUTPUT_OVERRIDE_0;

    return gpio_mode_set(pm, gpio, mode);
}

void npm1300_init(npm1300_instance_t * pm)
{
    pm->led_mode[(uint32_t)NPM1300_LED_INSTANCE_0] = NPM1300_LED_MODE_ERROR;    /* From product specification */
    pm->led_mode[(uint32_t)NPM1300_LED_INSTANCE_1] = NPM1300_LED_MODE_CHARGING; /* From product specification */
    pm->led_mode[(uint32_t)NPM1300_LED_INSTANCE_2] = NPM1300_LED_MODE_HOST;     /* From product specification */
    pm->adc_config = ADC_ADCCONFIG_ResetValue;                                  /* ADC Configuration register value from product specification */
    if (pm->generic_cb)
    {
        for (uint32_t i = 0; i < NPM1300_CALLBACK_TYPE_COUNT; i++)
        {
            pm->registered_cb[i] = pm->generic_cb;
        }
    }
}

void npm1300_register_cb(npm1300_instance_t *    pm,
                         npm1300_callback_t      cb,
                         npm1300_callback_type_t type)
{
    if (cb)
    {
        pm->registered_cb[(uint32_t)type] = cb;
    }
}

bool npm1300_event_interrupt_enable(npm1300_instance_t *  pm,
                                    npm1300_event_group_t event,
                                    uint8_t               flags_mask)
{
    event_clear(pm, event, flags_mask);
    pm->event_group_enable_mask[(uint8_t)event] = flags_mask;
    uint16_t reg = event_irq_enable_register_get(event);
    return 0 == pm->write_registers(pm->user_handler, reg, &flags_mask, 1);
}

bool npm1300_event_get(npm1300_instance_t * pm, npm1300_event_group_t event, uint8_t * p_flags_mask)
{
    return event_get(pm, event, p_flags_mask);
}

bool npm1300_event_clear(npm1300_instance_t * pm, npm1300_event_group_t event, uint8_t flags_mask)
{
    return event_clear(pm, event, flags_mask);
}

bool npm1300_proc(npm1300_instance_t * pm)
{
    if (pm->interrupt)
    {
        pm->interrupt = false;
        if (false == check_events(pm))
        {
            return false;
        }
    }

    if (pm->charger_error)
    {
        pm->charger_error = false;
        if (false == check_errors(pm))
        {
            return false;
        }
    }
    return true;
}

void npm1300_interrupt(npm1300_instance_t * pm)
{
    pm->interrupt = true;
}

bool npm1300_clear_charger_error(npm1300_instance_t * pm)
{
    uint8_t data = 0x01;

    // TODO: check if TASKRELEASEERR (Release Charger from Error) is required after clearing
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->TASKCLEARCHGERR),
                                    &data,
                                    1);
}

bool npm1300_charger_module_enable(npm1300_instance_t *          pm,
                                   npm1300_charger_module_mask_t flags_mask)
{
    int ret_0 = 0;
    int ret_1 = 0;

    uint8_t data_enable_register =
        (uint8_t)((uint8_t)flags_mask >> (uint8_t)CHARGER_ENABLE_LOGIC_POSITIVE_Pos) &
        (uint8_t)CHARGER_ENABLE_LOGIC_POSITIVE_Msk;

    uint8_t data_disable_register =
        (uint8_t)((uint8_t)flags_mask >> (uint8_t)CHARGER_ENABLE_LOGIC_NEGATIVE_Pos) &
        (uint8_t)CHARGER_ENABLE_LOGIC_NEGATIVE_Msk;

    if (data_enable_register > 0)
    {
        ret_0 = pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGENABLESET),
                                    &data_enable_register,
                                    1);
    }
    if (data_disable_register > 0)
    {
        ret_1 = pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGDISABLECLR),
                                    &data_disable_register,
                                    1);
    }
    return (0 == ret_0) && (0 == ret_1);
}

bool npm1300_charger_module_disable(npm1300_instance_t *          pm,
                                    npm1300_charger_module_mask_t flags_mask)
{
    int     ret_0 = 0;
    int     ret_1 = 0;
    uint8_t data_disable_register =
        (uint8_t)((uint8_t)flags_mask >> (uint8_t)CHARGER_ENABLE_LOGIC_POSITIVE_Pos) &
        (uint8_t)CHARGER_ENABLE_LOGIC_POSITIVE_Msk;

    uint8_t data_enable_register =
        (uint8_t)((uint8_t)flags_mask >> (uint8_t)CHARGER_ENABLE_LOGIC_NEGATIVE_Pos) &
        (uint8_t)CHARGER_ENABLE_LOGIC_NEGATIVE_Msk;

    if (data_disable_register > 0)
    {
        ret_0 = pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGENABLECLR),
                                    &data_disable_register,
                                    1);
    }
    if (data_enable_register > 0)
    {
        ret_1 = pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGDISABLESET),
                                    &data_enable_register,
                                    1);
    }
    return (0 == ret_0) && (0 == ret_1);
}

bool npm1300_charger_module_get(npm1300_instance_t *            pm,
                                npm1300_charger_module_mask_t * p_modules_mask)
{
    uint8_t ret_data[3];

    /* Only two bytes are used, but reading three bytes in a row is faster */
    if (0 != pm->read_registers(pm->user_handler,
                                NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGENABLESET),
                                &ret_data[0],
                                3))
    {
        return false;
    }
    *p_modules_mask = ((ret_data[0] << CHARGER_ENABLE_LOGIC_POSITIVE_Pos) &
                       (uint8_t)(CHARGER_ENABLE_LOGIC_POSITIVE_Msk));
    ret_data[2]      = ~ret_data[2] & (uint8_t)(CHARGER_ENABLE_LOGIC_NEGATIVE_Msk);
    ret_data[2]    <<= CHARGER_ENABLE_LOGIC_NEGATIVE_Pos;
    *p_modules_mask |= ret_data[2];
    return true;
}

bool npm1300_charger_charging_current_set(npm1300_instance_t * pm, uint16_t current)
{
    if (current == 0)
    {
        return false;
    }
    uint8_t data[2] = {0, 0};
    if (current < 32)
    {
        current = 32;
    }
    if (current > 800)
    {
        current = 800;
    }
    current /= 2U;
    data[1]  = (uint8_t)(current & 1U);
    data[0]  = (uint8_t)(current / 2U);
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGISETMSB),
                                    data,
                                    2);
}

bool npm1300_charger_discharging_current_set(npm1300_instance_t * pm, uint16_t current)
{
    if (current == 0)
    {
        return false;
    }
    uint8_t data[2] = {0, 0};
    if (current < 270)
    {
        current = 270;
    }
    if (current > 1340)
    {
        current = 1340;
    }
    current /= 2U;
    data[1]  = (uint8_t)(current & 1U);
    data[0]  = (uint8_t)(current / 2U);
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGISETDISCHARGEMSB),
                                    data,
                                    2);
}

bool npm1300_charger_termination_volatge_normal_set(npm1300_instance_t *      pm,
                                                    npm1300_charger_voltage_t voltage)
{
    NPMX_ASSERT(voltage <= NPM1300_CHARGER_VOLTAGE_4V45);
    uint8_t data = (uint8_t)voltage;
    if (voltage > NPM1300_CHARGER_VOLTAGE_4V45)
    {
        return false;
    }
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGVTERM),
                                    &data,
                                    1);
}

bool npm1300_charger_termination_volatge_warm_set(npm1300_instance_t *      pm,
                                                  npm1300_charger_voltage_t voltage)
{
    NPMX_ASSERT(voltage <= NPM1300_CHARGER_VOLTAGE_4V45);
    /* TODO: check possible endianness to remove temporary data variable */
    uint8_t data = (uint8_t)voltage;
    if (voltage > NPM1300_CHARGER_VOLTAGE_4V45)
    {
        return false;
    }
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGVTERMR),
                                    &data,
                                    1);
}

bool npm1300_charger_trickle_level_set(npm1300_instance_t * pm, npm1300_charger_trickle_t val)
{
    /* TODO: check possible endianness to remove temporary data variable */
    uint8_t data = (uint8_t)val;

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGVTRICKLESEL),
                                    &data,
                                    1);
}

bool npm1300_charger_iterm_level_set(npm1300_instance_t * pm, npm1300_charger_iterm_t iterm)
{
    /* TODO: check possible endianness to remove temporary data variable */
    uint8_t data = (uint8_t)iterm;

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGITERMSEL),
                                    &data,
                                    1);
}

bool npm1300_charger_cold_resistance_set(npm1300_instance_t * pm, uint32_t resistance)
{
    if (resistance == 0)
    {
        return false;
    }
    uint16_t code    = ntc_code_get(resistance, pm->ntc_resistance);
    uint8_t  data[2] = NPM1300_NTC_CODE_TO_DATA(code);
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->NTCCOLD),
                                    data,
                                    2);
}

bool npm1300_charger_cool_resistance_set(npm1300_instance_t * pm, uint32_t resistance)
{
    if (resistance == 0)
    {
        return false;
    }
    uint16_t code    = ntc_code_get(resistance, pm->ntc_resistance);
    uint8_t  data[2] = NPM1300_NTC_CODE_TO_DATA(code);
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->NTCCOOL),
                                    data,
                                    2);
}

bool npm1300_charger_warm_resistance_set(npm1300_instance_t * pm, uint32_t resistance)
{
    if (resistance == 0)
    {
        return false;
    }
    uint16_t code    = ntc_code_get(resistance, pm->ntc_resistance);
    uint8_t  data[2] = NPM1300_NTC_CODE_TO_DATA(code);
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->NTCWARM),
                                    data,
                                    2);
}

bool npm1300_charger_hot_resistance_set(npm1300_instance_t * pm, uint32_t resistance)
{
    if (resistance == 0)
    {
        return false;
    }
    uint16_t code    = ntc_code_get(resistance, pm->ntc_resistance);
    uint8_t  data[2] = NPM1300_NTC_CODE_TO_DATA(code);
    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_BCHARGER->NTCHOT),
                                    data,
                                    2);
}

bool npm1300_charger_status_get(npm1300_instance_t *            pm,
                                npm1300_charger_status_mask_t * p_status_mask)
{
    /* TODO: check possible endianness to remove temporary data variable */
    uint8_t data = 0;
    int     ret  = pm->read_registers(pm->user_handler,
                                      NPM1300_REG_TO_ADDR(NPM_BCHARGER->BCHGCHARGESTATUS),
                                      &data,
                                      1);

    if (ret == 0)
    {
        *p_status_mask = (npm1300_charger_status_mask_t)data;
        return true;
    }
    return false;
}

bool npm1300_adc_single_shot_start(npm1300_instance_t * pm, npm1300_adc_request_t request)
{
    uint16_t write_address = 0;

    switch (request)
    {
        case NPM1300_ADC_REQUEST_BAT:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKVBATMEASURE);
            break;
        case NPM1300_ADC_REQUEST_NTC:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKNTCMEASURE);
            break;
        case NPM1300_ADC_REQUEST_DIE_TEMP:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKTEMPMEASURE);
            break;
        case NPM1300_ADC_REQUEST_VSYS:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKVSYSMEASURE);
            break;
        case NPM1300_ADC_REQUEST_VSET1:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKVSET1MEASURE);
            break;
        case NPM1300_ADC_REQUEST_VSET2:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKVSET2MEASURE);
            break;
        case NPM1300_ADC_REQUEST_IBAT:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKIBATMEASURE);
            break;
        case NPM1300_ADC_REQUEST_VBUS:
            write_address = NPM1300_REG_TO_ADDR(NPM_ADC->TASKVBUS7MEASURE);
            break;
        default:
            NPMX_ASSERT(0);
            return false;
    }

    uint8_t data = 1U; /* All task registers require 1U value to be triggered */

    return 0 == pm->write_registers(pm->user_handler, write_address, &data, 1);
}

bool npm1300_adc_ntc_set(npm1300_instance_t * pm, npm1300_battery_ntc_type_t battery_ntc)
{
    uint8_t data = (uint8_t)battery_ntc;

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_ADC->ADCNTCRSEL),
                                    &data,
                                    1);
}

bool npm1300_adc_vbat_ready_check(npm1300_instance_t * pm, bool * p_status)
{
    uint8_t flags_mask;
    bool    ret = event_get(pm, NPM1300_EVENT_GROUP_ADC, &flags_mask);

    if (ret)
    {
        flags_mask &= (uint8_t)NPM1300_EVENT_GROUP_ADC_BAT_READY_MASK;
        *p_status   = flags_mask > 0;
        if (flags_mask)
        {
            return event_clear(pm, NPM1300_EVENT_GROUP_ADC, flags_mask);
        }
        return true;
    }
    return false;
}

bool npm1300_adc_vbat_get(npm1300_instance_t * pm, uint16_t * p_voltage)
{
    uint8_t data[5];

    /* TODO: Reading 5 bytes in row is faster then two reading requests.
     *  For now only battery voltage data is used. */
    if (0 != pm->read_registers(pm->user_handler,
                                NPM1300_REG_TO_ADDR(NPM_ADC->ADCVBATRESULTMSB),
                                &data[0],
                                5))
    {
        return false;
    }

    /* Get two LSB bits from ADCGP0RESULTLSBS register.
     *  ADC result LSB's (Vbat, Ntc, Temp and Vsys). */
    static const uint8_t delta_reg_vbat = NPM1300_REG_TO_ADDR(NPM_ADC->ADCGP0RESULTLSBS) -
                                          NPM1300_REG_TO_ADDR(NPM_ADC->ADCVBATRESULTMSB);
    uint16_t code = data[delta_reg_vbat] & (uint8_t)ADC_ADCGP0RESULTLSBS_VBATRESULTLSB_Msk;

    code >>= ADC_ADCGP0RESULTLSBS_VBATRESULTLSB_Pos;

    /* Get MSB data from ADCVBATRESULTMSB register */
    code |= ((uint16_t)data[0] << ADC_RESULT_MSB_SHIFT);

    *p_voltage = code * ADC_VFS_VBAT_MV / ADC_BITS_RESOLUTION;
    return true;
}

bool npm1300_adc_vbat_auto_meas_enable(npm1300_instance_t * pm)
{
    /* Fix from issue, Autoupdate needs to be enabled before VBATAUTOENABLE in ADCCONFIG */

    uint8_t data = (uint8_t)(ADC_TASKAUTOTIMUPDATE_TASKAUTOTIMUPDATE_UPDATEAUTOTIM
                             << ADC_TASKAUTOTIMUPDATE_TASKAUTOTIMUPDATE_Pos);

    if (0 != pm->write_registers(pm->user_handler,
                                 NPM1300_REG_TO_ADDR(NPM_ADC->TASKAUTOTIMUPDATE),
                                 &data,
                                 1))
    {
        return false;
    }

    pm->adc_config |= (uint8_t)ADC_ADCCONFIG_VBATAUTOENABLE_Msk;

    /* Set register value for enable VBAT auto measurement every 1 second */
    if (0 != pm->write_registers(pm->user_handler,
                                 NPM1300_REG_TO_ADDR(NPM_ADC->ADCCONFIG),
                                 &pm->adc_config,
                                 1))
    {
        return false;
    }
    return true;
}

bool npm1300_adc_vbat_auto_meas_disable(npm1300_instance_t * pm)
{
    pm->adc_config &= ~(uint8_t)ADC_ADCCONFIG_VBATAUTOENABLE_Msk;

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_ADC->ADCCONFIG),
                                    &pm->adc_config,
                                    1);
}

bool npm1300_pof_enable(npm1300_instance_t *    pm,
                        ntc1300_pof_polarity_t  polarity,
                        ntc1300_pof_threshold_t threshold)
{
    uint8_t data = (uint8_t)(POF_POFCONFIG_POFENA_ENABLED << POF_POFCONFIG_POFENA_Pos);

    data |= (uint8_t)(polarity << POF_POFCONFIG_POFWARNPOLARITY_Pos);

    data |= (uint8_t)((threshold << POF_POFCONFIG_POFVSYSTHRESHSEL_Pos) &
                      POF_POFCONFIG_POFVSYSTHRESHSEL_Msk);

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_POF->POFCONFIG),
                                    &data,
                                    1);
}

bool npm1300_pof_disable(npm1300_instance_t * pm)
{
    uint8_t data = (uint8_t)POF_POFCONFIG_ResetValue;

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_POF->POFCONFIG),
                                    &data,
                                    1);
}

bool npm1300_suspend_mode_set(npm1300_instance_t * pm, bool status)
{
    uint8_t data = (uint8_t)(status ?
                             (VBUSIN_VBUSSUSPEND_VBUSSUSPENDENA_SUSPENDMODE
                              << VBUSIN_VBUSSUSPEND_VBUSSUSPENDENA_Pos) :
                             (VBUSIN_VBUSSUSPEND_VBUSSUSPENDENA_NORMAL
                              << VBUSIN_VBUSSUSPEND_VBUSSUSPENDENA_Pos));

    return 0 == pm->write_registers(pm->user_handler,
                                    NPM1300_REG_TO_ADDR(NPM_VBUSIN->VBUSSUSPEND),
                                    &data,
                                    1);
}
