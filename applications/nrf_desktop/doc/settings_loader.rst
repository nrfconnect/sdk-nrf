.. _nrf_desktop_settings_loader:

Settings loader module
######################

Use the Settings loader module to trigger loading the data from non-volatile memory.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_settings_loader_start
    :end-before: table_settings_loader_end

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

The Settings loader module is enabled for every nRF Desktop device with :ref:`zephyr:settings_api` enabled.
The :ref:`zephyr:settings_api` subsystem is enabled with the :option:`CONFIG_SETTINGS` Kconfig option.

The Zephyr Bluetooth stack does not load the :ref:`zephyr:settings_api` data on its own.
Zephyr assumes that the application will call :cpp:func:`settings_load` after completing all necessary initialization.
This function is called on the ``settings_loader`` module initialization.
Make sure that all settings handlers are registered and :cpp:func:`bt_enable` is called before the ``settings_loader`` module is initialized.

Settings are by default loaded in the system workqueue context.
This blocks the workqueue until the operation is finished.
You can set the ``CONFIG_DESKTOP_SETTINGS_LOADER_USE_THREAD`` Kconfig option to load the settings in a separate thread in the background instead of using the system workqueue for that purpose.
This will prevent blocking the system workqueue, but it requires creating an additional thread.
The stack size for the background thread is defined as ``CONFIG_DESKTOP_SETTINGS_LOAD_THREAD_STACK_SIZE``.

.. tip::
   Using separate thread is recommended for nRF Desktop keyboards.
   The :ref:`nrf_desktop_buttons` module uses the system workqueue to scan the keyboard matrix.
   Loading the settings in the system workqueue context could block the workqueue and result in missing key presses on system reboot.
   For this reason, ``CONFIG_DESKTOP_SETTINGS_LOADER_USE_THREAD`` is enabled for keyboard reference design (nRF52832 Desktop Keyboard)
