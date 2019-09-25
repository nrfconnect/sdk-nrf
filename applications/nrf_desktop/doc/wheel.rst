.. _wheel:

wheel module
############

The ``wheel`` module is responsible for generating events related to the rotation of
the mouse wheel.

Module Events
*************

+----------------+-------------+--------------+-----------------+------------------+
| Source Module  | Input Event | This Module  | Output Event    | Sink Module      |
+================+=============+==============+=================+==================+
|                |             | :ref:`wheel` | ``wheel_event`` | :ref:`hid_state` |
+----------------+-------------+--------------+-----------------+------------------+

Configuration
*************

The module is enabled by the ``CONFIG_DESKTOP_WHEEL_ENABLE`` define.

For detecting rotation, ``wheel`` uses Zephyr's QDEC driver. The module can be enabled
only when QDEC is configured in DTS (:option:`CONFIG_QDEC_NRFX` is set).

The QDEC DTS configuration specifies how many steps are done during one full angle.
The sensor reports the rotation data in angle degrees.
``CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER`` (a ``wheel`` configuration option) then converts the
angle value into a value that is passed via the ``wheel_event`` to destination module.

For example, configuring QDEC with 24 steps means that for each step the sensor will
report a rotation of 15 degrees. For HID to see this rotation as increment of
one, ``CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER`` should be set to 15.

Implementation details
**********************

The ``wheel`` module can be in the following states:
    * ``STATE_DISABLED``
    * ``STATE_ACTIVE_IDLE``
    * ``STATE_ACTIVE``
    * ``STATE_SUSPENDED``

When in ``STATE_ACTIVE``, ``wheel`` enables the QDEC driver and waits for callback
that indicates rotation. The QDEC driver may consume power during its operation. If no
rotation is detected after the time specified in
``CONFIG_DESKTOP_WHEEL_SENSOR_IDLE_TIMEOUT``, ``wheel`` switches
to ``STATE_ACTIVE_IDLE``, in which QDEC is disabled. In this state, the rotation is
detected using GPIO interrupts. When the rotation is detected, ``wheel`` switches
back to ``STATE_ACTIVE``.

When the system enters the low power state, ``wheel`` goes to ``STATE_SUSPENDED``.
In this state, the rotation is detected using the GPIO interrupts. The module issues
a system wakeup event on wheel movement.
