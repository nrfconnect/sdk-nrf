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
For this reason, you should set :kconfig:option:`CONFIG_GPIO` option.

By default, the module is disabled and the :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_NONE <config_desktop_app_options>` option is selected.
Set the option :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_DISCRETE <config_desktop_app_options>` to enable the module.

The following module configuration options are available:

* :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PIN <config_desktop_app_options>` - Configures the pin to which the charge status output (CSO) from the charger is connected.

  * :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_NONE <config_desktop_app_options>` - CSO pin pull disabled (default setting).
  * :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_UP <config_desktop_app_options>` - CSO pin pull up.
  * :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_DOWN <config_desktop_app_options>` - CSO pin pull down.

* :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_PIN <config_desktop_app_options>` - Configures the pin that enables the battery charging.
* :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_INVERSED <config_desktop_app_options>` - Option for inversing the charging enable signal.
* :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ <config_desktop_app_options>` - Set this Kconfig option to the CSO pin state switching frequency (in Hz) for when a charging error occurs.

Implementation details
**********************

As required by the USB specification, the module disables the battery charging when USB is suspended to reduce the power consumption.
In addition, it also disables charging when USB is disconnected.
The module enables charging when USB is powered or active.

.. note::
    Make sure that the hardware configuration enables charging by default (for example, when the pin to enable charging is not configured).
    Otherwise, it will be impossible to enable charging when the battery is empty.

The module checks the battery state using :c:struct:`k_work_delayable`.
The check is based on the edge interrupt count on the CSO pin in a period of time defined in ``ERROR_CHECK_TIMEOUT_MS``, and on the CSO pin state while the work is processed.

On a battery state change, the new state is sent using ``battery_state_event``.
The battery state can have one of the following values:

* :c:enumerator:`BATTERY_STATE_IDLE` - Battery is not being charged (CSO pin set to logical high).
* :c:enumerator:`BATTERY_STATE_CHARGING` - Battery is being charged (CSO pin set to logical low).
* :c:enumerator:`BATTERY_STATE_ERROR` - Battery charger reported an error (a signal with the :ref:`CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ <config_desktop_app_options>` frequency and a 50% duty cycle on the CSO pin).
