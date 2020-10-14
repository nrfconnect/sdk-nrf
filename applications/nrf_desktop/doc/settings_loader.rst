.. _nrf_desktop_settings_loader:

Settings loader module
######################

.. contents::
   :local:
   :depth: 2

Use the settings loader module to trigger loading the data from non-volatile memory.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_settings_loader_start
    :end-before: table_settings_loader_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The settings loader module is enabled for every nRF Desktop device with Zephyr's :ref:`zephyr:settings_api` enabled.
The :ref:`zephyr:settings_api` subsystem is enabled with the :option:`CONFIG_SETTINGS` Kconfig option.

Zephyr's Bluetooth stack does not load the :ref:`zephyr:settings_api` data on its own.
Zephyr assumes that the application will call :c:func:`settings_load` after completing all necessary initialization.
This function is called on the settings loader module initialization.

.. note::
    Make sure that all settings handlers are registered and :c:func:`bt_enable` is called before the settings loader module module is initialized.

Settings are by default loaded in the system workqueue context.
This blocks the workqueue until the operation is finished.
You can set the :option:`CONFIG_DESKTOP_SETTINGS_LOADER_USE_THREAD` Kconfig option to load the settings in a separate thread in the background instead of using the system workqueue for that purpose.
This will prevent blocking the system workqueue, but it requires creating an additional thread.
The stack size for the background thread is defined as :option:`CONFIG_DESKTOP_SETTINGS_LOADER_THREAD_STACK_SIZE`.

.. tip::
   Using separate thread is recommended for nRF Desktop keyboards.
   The :ref:`nrf_desktop_buttons` uses the system workqueue to scan the keyboard matrix.
   Loading the settings in the system workqueue context could block the workqueue and result in missing key presses on system reboot.
   For this reason, :option:`CONFIG_DESKTOP_SETTINGS_LOADER_USE_THREAD` is enabled for keyboard reference design (nRF52832 Desktop Keyboard)
