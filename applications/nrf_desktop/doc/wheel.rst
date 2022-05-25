.. _nrf_desktop_wheel:

Wheel module
############

.. contents::
   :local:
   :depth: 2

The wheel module is responsible for generating events related to the rotation of the mouse wheel.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_wheel_start
    :end-before: table_wheel_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Enable the module with the :ref:`CONFIG_DESKTOP_WHEEL_ENABLE <config_desktop_app_options>` Kconfig option.

For detecting rotation, the wheel module uses Zephyr's QDEC driver.
The module can be enabled only when QDEC is configured in DTS (that is, :kconfig:option:`CONFIG_QDEC_NRFX` is set).

The QDEC DTS configuration specifies how many steps are done during one full angle.
The sensor reports the rotation data in angle degrees.
Then, :ref:`CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER <config_desktop_app_options>` (a wheel configuration option) converts the angle value into a value that is passed with a ``wheel_event`` to the destination module.

For example, configuring QDEC with 24 steps means that for each step the sensor will report a rotation of 15 degrees.
For HID to see this rotation as increment of one, :ref:`CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER <config_desktop_app_options>` should be set to 15.

Implementation details
**********************

The wheel module can be in the following states:

* ``STATE_DISABLED``
* ``STATE_ACTIVE_IDLE``
* ``STATE_ACTIVE``
* ``STATE_SUSPENDED``

When in ``STATE_ACTIVE``, the module enables the QDEC driver and waits for callback that indicates rotation.
The QDEC driver may consume power during its operation.

If no rotation is detected after the time specified in :ref:`CONFIG_DESKTOP_WHEEL_SENSOR_IDLE_TIMEOUT <config_desktop_app_options>`, the wheel module switches to ``STATE_ACTIVE_IDLE``, in which QDEC is disabled.
In this state, the rotation is detected using GPIO interrupts.
When the rotation is detected, the module switches back to ``STATE_ACTIVE``.

When the system enters the low power state, the wheel module goes to ``STATE_SUSPENDED``.
In this state, the rotation is detected using the GPIO interrupts.
The module issues a system wake-up event on wheel movement.
