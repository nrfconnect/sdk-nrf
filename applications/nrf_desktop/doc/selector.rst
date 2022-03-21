.. _nrf_desktop_selector:

Selector module
###############

.. contents::
   :local:
   :depth: 2

The selector module is used to send ``selector_event`` that informs about the current selector state.
The module has one implementation, which uses hardware selectors (:file:`selector_hw.c`).

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_selector_start
    :end-before: table_selector_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module implemented in :file:`selector_hw.c` uses the Zephyr :ref:`zephyr:gpio_api` driver to check the state of hardware selectors.
For this reason, you should set :kconfig:option:`CONFIG_GPIO` option.

Set :ref:`CONFIG_DESKTOP_SELECTOR_HW_ENABLE <config_desktop_app_options>` option to enable the module.
The configuration for this module is an array of :c:struct:`selector_config` pointers.
The array is written in the :file:`selector_hw_def.h` file located in the board-specific directory in the application configuration directory.

For every hardware selector, define the following parameters:

* :c:member:`selector_config.id` - ID of the hardware selector.
* :c:member:`selector_config.pins` - Pointer to the array of :c:struct:`gpio_pin`.
* :c:member:`selector_config.pins_size` - Size of the array of :c:struct:`gpio_pin`.

.. note::
    Each source of ``selector_event`` must have a unique ID to properly distinguish events from different sources.

Implementation details
**********************

The ``selector_event`` that the module sends to inform about the current hardware selector state is sent on the following occasions:

* System start or wakeup (for every defined selector).
* Selector state change when the application is running (only for the selector that changed state).

When the application goes to sleep, selectors are not informing about the state change (interrupts are disabled).

If a selector is placed between states, it is in unknown state and ``selector_event`` is not sent.

Recording of selector state changes is implemented using GPIO callbacks (:c:struct:`gpio_callback`) and work (:c:struct:`k_work_delayable`).
Each state change triggers an interrupt (GPIO interrupt for pin level high).
Callback of an interrupt submits work, which sends ``selector_event``.
