.. _nrf_desktop_motion:

Motion module
#############

.. contents::
   :local:
   :depth: 2

The motion module is responsible for generating events related to movement.
The movement can be detected using the motion sensor, but it can also be sourced from GPIO pins or simulated.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_motion_start
    :end-before: table_motion_end

.. note::
    |nrf_desktop_module_event_note|

.. _nrf_desktop_motion_configuration:

Configuration
*************

The motion module selects the source of movement based on the following configuration options:

* :ref:`CONFIG_DESKTOP_MOTION_NONE <config_desktop_app_options>` - Module is disabled.
* :ref:`CONFIG_DESKTOP_MOTION_SENSOR_PMW3360_ENABLE <config_desktop_app_options>` - Movement data is obtained from the gaming-grade ``PMW3360`` motion sensor.
* :ref:`CONFIG_DESKTOP_MOTION_SENSOR_PAW3212_ENABLE <config_desktop_app_options>` - Movement data is obtained from ``PAW3212`` motion sensor.
* :ref:`CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE <config_desktop_app_options>` - Movement data is generated using buttons.
* :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE <config_desktop_app_options>` - Movement data is simulated.
  The movement data generation can be controlled from Zephyr's :ref:`zephyr:shell_api`.

See the following sections for more information.

Depending on the selected configuration option, a different implementation file is used during the build process.

You can use the :ref:`CONFIG_DESKTOP_MOTION_PM_EVENTS <config_desktop_app_options>` Kconfig option to enable or disable handling of the power management events, such as :c:struct:`power_down_event` and :c:struct:`wake_up_event`.
The option is enabled by default and depends on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.
The option is unavailable for the motion module implementation that generates movement data using buttons (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE <config_desktop_app_options>`).
The implementation does not react to the power management events.

Movement data from motion sensors
=================================

Selecting either of the motion sensors (:ref:`CONFIG_DESKTOP_MOTION_SENSOR_PMW3360_ENABLE <config_desktop_app_options>` or :ref:`CONFIG_DESKTOP_MOTION_SENSOR_PAW3212_ENABLE <config_desktop_app_options>`) adds the :file:`src/hw_interface/motion_sensor.c` file to the compilation.

The motion sensor is sampled from the context of a dedicated thread.
The option :ref:`CONFIG_DESKTOP_MOTION_SENSOR_THREAD_STACK_SIZE <config_desktop_app_options>` is used to set the thread's stack size.

The motion sensor default sensitivity and power saving switching times can be set with the following options:

* :ref:`CONFIG_DESKTOP_MOTION_SENSOR_CPI <config_desktop_app_options>` - Default CPI.
* :ref:`CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_TIMEOUT_MS <config_desktop_app_options>` - ``Sleep 1`` mode default switch time.
* :ref:`CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_TIMEOUT_MS <config_desktop_app_options>` - ``Sleep 2`` mode default switch time.
* :ref:`CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_TIMEOUT_MS <config_desktop_app_options>` - ``Sleep 3`` mode default switch time.

For more information, see the sensor documentation and the Kconfig help.

Movement data from buttons
==========================

Selecting the :ref:`CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE <config_desktop_app_options>` option adds the :file:`src/hw_interface/motion_buttons.c` file to the compilation.

The movement data is generated when pressing a button.
The module detects the button presses by relying on the received :c:struct:`button_event`.
Generating motion for every direction is triggered using a separate button.
Key ID (:c:member:`button_event.key_id`) of the button used to generate motion for a given direction can be configured with a dedicated Kconfig option:

* Up (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_UP_KEY_ID <config_desktop_app_options>`)
* Down (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_DOWN_KEY_ID <config_desktop_app_options>`)
* Left (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_LEFT_KEY_ID <config_desktop_app_options>`)
* Right (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_RIGHT_KEY_ID <config_desktop_app_options>`)

Pressing and holding one of the mentioned buttons results in generating data for movement in a given direction.
The :ref:`CONFIG_DESKTOP_MOTION_BUTTONS_MOTION_PER_SEC <config_desktop_app_options>` can be used to control a motion generated per second during a button press.
By default, ``1000`` of motion is generated per second during a button press.

