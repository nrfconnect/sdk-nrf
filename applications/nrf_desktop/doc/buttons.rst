.. _buttons:

buttons module
##############

The ``buttons`` module is responsible for generating events related to key presses.
The source of events are changes to GPIO pins.

Module Events
*************

+----------------+-------------+----------------+------------------+-------------------------+
| Source Module  | Input Event | This Module    | Output Event     | Sink Module             |
+================+=============+================+==================+=========================+
|                |             | :ref:`buttons` | ``button_event`` | :ref:`hid_state`        |
+                +             +                +                  +-------------------------+
|                |             |                |                  | :ref:`fn_keys`          |
+                +             +                +                  +-------------------------+
|                |             |                |                  | :ref:`click_detector`   |
+----------------+-------------+----------------+------------------+-------------------------+


Configuration
*************

The module is enabled with the ``CONFIG_DESKTOP_BUTTONS_ENABLE`` define.

The module can handle both matrix keyboard and buttons connected directly to GPIO pins.

When defining how buttons are connected, two arrays are used, both defined in the ``buttons_def.h``
file that is located in the board-specific directory in the application configuration folder:
- The first array contains pins associated with matrix rows.
- The second array contains pins associated with columns - it can be left empty
(buttons will be assumed to be directly connected to row pins, one button per pin).

By default, a button press is indicated by the pin switch from the low
to the high state. You can change this with ``CONFIG_DESKTOP_BUTTONS_POLARITY_INVERSED``,
which will cause the application to react to an opposite pin change (from the high to the low state).

Implementation details
**********************

``buttons`` can be in the following states:
    * ``STATE_IDLE``
    * ``STATE_ACTIVE``
    * ``STATE_SCANNING``
    * ``STATE_SUSPENDING``

After initialization, the module starts in ``STATE_ACTIVE``. In this state,
the module enables the GPIO interrupts and waits for the pin state to
When a button is pressed, the module switches to ``STATE_SCANNING``.

When the switch occurs, the module submits a work with a delay set to
``CONFIG_DESKTOP_BUTTONS_DEBOUNCE_INTERVAL``. The work scans the
keyboard matrix noting changes to button states and sending related events.
If the button is kept pressed while the scanning is performed, the work will be
re-submitted with delay set to ``CONFIG_DESKTOP_BUTTONS_SCAN_INTERVAL``.
If no button is pressed, the module switches back to ``STATE_ACTIVE``.

When the system enters the low-power state, the ``buttons`` module goes to ``STATE_IDLE``, in which
it waits for GPIO interrupts that indicate a change to button states. When an interrupt
is triggered, the module will issue a system wakeup event.

If a request for power off comes while the button is pressed, the module switches to
``STATE_SUSPENDING`` and remains in this state until no button is pressed. Then, it
will switch to ``STATE_IDLE``.
