.. _nrf_desktop_settings_loader:

Settings loader module
######################

.. contents::
   :local:
   :depth: 2

Use the settings loader module to trigger loading the data from non-volatile memory.
The Settings loader module is enabled for every nRF Desktop device with Zephyr's :ref:`zephyr:settings_api` enabled.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_settings_loader_start
    :end-before: table_settings_loader_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

nRF Desktop uses the Settings loader module from :ref:`lib_caf` (CAF).
See the :ref:`CAF Settings loader module <caf_settings_loader>` page for implementation details.

.. tip::
   Enabling :kconfig:`CONFIG_CAF_SETTINGS_LOADER_USE_THREAD` is recommended for keyboard reference design (nRF52832 Desktop Keyboard).
   The :ref:`caf_buttons` uses the system workqueue to scan the keyboard matrix.
   Loading the settings in the system workqueue context could block the workqueue and result in missing key presses on system reboot.
