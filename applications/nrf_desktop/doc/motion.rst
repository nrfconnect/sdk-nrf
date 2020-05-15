.. _nrf_desktop_motion:

Motion module
#############

The motion module is responsible for generating events related to movement.
The movement can be detected using the motion sensor, but it can also be sourced from GPIO pins or simulated.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_motion_start
    :end-before: table_motion_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The motion module selects the source of movement based on the following configuration options:

* ``CONFIG_DESKTOP_MOTION_NONE`` - Module is disabled.
* ``CONFIG_DESKTOP_MOTION_SENSOR_PMW3360_ENABLE`` - Movement data is obtained from the gaming-grade ``PMW3360`` motion sensor.
* ``CONFIG_DESKTOP_MOTION_SENSOR_PAW3212_ENABLE`` - Movement data is obtained from ``PAW3212`` motion sensor.
* ``CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE`` - Movement data is generated using buttons.
* ``CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE`` - Movement data is simulated (controlled from Zephyr's shell).

See the following sections for more information.

Depending on the selected configuration option, different implementation file is used during the build process.

Movement data from motion sensors
=================================

Selecting either of the motion sensors (``CONFIG_DESKTOP_MOTION_SENSOR_PMW3360_ENABLE`` or ``CONFIG_DESKTOP_MOTION_SENSOR_PAW3212_ENABLE``) adds the :file:`src/hw_interface/motion_sensor.c` file to the compilation.

The motion sensor is sampled from the context of a dedicated thread.
The option ``CONFIG_DESKTOP_MOTION_SENSOR_THREAD_STACK_SIZE`` is used to set the thread's stack size.

The motion sensor default sensitivity and power saving switching times can be set with the following options:

* ``CONFIG_DESKTOP_MOTION_SENSOR_CPI`` - Default CPI.
* ``CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_TIMEOUT_MS`` - ``Sleep 1`` mode default switch time.
* ``CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_TIMEOUT_MS`` - ``Sleep 2`` mode default switch time.
* ``CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_TIMEOUT_MS`` - ``Sleep 3`` mode default switch time.

For more information, see the sensor documentation and the Kconfig help.

Movement data from buttons
==========================

Selecting the ``CONFIG_DESKTOP_MOTION_BUTTONS_ENABLE`` option adds the :file:`src/hw_interface/motion_buttons.c` file to the compilation.

Simulated movement data
=======================

Selecting the ``CONFIG_DESKTOP_MOTION_SIMULATED_ENABLE`` option adds the :file:`src/hw_interface/motion_simulated.c` file to the compilation.
This option depends on the shell (``CONFIG_DESKTOP_SHELL_ENABLE`` option) to be enabled in the application configuration.

When using this option, the motion module registers a shell module ``motion_sim`` and links to it two commands: ``start`` and ``stop``.

When started, the module will generate simulated motion events.
The movement data in each event will be tracing the predefined path, an eight-sided polygon.

You can configure the path with the following options:

* ``CONFIG_DESKTOP_MOTION_SIMULATED_EDGE_TIME`` - Sets how long each edge is traced.
* ``CONFIG_DESKTOP_MOTION_SIMULATED_SCALE_FACTOR`` - Scales the size of the polygon.

The ``stop`` command will cause the module to stop generating new events.

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
Using a dedicated thread also simplifies the module, because the interaction with the sensor becomes sequential.
The sampling thread priority is set to 0 (the highest preemptive thread priority), because it is assumed that the data sampling will happen in the background.

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
The ``motion`` module assumes no motion when a number of consecutive samples equal to ``CONFIG_DESKTOP_MOTION_SENSOR_EMPTY_SAMPLES_COUNT`` returns zero on both axis.
In such case, the module will switch back to ``STATE_IDLE`` and wait for the motion sensor trigger.