Simulated movement data
=======================

Selecting the :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE <config_desktop_app_options>` option adds the :file:`src/hw_interface/motion_simulated.c` file to the compilation.

The generated movement data traces a predefined octagon path.
As soon as the HID subscriber connects, the motion data is sent to it automatically.
The X and Y coordinates of subsequent vertexes of the octagon are defined as the ``coords`` array in the module's source code.

You can configure the path with the following options:

* :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_EDGE_TIME <config_desktop_app_options>` - Sets how long each edge is traced.
  To speed up calculations, this Kconfig option value must be set to a power of two.
* :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_SCALE_FACTOR <config_desktop_app_options>` - Scales the size of the octagon.
  The Kconfig option's value is used as the ``SCALE`` factor in the ``coords`` array.

Shell integration
-----------------

If the Zephyr shell is enabled (meaning the :kconfig:option:`CONFIG_SHELL` Kconfig option is set), the motion module registers a ``motion_sim`` shell module and links to it two commands:

* ``start`` - Start sending simulated movement data to the HID subscriber.
* ``stop``- Stop sending simulated movement data to the HID subscriber.

If the shell is enabled, motion generation no longer starts automatically when the HID subscriber connects and after system wakeup (when :c:struct:`wake_up_event` is received).
The simulated movement data generation needs to be triggered using a shell command.

Configuration channel
*********************

In a :ref:`configuration <nrf_desktop_motion_configuration>` where either :ref:`CONFIG_DESKTOP_MOTION_SENSOR_PMW3360_ENABLE <config_desktop_app_options>` or :ref:`CONFIG_DESKTOP_MOTION_SENSOR_PAW3212_ENABLE <config_desktop_app_options>` is used, you can configure the module through the :ref:`nrf_desktop_config_channel`.
In these configurations, the module is a configuration channel listener and it provides the following configuration options:

* :c:macro:`OPT_DESCR_MODULE_VARIANT`
    This is a special, read-only option used to provide information about the module variant.
    For the motion sensor, the module variant is a sensor model written in lowercase, for example ``pmw3360`` or ``paw3212``.

    The :ref:`nrf_desktop_config_channel_script` uses the sensor model to identify the descriptions of the configuration options.
    These descriptions are defined in the :file:`nrf/scripts/hid_configurator/modules/module_config.py`.
* ``cpi``
    The motion sensor CPI.
* ``downshift``, ``rest1``, ``rest2``
    These firmware option names correspond to switch times of motion sensor modes, namely the sleep modes of ``PAW3212`` and the rest modes of ``PMW3360``.
    These modes are used by a motion sensor to reduce the power consumption, but they also increase the sensor's response time when a motion is detected after a period of inactivity.

    The :ref:`nrf_desktop_config_channel_script` refers to these mode options using names that depend on the sensor variant:

    * For ``PAW3212``, the mode options are called respectively: ``sleep1_timeout``, ``sleep2_timeout``, and ``sleep3_timeout``.
    * For ``PMW3360``, the mode options are called respectively: ``downshift_run``, ``downshift_rest1``, and ``downshift_rest2``.

Implementation details
**********************

Implementation details depend on the selected implementation.
See the following sections for details specific to a given implementation.
The :ref:`nrf_desktop_motion_report_rate` section is common for all implementations and contains information useful for the HID report rate measurements.

Movement data from motion sensors
=================================

This section describes the motion module implementation for motion sensors.

Motion sensor
-------------

The motion module samples movement data from the motion sensor using the motion sensor driver.

The name of the device linked with the driver changes depending on the selected sensor.
The same is true for the names of sensor configuration options.
The nRF Desktop application aggregates the sensor-specific information and translates them to the application abstracts in the :file:`configuration/common/motion_sensor.h` header file.

Sampling thread
---------------

The motion module uses a dedicated sampling thread to sample data from the motion sensor.
The reason for using the sampling thread is the long time required for data to be ready.
Using a dedicated thread also simplifies the module because the interaction with the sensor becomes sequential.
The sampling thread priority is set to 0 (the highest preemptive thread priority) because it is assumed that the data sampling happens in the background.

The sampling thread stays in the unready state blocked on a semaphore.
The semaphore is triggered when the motion sensor trigger sends a notification that the data is available or when other application event requires the module interaction with the sensor (for example, when configuration is submitted from the host).

Sampling pipeline
-----------------

The motion module starts in ``STATE_DISABLED`` and after initialization enters ``STATE_DISCONNECTED``.
When disconnected, the module is not generating motion events, but the motion sensor is sampled to make sure its registers are cleared.

Upon connection, the following happens:

1. The motion module switches to the ``STATE_IDLE`` state, in which the module waits for the motion sensor trigger.
#. When a first motion is detected and the device is connected to a host (meaning it can transmit the HID data out), the module completes the following actions:

    a. Switches to ``STATE_FETCHING``.
    #. Samples the motion data.
    #. Submits the :c:struct:`motion_event`.
    #. Waits for the indication that the :c:struct:`motion_event` data was transmitted to the host.
       This is done when the module receives the :c:struct:`hid_report_sent_event` event.

#. At this point, a next motion sampling is performed and the next :c:struct:`motion_event` sent.

The module continues to sample data until disconnection or when there is no motion detected.
The ``motion`` module assumes no motion when a number of consecutive samples equal to :ref:`CONFIG_DESKTOP_MOTION_SENSOR_EMPTY_SAMPLES_COUNT <config_desktop_app_options>` returns zero on both axis.
In such case, the module will switch back to ``STATE_IDLE`` and wait for the motion sensor trigger.

Movement data from buttons
==========================

Motion is generated based on the total time a button is pressed.
The time measurements rely on the hardware clock.
If available, the module uses the :c:func:`k_cycle_get_64` function to read the hardware clock.
Otherwise, the :c:func:`k_cycle_get_32` function is used for that purpose.

When a HID subscriber is connected, that is when the device is connected either over USB or Bluetooth LE, the module forwards the generated motion to the subscriber using :c:struct:`motion_event`.
The first :c:struct:`motion_event` is generated when a button is pressed.
The subsequent :c:struct:`motion_event` is submitted when the previously generated motion data is sent to the subscriber, that is when :c:struct:`hid_report_sent_event` is received by the module.

Simulated movement data
=======================

Current position in the generated trajectory is determined by a hardware clock.
If available, the module uses the :c:func:`k_cycle_get_64` function to read the hardware clock when generating motion.
Otherwise, the :c:func:`k_cycle_get_32` function is used for that purpose.

When a HID subscriber connects, that is when the device is connected either over USB or Bluetooth LE, the module submits the first :c:struct:`motion_event` containing motion calculated from the previously reported position.
The subsequent :c:struct:`motion_event` is submitted when the previously generated motion data is sent to the subscriber, that is when :c:struct:`hid_report_sent_event` is received by the module.
The values of motion for X and Y axes are generated from an updated timestamp and the previously reported position.

.. _nrf_desktop_motion_report_rate:

Motion report rate
==================

If the device is polled for HID data with high frequency, that is when the :c:struct:`hid_report_sent_event` is received often by the module, the generated :c:struct:`motion_event` may contain value of ``0`` for both X and Y axes.

.. note::
   A HID report with reporting motion of ``0`` for both axes may be ignored by host computer's tools that analyze the HID report rate.

You can perform the following actions to ensure that non-zero motion is reported by the motion module:

* ``Movement data from motion sensors``
  By constantly moving your mouse, you can ensure that the used motion sensor always reports motion for at least one of the axes.
  You can also increase the motion sensor's CPI to make the sensor report bigger values of motion per every inch of distance covered by the mouse.
* ``Movement data from buttons``
  Increase a motion generated per second during a button press.
* ``Simulated movement data``
  Increase the value of the used scale factor or reduce time for transition between two points in the trajectory to increase the generated motion values.
  Keep in mind that the generated shape is periodically repeated, so the transition time between two points in the trajectory should not be too short.
  If it is, it might cause the same point to generate twice which would also result in submitting a :c:struct:`motion_event` with values set to ``0`` for both axes.

See the :ref:`nrf_desktop_motion_configuration` section for details on how to modify configuration for a given implementation of the module.
