.. _lib_accel_to_angle:

Accel to angle
##############

.. contents::
   :local:
   :depth: 2

This library provides functionality to convert accelerometer data into rotation data.
The calculated data can be further used to check for the motion detection of the device.

Overview
********

This library processes raw accelerometer data and computes rotation values based on the input.
Using an accelerometer instead of a gyroscope decreases the power consumption and cost.

The library supports multiple axes and you can configure it to work with different accelerometer sensors.
You can use it simultaneously or independently for rotation calculation and threshold-based motion detection.

Implementation
==============

The library is divided into the following two parts:

* The core library that handles the conversion of accelerometer data to rotation data.
* The motion detection module that uses the rotation data to determine if motion has occurred based on thresholds.

You can use both parts independently or together, depending on the application requirements.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_ACCEL_TO_ANGLE` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

Initialize the library using the :c:func:`accel_to_angle_init` function and use the API to configure and process the data.

Usage
*****

To use the library, perform the following steps:

1. Define and initialize the library context structure by calling the :c:func:`accel_to_angle_init` function.
   The context structure must never be accessed or used directly by the application.
#. (Optional) Configure the filter with the :c:func:`accel_to_angle_ema_filter_set` function to use the Exponential Moving Average (EMA) filter for smoothing the accelerometer data, or use the :c:func:`accel_to_angle_custom_filter_set` function to set a custom filter.
   When using the EMA filter, allocate the :c:struct:`accel_to_angle_ema_ctx` structure and make sure it is available during the library usage.
   The contents of the structure must never be accessed or used directly by the application.
#. Fetch accelerometer data from the sensor and populate the sensor values array.
#. Call the :c:func:`accel_to_angle_calc` function to convert accelerometer data to rotation data.
#. Use the calculated rotation data in your application.
#. Call the :c:func:`accel_to_angle_diff_check` function to check if the change in rotation exceeds thresholds for motion detection.

Additionally, you can use the :c:func:`accel_to_angle_state_clean` function to reset the accumulated state of the motion difference.

Limitations
***********

Due to the nature of mathematical calculations involved in converting accelerometer data to rotation data, the library can only calculate pitch and roll angles (without yaw).

Dependencies
************

This library has the following dependencies:

* Zephyr :zephyr:ref:`sensor`
* Standard C library ``math.h``

API documentation
*****************

.. code-block::

   | Header file: :file:`include/accel_to_angle.h`
   | Source files: :file:`lib/accel_to_angle`

   .. doxygengroup:: accel_to_angle
