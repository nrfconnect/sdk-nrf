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
* :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE <config_desktop_app_options>` - Movement data is simulated (controlled from Zephyr's :ref:`zephyr:shell_api`).

See the following sections for more information.

Depending on the selected configuration option, a different implementation file is used during the build process.

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

Simulated movement data
=======================

Selecting the :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE <config_desktop_app_options>` option adds the :file:`src/hw_interface/motion_simulated.c` file to the compilation.

If the shell is available (the :kconfig:option:`CONFIG_SHELL` option is set), the motion module registers a shell module ``motion_sim`` and links to it two commands: ``start`` and ``stop``.
If the shell is not available, motion generation starts automatically when the device is connected to USB or Bluetooth®.

When started, the module will generate simulated motion events.
The movement data in each event will be tracing the predefined path, an eight-sided polygon.

You can configure the path with the following options:

* :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_EDGE_TIME <config_desktop_app_options>` - Sets how long each edge is traced.
* :ref:`CONFIG_DESKTOP_MOTION_SIMULATED_SCALE_FACTOR <config_desktop_app_options>` - Scales the size of the polygon.

The ``stop`` command will cause the module to stop generating new events.

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

This section describes the motion module implementation for motion sensors.

Motion sensor
=============

The motion module samples movement data from the motion sensor using the motion sensor driver.

The name of the device linked with the driver changes depending on the selected sensor.
The same is true for the names of sensor configuration options.
The nRF Desktop application aggregates the sensor-specific information and translates them to the application abstracts in the :file:`configuration/common/motion_sensor.h` header file.

Sampling thread
===============

The motion module uses a dedicated sampling thread to sample data from the motion sensor.
The reason for using the sampling thread is the long time required for data to be ready.
Using a dedicated thread also simplifies the module because the interaction with the sensor becomes sequential.
The sampling thread priority is set to 0 (the highest preemptive thread priority) because it is assumed that the data sampling happens in the background.

The sampling thread stays in the unready state blocked on a semaphore.
The semaphore is triggered when the motion sensor trigger sends a notification that the data is available or when other application event requires the module interaction with the sensor (for example, when configuration is submitted from the host).

Sampling pipeline
=================

The motion module starts in ``STATE_DISABLED`` and after initialization enters ``STATE_DISCONNECTED``.
When disconnected, the module is not generating motion events, but the motion sensor is sampled to make sure its registers are cleared.

Upon connection, the following happens:

1. The motion module switches to the ``STATE_IDLE`` state, in which the module waits for the motion sensor trigger.
#. When a first motion is detected and the device is connected to a host (meaning it can transmit the HID data out), the module completes the following actions:

    a. Switches to ``STATE_FETCHING``.
    #. Samples the motion data.
    #. Submits the ``motion_event``.
    #. Waits for the indication that the ``motion_event`` data was transmitted to the host.
       This is done when the module receives the ``hid_report_sent_event`` event.

#. At that point, a next motion sampling is performed and the next ``motion_event`` sent.

The module continues to sample data until disconnection or when there is no motion detected.
The ``motion`` module assumes no motion when a number of consecutive samples equal to :ref:`CONFIG_DESKTOP_MOTION_SENSOR_EMPTY_SAMPLES_COUNT <config_desktop_app_options>` returns zero on both axis.
In such case, the module will switch back to ``STATE_IDLE`` and wait for the motion sensor trigger.
