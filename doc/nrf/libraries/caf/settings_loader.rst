.. _caf_settings_loader:

CAF: Settings loader module
###########################

.. contents::
   :local:
   :depth: 2

The |settings_loader| of the :ref:`lib_caf` (CAF) is a simple, stateless module responsible for calling the :c:func:`settings_load` function.
If any of the application modules relies on settings, this module ensures that the data stored in non-volatile memory is read after completing all necessary initialization.

Configuration
*************

The following Kconfig options are required:

* :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER` - This option enables the |settings_loader|.
* :kconfig:option:`CONFIG_SETTINGS` - This option enables Zephyr's :ref:`zephyr:settings_api`.

The following Kconfig options are also available for the module:

* :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_DEF_PATH`
* :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_USE_THREAD`
* :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_THREAD_STACK_SIZE`

To use the module, you must complete the following steps:

1. Enable the :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER` and :kconfig:option:`CONFIG_SETTINGS` Kconfig options.
#. Add the configuration file that implements the function :c:func:`get_req_modules`, which sets bits of modules that are required to be initialized before settings are loaded.
   For example, the file content could look like follows:

   .. code-block:: c

      #include <caf/events/module_state_event.h>

      static inline void get_req_modules(struct module_flags *mf)
      {
              module_flags_set_bit(mf, MODULE_IDX(main));
      #if CONFIG_CAF_BLE_ADV
              module_flags_set_bit(mf, MODULE_IDX(ble_adv));
      #endif
      };

This function is called on the settings loader module initialization.
After each of modules that sets bit in :c:func:`get_req_modules` is initialized, the |settings_loader| calls :c:func:`settings_load` function and starts loading all the settings from non-volatile memory.

File system as settings backend
===============================

If the settings backend is a file system (set with the :kconfig:option:`CONFIG_SETTINGS_FS` Kconfig option), make sure that the application mounts the file system before the Zephyr settings subsystem is initialized.
The CAF settings loader module calls the :c:func:`settings_subsys_init` initialization function during the system boot with the ``APPLICATION`` level and the initialization priority set by the :kconfig:option:`CONFIG_APPLICATION_INIT_PRIORITY` Kconfig option.

Implementation details
**********************

Getting the required modules is wrapped into the :c:func:`get_req_modules` function due to implementation limitations.

Settings are loaded in the :ref:`app_event_manager` handler, which by default is invoked from a system workqueue context.
This blocks the workqueue until the operation is finished.
You can set the :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_USE_THREAD` Kconfig option to load the settings in a separate thread in the background instead of using the system workqueue for that purpose.
This prevents blocking the system workqueue, but it requires creating an additional thread.
The stack size for the background thread is defined in the :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_THREAD_STACK_SIZE` Kconfig option.
