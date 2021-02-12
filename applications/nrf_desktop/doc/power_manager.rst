.. _nrf_desktop_power_manager:

Power manager module
####################

.. contents::
   :local:
   :depth: 2

Use the power manager module to reduce the energy consumption of battery-powered devices.
The module achieves this by switching to low power modes when the device is not used for a longer period of time.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_power_manager_start
    :end-before: table_power_manager_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

You can enable the power manager module by selecting the :option:`CONFIG_DESKTOP_POWER_MANAGER_ENABLE` option in the configuration.

This module uses Zephyr's :ref:`zephyr:power_management_api` subsystem.
It depends on :option:`CONFIG_HAS_POWER_STATE_DEEP_SLEEP_1` being enabled and selects :option:`CONFIG_DEVICE_POWER_MANAGEMENT` and :option:`CONFIG_PM_DEEP_SLEEP_STATES`.

Time-out configuration options
==============================

With the :option:`CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT` configuration option, you can set the amount of time after which the application will go to the low power mode.
By default, the time-out is set to 120 seconds.

The :option:`CONFIG_DESKTOP_POWER_MANAGER_ERROR_TIMEOUT` sets the amount of time after which the device is turned off upon an internal error.

Optional boolean for keeping the system on
==========================================

The :option:`CONFIG_DESKTOP_POWER_MANAGER_STAY_ON` lets the system stay on also when there are no active connections.

For more information about configuration options, check the help in the configuration tool.

Implementation details
**********************

The power manager is started when the :ref:`nrf_desktop_main` is ready (which is reported using ``module_state_event``).

When started, it can do the following operations:

* Manage `Power states`_
* Trigger `Switching to low power`_
* Handle `Wake-up scenarios`_ from suspended or off states

Power states
============

The application can be in the following power states:

* `Idle`_
* `Suspended`_
* `Off`_
* `Error`_

The power manager takes control of the overall operating system power state.
See the following section for more details about how the application states converts to the system power state.

Idle
----

In this state, the board is either connected or actively looking for a new connection (advertising).
The peripherals can be turned on, including LEDs.

The application remains in this state indefinitely if the device is connected through USB.
In such case, the operating system will be kept in the ``POWER_STATE_ACTIVE`` state.

If the device is not connected through USB, the module counts time elapsed since the last user interaction (that is, since the last HID report sent from the device).
On timeout, the power manager module sets the application to either the suspended or the off state.

Suspended
---------

Upon power-down time-out, the power manager will switch the application to the suspended state if one of the following conditions is met:

* The device is connected to a remote peer.
* The option :option:`CONFIG_DESKTOP_POWER_MANAGER_STAY_ON` is selected.

The other modules of the application, if applicable, will turn off the peripherals or switch them to standby to conserve power.
The operating system will be kept in the ``POWER_STATE_ACTIVE`` state.

It is assumed that the operating system will conserve power by setting the CPU state to idle whenever possible.
The established connection is maintained.

Off
---

Upon power-down time-out, the power manager will switch the application to the deep sleep mode if the following conditions are met:

* The device is disconnected.
* The option :option:`CONFIG_DESKTOP_POWER_MANAGER_STAY_ON` is disabled.

If applicable, the other modules of the application will turn off the peripherals or switch them to standby to conserve power.
The operating system will be switched to the ``POWER_STATE_DEEP_SLEEP_1`` state.
The devices will be suspended and the CPU will be switched to the deep sleep (off) mode.

A device reboot is required to exit this state.

Error
-----

The power manager module checks if any application modules reported an error condition.

When any application module switches to the error state (that is, broadcasts ``MODULE_STATE_ERROR`` through ``module_state_event``), the power manager will put the application into the error state.
Then, after the amount of time defined by :option:`CONFIG_DESKTOP_POWER_MANAGER_ERROR_TIMEOUT`, it will put the application to off state.
During this period, the error condition can be reported to the user by other modules (for example, :ref:`nrf_desktop_led_state`).

Switching to low power
======================

When the power manager detects that the application is about to enter the low power state (either suspended or off), it sends a ``power_down_event``.
Other application modules react to this event by changing their configuration to low power, for example by turning off LEDs.

It is possible that some modules will not be ready to switch to the lower power state.
In such case, the module that is not yet ready should consume the ``power_down_event`` and change its internal state, so that it enters the low power state when ready.

After entering the low power state, each module must report this by sending a ``module_state_event``.
The power manager will continue with the low power state change when it gets a notification that the module switched to the low power.

Only after all modules confirmed that they have entered the low power state (by not consuming the ``power_down_event``), the power manager will set the required application's state.

If a disconnection happens while the device is in the suspended state, the power manager will switch the application to the off state.

However, the application can also be configured to keep the system in the suspended state when there are no active connections, instead of switching to the off state.
To select this behavior, use the :option:`CONFIG_DESKTOP_POWER_MANAGER_STAY_ON` configuration option.

Wake-up scenarios
=================

The application can be woken up in the following scenarios:

* `Wake-up from suspended state`_
* `Wake-up from off state`_

Wake-up from suspended state
----------------------------

Any module can trigger the application to switch from the suspended state back to the idle state by submitting a ``wake_up_event``.
This is normally done on some external event, for example upon interaction from the user of the device.

The ``wake_up_event`` is received by the application modules and it switches them back to the normal operation.
The power manager will set the application to the idle state.
It will also restart its power down counter if the device is not connected through USB.

Wake-up from off state
----------------------

In the off state, the CPU is not running and the CPU reboot is required.

Before the application enters the off state, at least one module must configure the peripheral under its control, so that it issues a hardware-related event capable of rebooting the CPU (that is, capable of leaving the CPU off mode).

After the reboot, the application initializes itself again.
