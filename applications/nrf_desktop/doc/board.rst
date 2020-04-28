.. _nrf_desktop_board:

Board module
############

The ``board`` module is used to ensure the proper state of GPIO pins that are not configured by other modules in the application.
Ensuring the proper state is done by setting the state of the mentioned pins on the system start and wakeup, and when the system enters the low-power mode.

Module Events
*************

+----------------+-------------+--------------+-----------------+------------------+
| Source Module  | Input Event | This Module  | Output Event    | Sink Module      |
+================+=============+==============+=================+==================+
|                |             | ``board``    |                 |                  |
+----------------+-------------+--------------+-----------------+------------------+

Configuration
*************

The module uses the Zephyr :ref:`zephyr:gpio` driver to set the pin state.
For this reason, you should set :option:`CONFIG_GPIO` option.

For every configuration, you must define the ``port_state_def.h`` file in the board-specific directory in the application configuration folder.

The ``port_state_def.h`` file defines the states set to the GPIO ports by the following arrays:

* ``port_state_on`` - state set on the system start and wakeup.
* ``port_state_off`` - state set when the system enters the low-power mode.

Every :c:type:`struct port_state` refers to a single GPIO port and contains the following information:

* :cpp:member:`name` - GPIO device name (obtained from :ref:`devicetree <zephyr:devicetree>`, for example with ``DT_LABEL(DT_NODELABEL(gpio0))``).
* :cpp:member:`ps` - pointer to the array of :c:type:`struct pin_state`.
* :cpp:member:`ps_count` - size of the `ps` array.

Every :c:type:`struct pin state` defines the state of a single GPIO pin:

* :cpp:member:`pin` - pin number.
* :cpp:member:`val` - value set for the pin.
