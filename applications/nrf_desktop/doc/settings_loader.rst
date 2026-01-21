.. _nrf_desktop_settings_loader:

Settings loader module
######################

.. contents::
   :local:
   :depth: 2

Use the settings loader module to trigger loading the data from non-volatile memory.
The settings loader module is by default enabled, along with all of the required dependencies for every nRF Desktop device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_settings_loader_start
    :end-before: table_settings_loader_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

nRF Desktop uses the settings loader module from the :ref:`lib_caf` (CAF).
The :option:`CONFIG_DESKTOP_SETTINGS_LOADER` Kconfig option selects :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER` and aligns the module configuration to the application requirements.
The :option:`CONFIG_DESKTOP_SETTINGS_LOADER` Kconfig option is implied by the :option:`CONFIG_DESKTOP_COMMON_MODULES` Kconfig option.
The :option:`CONFIG_DESKTOP_COMMON_MODULES` option is enabled by default and is not user-assignable.

For details on the default configuration alignment, see the following sections.

Settings backend
================

By default, the nRF Desktop application, depending on the non-volatile memory technology used by the device, uses one of the following settings backends:

* :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` - Used for the devices with non-volatile memory that do not require explicit erase (MRAM, RRAM).
* :ref:`Non-Volatile Storage (NVS) <zephyr:nvs_api>` - Used for the devices with non-volatile memory that require explicit erase (FLASH).

The storage partition is located in the internal non-volatile memory.

Settings load in a separate thread
==================================

Enabling the :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_USE_THREAD` option is recommended for the keyboard reference design (nRF52832 Desktop Keyboard).
The :ref:`caf_buttons` uses the system workqueue to scan the keyboard matrix.
Loading the settings in the system workqueue context could block the workqueue and result in missing key presses on system reboot.

Implementation details
**********************

See the :ref:`CAF Settings loader module <caf_settings_loader>` page for implementation details.
