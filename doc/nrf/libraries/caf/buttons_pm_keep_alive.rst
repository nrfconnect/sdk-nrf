.. _caf_buttons_pm_keep_alive:

CAF: Power manager keep alive module for buttons
################################################

.. contents::
   :local:
   :depth: 2

The |buttons_pm_keep_alive| of the :ref:`lib_caf` (CAF) is responsible for generating :c:struct:`keep_alive_event` on any configured button press.
The :c:struct:`keep_alive_event` is used by the :ref:`CAF power manager module <caf_power_manager>` to reset the power-down counter when it is managing the overall operating system power state.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :kconfig:option:`CONFIG_CAF_BUTTONS_PM_KEEP_ALIVE` - This option enables the |buttons_pm_keep_alive|.
  It is enabled by default if :kconfig:option:`CONFIG_CAF_POWER_MANAGER` and :kconfig:option:`CONFIG_CAF_BUTTONS` are enabled.
* :kconfig:option:`CONFIG_CAF_BUTTONS` - This option enables the :ref:`CAF buttons module <caf_buttons>`.
* :kconfig:option:`CONFIG_CAF_POWER_MANAGER` - This option enables the :ref:`CAF power manager module <caf_power_manager>` and it is required by :kconfig:option:`CONFIG_CAF_BUTTONS_PM_KEEP_ALIVE`.

Implementation details
**********************

The module reacts to :c:struct:`button_event` and generates :c:struct:`keep_alive_event` for the CAF power manager if any configured button is pressed.
See :ref:`CAF buttons module <caf_buttons>` documentation page for information about how to configure buttons and the :ref:`CAF power manager module <caf_power_manager>` documentation page for more information about the power state management.
