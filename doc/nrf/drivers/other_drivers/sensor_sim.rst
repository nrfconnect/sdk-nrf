.. _sensor_sim:

Simulated sensor driver
#######################

.. contents::
   :local:
   :depth: 2

The simulated sensor driver implements a simulated sensor that is compatible with Zephyr's :ref:`zephyr:sensor`.
The sensor provides readouts for predefined set of sensor channels and supports sensor triggers.

Configuration
*************

You can define instances of the sensor simulator in devicetree as shown in the following code snippet:

.. code-block:: devicetree

        sensor-sim {
                compatible = "nordic,sensor-sim";
                acc-signal = "toggle";
        };

The sensor driver will automatically be built if ``CONFIG_SENSOR=y`` and any of its instances in devicetree have status set to ``okay``.

Configuration of generated readouts
===================================

The algorithms used to generate simulated sensor readouts are configurable.
The following sensor channels and configuration options are available:

* Ambient temperature (:c:enum:`SENSOR_CHAN_AMBIENT_TEMP`) - The value is generated as the sum of the value of the devicetree property ``base-temperature`` and a pseudo-random number between ``-1`` and ``1``.
* Humidity (:c:enum:`SENSOR_CHAN_HUMIDITY`) - The value is generated as the sum of the value of the devicetree property ``base-humidity`` and a pseudo-random number between ``-1`` and ``1``.
* Pressure (:c:enum:`SENSOR_CHAN_PRESS`) - The value is generated as the sum of the value of the devicetree property ``base-pressure`` and a pseudo-random number between ``-1`` and ``1``.
* Acceleration in X, Y, and Z axes (:c:enum:`SENSOR_CHAN_ACCEL_X`, :c:enum:`SENSOR_CHAN_ACCEL_Y`, :c:enum:`SENSOR_CHAN_ACCEL_Z`, for each axis respectively, and :c:enum:`SENSOR_CHAN_ACCEL_XYZ` for all axes at once).
  The acceleration is generated depending on the selected devicetree ``acc-signal`` property:

  * ``toggle`` - With this choice, the acceleration is toggled on fetch between statically defined values.
  * ``wave`` - With this choice, the acceleration is generated as value of a periodic wave signal.
    The wave signal value is generated using the :ref:`wave_gen` library.
    You can use the :c:func:`sensor_sim_set_wave_param` function to configure generated waves.
    By default, the function generates a sine wave.

Configuration of sensor triggers
================================

Use :kconfig:option:`CONFIG_SENSOR_SIM_TRIGGER` to enable the sensor trigger.
The simulated sensor supports the :c:enum:`SENSOR_TRIG_DATA_READY` trigger.

You can configure the event that generates the trigger in devicetree by using the ``trigger-gpios`` or ``trigger-timeout`` options.

For both triggers, the handler function is called by a dedicated thread.
The thread has the following configuration options:

* :kconfig:option:`CONFIG_SENSOR_SIM_THREAD_PRIORITY` - This Kconfig option defines the priority.
* :kconfig:option:`CONFIG_SENSOR_SIM_THREAD_STACK_SIZE` - This Kconfig option defines the stack size.

API documentation
*****************

| Header file: :file:`include/drivers/sensor_sim.h`
| Source file: :file:`drivers/sensor/sensor_sim/sensor_sim.c`

.. doxygengroup:: sensor_sim
