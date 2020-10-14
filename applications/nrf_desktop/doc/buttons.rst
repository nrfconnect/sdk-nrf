.. _nrf_desktop_buttons:

Buttons module
##############

.. contents::
   :local:
   :depth: 2

The buttons module is responsible for generating events related to key presses.
The source of events are changes to GPIO pins.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_buttons_start
    :end-before: table_buttons_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module is enabled with the :option:`CONFIG_DESKTOP_BUTTONS_ENABLE` define.

The module can handle both matrix keyboard and buttons connected directly to GPIO pins.

When defining how buttons are connected, two arrays are used, both defined in the :file:`buttons_def.h` file that is located in the board-specific directory in the application configuration directory:

* The first array contains pins associated with matrix rows.
* The second array contains pins associated with columns, and it can be left empty (buttons will be assumed to be directly connected to row pins, one button per pin).

By default, a button press is indicated by the pin switch from the low to the high state.
You can change this with :option:`CONFIG_DESKTOP_BUTTONS_POLARITY_INVERSED`, which will cause the application to react to an opposite pin change (from the high to the low state).

Implementation details
**********************

The module can be in the following states:

* ``STATE_IDLE``
* ``STATE_ACTIVE``
* ``STATE_SCANNING``
* ``STATE_SUSPENDING``

After initialization, the module starts in ``STATE_ACTIVE``.
In this state, the module enables the GPIO interrupts and waits for the pin state to change.
When a button is pressed, the module switches to ``STATE_SCANNING``.

When the switch occurs, the module submits a work with a delay set to :option:`CONFIG_DESKTOP_BUTTONS_DEBOUNCE_INTERVAL`.
The work scans the keyboard matrix for changes to button states and sends the related events.
If the button is kept pressed while the scanning is performed, the work will be re-submitted with a delay set to :option:`CONFIG_DESKTOP_BUTTONS_SCAN_INTERVAL`.
If no button is pressed, the module switches back to ``STATE_ACTIVE``.

When the system enters the low-power state, the ``buttons`` module goes to ``STATE_IDLE``, in which it waits for GPIO interrupts that indicate a change to button states.
When an interrupt is triggered, the module will issue a system wake-up event.

If a request for power off comes while the button is pressed, the module switches to ``STATE_SUSPENDING`` and remains in this state until no button is pressed.
Then, it will switch to ``STATE_IDLE``.
