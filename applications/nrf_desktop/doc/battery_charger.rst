.. _nrf_desktop_battery_charger:

Battery charger module
######################

The ``battery_charger`` module is responsible for battery charging control.

Module Events
*************

+------------------+---------------------+---------------------+-------------------------+------------------+
| Source Module    | Input Event         | This Module         | Output Event            | Sink Module      |
+==================+=====================+=====================+=========================+==================+
|                  |                     | ``battery_charger`` | ``battery_state_event`` | ``led_state``    |
+------------------+---------------------+                     +-------------------------+------------------+
| ``usb_state```   | ``usb_state_event`` |                     |                         |                  |
+------------------+---------------------+---------------------+-------------------------+------------------+

Configuration
*************

The module implemented in ``battery_charger.c`` uses the Zephyr :ref:`zephyr:gpio_api` driver to control and monitor battery charging. For this reason, you should set :option:`CONFIG_GPIO` option.

By default, the module is disabled and the ``CONFIG_DESKTOP_BATTERY_CHARGER_NONE`` option is selected.
Set the option ``CONFIG_DESKTOP_BATTERY_CHARGER_DISCRETE`` to enable the module.

The following module configuration options are available:

* ``CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PIN`` - pin to which the charge status output (CSO) from the charger is connected.

  * ``CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_NONE`` - CSO pin pull disabled (default setting).
  * ``CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_UP`` - CSO pin pull up.
  * ``CONFIG_DESKTOP_BATTERY_CHARGER_CSO_PULL_DOWN`` - CSO pin pull down.

* ``CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_PIN`` - pin that enables the battery charging.
* ``CONFIG_DESKTOP_BATTERY_CHARGER_ENABLE_INVERSED`` - option inversing charging enable signal.
* ``CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ`` - CSO pin state switching frequency for when a charging error occurrs (in Hz).

Implementation details
**********************

As required by the USB specification, in order to reduce power consumption, the module disables the battery charging when the USB is suspended. In addition, it also disables charging when the USB is disconnected.
The module enables charging when the USB is powered or active.

.. warning::
  Make sure hardware configuration enables charging by default (for example, when the pin to enable charging is not configured).
  Otherwise, it will be impossible to enable charging when the battery is empty.

The module checks the battery state using :c:type:`struct k_delayed_work`.
The check is based on the edge interrupt count on the CSO pin in a defined period of time (``ERROR_CHECK_TIMEOUT_MS``) and on the CSO pin state while the work is processed.

On a battery state change, the new state is sent using ``battery_state_event``.
The battery state can have one of the following values:

* :cpp:enumerator:`BATTERY_STATE_IDLE` - battery is not being charged (CSO pin set to logical high).
* :cpp:enumerator:`BATTERY_STATE_CHARGING` - battery is being charged (CSO pin set to logical low).
* :cpp:enumerator:`BATTERY_STATE_ERROR` - the battery charger reported an error (a signal with the ``CONFIG_DESKTOP_BATTERY_CHARGER_CSO_FREQ`` frequency and a 50% duty cycle on the CSO pin).
