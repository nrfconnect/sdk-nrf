.. _nrf_desktop_battery_charger:

Battery charger module
######################

.. contents::
   :local:
   :depth: 2

The battery charger module is responsible for battery charging control.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_battery_charger_start
    :end-before: table_battery_charger_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module implemented in :file:`battery_charger.c` uses Zephyr's :ref:`zephyr:gpio_api` driver to control and monitor battery charging.
For this reason, you should set :option:`CONFIG_GPIO` option.

By default, the module is disabled and the :option:`CONFIG_DESKTOP_BATTERY_CHARGER_NONE` option is selected.
Set the option :option:`CONFIG_DESKTOP_BATTERY_CHARGER_DISCRETE` to enable the module.

The following module configuration options are available:

* :option:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PIN` - Configures the pin to which the charge status output (CSO) from the charger is connected.

  * :option:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_NONE` - CSO pin pull disabled (default setting).
  * :option:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_UP` - CSO pin pull up.
  * :option:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_DOWN` - CSO pin pull down.

* :option:`CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_PIN` - Configures the pin that enables the battery charging.
* :option:`CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_INVERSED` - Option for inversing the charging enable signal.
* :option:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ` - Set this Kconfig option to the CSO pin state switching frequency (in Hz) for when a charging error occurs.

Implementation details
**********************

As required by the USB specification, the module disables the battery charging when USB is suspended to reduce the power consumption.
In addition, it also disables charging when USB is disconnected.
The module enables charging when USB is powered or active.

.. note::
    Make sure that the hardware configuration enables charging by default (for example, when the pin to enable charging is not configured).
    Otherwise it will be impossible to enable charging when the battery is empty.

The module checks the battery state using :c:struct:`k_delayed_work`.
The check is based on the edge interrupt count on the CSO pin in a period of time defined in ``ERROR_CHECK_TIMEOUT_MS``, and on the CSO pin state while the work is processed.

On a battery state change, the new state is sent using ``battery_state_event``.
The battery state can have one of the following values:

* :c:enumerator:`BATTERY_STATE_IDLE` - Battery is not being charged (CSO pin set to logical high).
* :c:enumerator:`BATTERY_STATE_CHARGING` - Battery is being charged (CSO pin set to logical low).
* :c:enumerator:`BATTERY_STATE_ERROR` - Battery charger reported an error (a signal with the :option:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ` frequency and a 50% duty cycle on the CSO pin).
