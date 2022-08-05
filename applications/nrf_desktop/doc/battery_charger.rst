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

The module requires a node labeled ``battery_charger`` with a ``battery-charger`` compatible set in Devicetree.
The charge status output (CSO) GPIO spec, the Enable GPIO spec, and the CSO switching frequency properties are also required.
See the following snippet for an example:

.. code-block:: devicetree

    battery_charger: battery-charger {
        compatible = "battery-charger";
        cso-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
        enable-gpios = <&gpio0 1 GPIO_ACTIVE_HIGH>;
        cso-switching-freq = <1000>;
    };

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
