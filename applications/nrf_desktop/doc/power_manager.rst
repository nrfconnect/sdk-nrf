.. _nrf_desktop_power_manager:

Power manager module
####################

.. contents::
   :local:
   :depth: 2

Use the power manager module to reduce the energy consumption of battery-powered devices.
The module achieves this by switching to low power modes when the device is not used for a longer period of time.
The power manager module is by default enabled, along with all of the required dependencies for every nRF Desktop device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_power_manager_start
    :end-before: table_power_manager_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

nRF Desktop uses the power manager module from the :ref:`lib_caf` (CAF).
The :option:`CONFIG_DESKTOP_POWER_MANAGER` Kconfig option selects :kconfig:option:`CONFIG_CAF_POWER_MANAGER` and aligns the default module configuration to the application requirements.
The :option:`CONFIG_DESKTOP_POWER_MANAGER` Kconfig option is implied by the :option:`CONFIG_DESKTOP_COMMON_MODULES` Kconfig option.
The :option:`CONFIG_DESKTOP_COMMON_MODULES` option is enabled by default and is not user-assignable.

Additionally, the :kconfig:option:`CONFIG_CAF_POWER_MANAGER_STAY_ON` option is automatically enabled for nRF Desktop dongles (:option:`CONFIG_DESKTOP_ROLE_HID_DONGLE`).

Implementation details
**********************

See the :ref:`CAF power manager module <caf_power_manager>` page for implementation details.
