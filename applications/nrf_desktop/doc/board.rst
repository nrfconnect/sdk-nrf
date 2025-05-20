.. _nrf_desktop_board:

Board module
############

.. contents::
   :local:
   :depth: 2

The board module is used to ensure the proper state of GPIO pins that are not configured by other modules in the application.
Ensuring the proper state is done by setting the state of the mentioned pins on the system start and wakeup, and when the system enters the low-power mode.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_board_start
    :end-before: table_board_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module uses Zephyr's :ref:`zephyr:gpio_api` driver to set the pin state.
For this reason, set the :kconfig:option:`CONFIG_GPIO` option.

For every configuration, you must define the :file:`port_state_def.h` file in the board-specific directory in the application configuration directory.

The :file:`port_state_def.h` file defines the states set to the GPIO ports by the following arrays:

* ``port_state_on`` - State set on the system start and wakeup.
* ``port_state_off`` - State set when the system enters the low-power mode.

Every :c:struct:`port_state` refers to a single GPIO port and contains the following information:

* :c:member:`port_state.name` - GPIO device name (obtained from :ref:`devicetree <zephyr:dt-guide>`, for example with ``DT_LABEL(DT_NODELABEL(gpio0))``).
* :c:member:`port_state.ps` - Pointer to the array of :c:struct:`pin_state`.
* :c:member:`port_state.ps_count` - Size of the `ps` array.

Every :c:struct:`pin_state` defines the state of a single GPIO pin:

* :c:member:`pin_state.pin` - Pin number.
* :c:member:`pin_state.val` - Value set for the pin.

You can use the :ref:`CONFIG_DESKTOP_BOARD_PM_EVENTS <config_desktop_app_options>` Kconfig option to enable or disable handling of the power management events, such as :c:struct:`power_down_event` and :c:struct:`wake_up_event`.
The option is enabled by default and depends on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.
