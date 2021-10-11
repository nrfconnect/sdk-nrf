.. _sensor_sim:

Simulated sensor driver
#######################

.. contents::
   :local:
   :depth: 2

The simulated sensor driver implements a simulated sensor that is compatible with Zephyr's :ref:`zephyr:sensor_api`.
The sensor provides readouts for predefined set of sensor channels and supports sensor triggers.

Configuration
*************

You can enable the driver using the :kconfig:`CONFIG_SENSOR_SIM` Kconfig option.

To configure the device name used by the simulated sensor device, use the :kconfig:`CONFIG_SENSOR_SIM_DEV_NAME` Kconfig option.
The default device name is ``SENSOR_SIM``.

Configuration of generated readouts
===================================

The algorithms used to generate simulated sensor readouts are configurable.
The following sensor channels and configuration options are available:

* Ambient temperature (:c:enum:`SENSOR_CHAN_AMBIENT_TEMP`) - The value is generated as the sum of the value of the :kconfig:`CONFIG_SENSOR_SIM_BASE_TEMPERATURE` Kconfig option and a pseudo-random number between ``-1`` and ``1``.
* Humidity (:c:enum:`SENSOR_CHAN_HUMIDITY`) - The value is generated as the sum of the value of the :kconfig:`CONFIG_SENSOR_SIM_BASE_HUMIDITY` Kconfig option and a pseudo-random number between ``-1`` and ``1``.
* Pressure (:c:enum:`SENSOR_CHAN_PRESS`) - The value is generated as the sum of the value of the :kconfig:`CONFIG_SENSOR_SIM_BASE_PRESSURE` Kconfig option and a pseudo-random number between ``-1`` and ``1``.
* Acceleration in X, Y, and Z axes (:c:enum:`SENSOR_CHAN_ACCEL_X`, :c:enum:`SENSOR_CHAN_ACCEL_Y`, :c:enum:`SENSOR_CHAN_ACCEL_Z`, for each axis respectively, and :c:enum:`SENSOR_CHAN_ACCEL_XYZ` for all axes at once) - The acceleration is generated depending on the selected Kconfig option:

  * :kconfig:`CONFIG_SENSOR_SIM_ACCEL_TOGGLE` - With this option, the acceleration is toggled on fetch between statically defined values.
  * :kconfig:`CONFIG_SENSOR_SIM_ACCEL_WAVE` - With this option, the acceleration is generated as value of a periodic wave signal.
    The wave signal value is generated using the :ref:`wave_gen` library.
    You can use the :c:func:`sensor_sim_set_wave_param` function to configure generated waves.
    By default, the function generates a sine wave.

Configuration of sensor triggers
================================

Use :kconfig:`CONFIG_SENSOR_SIM_TRIGGER` to enable the sensor trigger.
The simulated sensor supports the :c:enum:`SENSOR_TRIG_DATA_READY` trigger.

You can configure the event that generates the trigger using one of the following Kconfig options:

* :kconfig:`CONFIG_SENSOR_SIM_TRIGGER_USE_TIMEOUT` - The trigger is generated periodically on timeout of the period defined in the :kconfig:`CONFIG_SENSOR_SIM_TRIGGER_TIMEOUT_MSEC` Kconfig option.
  By default, the trigger is generated every 1 second.
* :kconfig:`CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON` - The trigger is generated when the **Button 1** is pressed on the compatible development kit.
  The simulated sensor driver uses :ref:`zephyr:gpio_api` to access the GPIO pin.

For both triggers, the handler function is called by a dedicated thread.
The thread has the following configuration options:

* :kconfig:`CONFIG_SENSOR_SIM_THREAD_PRIORITY` - This Kconfig option defines the priority.
* :kconfig:`CONFIG_SENSOR_SIM_THREAD_STACK_SIZE` - This Kconfig option defines the stack size.

API documentation
*****************

| Header file: :file:`include/drivers/sensor_sim.h`
| Source file: :file:`drivers/sensor/sensor_sim/sensor_sim.c`

.. doxygengroup:: sensor_sim
   :project: nrf
   :members:
