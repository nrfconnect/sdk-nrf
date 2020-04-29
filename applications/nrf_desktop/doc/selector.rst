.. _nrf_desktop_selector:

Selector module
###############

The ``selector`` module is used to send ``selector_event`` that informs about the current selector state.
The module has one implementation, which uses hardware selectors (``selector_hw.c``).

Module Events
*************

+----------------+-------------+-----------------+--------------------+------------------+
| Source Module  | Input Event | This Module     | Output Event       | Sink Module      |
+================+=============+=================+====================+==================+
|                |             | ``selector``    | ``selector_event`` | ``ble_bond``     |
+----------------+-------------+-----------------+--------------------+------------------+

Configuration
*************

The module implemented in ``selector_hw.c`` uses the Zephyr :ref:`zephyr:gpio_api` driver to check the state of hardware selectors.
For this reason, you should set :option:`CONFIG_GPIO` option.

Set ``CONFIG_DESKTOP_SELECTOR_HW_ENABLE`` option to enable the module.
The configuration for this module is an array of :c:type:`struct selector_config` pointers.
The array is written in the ``selector_hw_def.h`` file located in the board-specific directory in the application configuration folder.

For every hardware selector, define the following parameters:
* :cpp:member:`id` - the ID of the hardware selector
* :cpp:member:`pins` - pointer to the array of :c:type:`struct gpio_pin`
* :cpp:member:`pins_size` - size of the array of :c:type:`struct gpio_pin`

.. warning::
  Each source of ``selector_event`` must have a unique ID to properly distinguish events from different sources.

Implementation details
**********************

The ``selector_event`` that the module sends to inform about the current hardware selector state is sent on the following occasions:
* on system start or wakeup (for every defined selector)
* on selector state change when the application is running (only for the selector that changed state)

When the application goes to sleep, selectors are not informing about state change (interrupts are disabled).

If a selector is placed between states, it is in unknown state. ``selector_event`` is not sent.

Recording of selector state changes is implemented using GPIO callbacks (:c:type:`struct gpio_callback`) and work (:c:type:`struct k_delayed_work`).
Each state change triggers an interrupt (GPIO interrupt for pin level high).
Callback of an interrupt submits work, which sends ``selector_event``.
