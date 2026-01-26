.. _lib_accel_to_angle:

Accel to angle
##############

.. contents::
   :local:
   :depth: 2

This library provides functionality to convert accelerometer data into pitch and roll angles.
You can use the calculated data further to check for the motion detection of the device.

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

The library also supports optional filtering of the accelerometer data before conversion.
You can use the already implemented filter or create your own custom filter that is compatible with the library API.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_ACCEL_TO_ANGLE` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

Usage
*****

To use the library, perform the following steps:

1. (Optional) If you want to use filtering in the data conversion pipeline, you must define the filter instance.
   Ensure that the filter structure is always available during library usage.
   The contents of it must never be accessed or used directly by the application.
   You have the following two options to define a filter:

    * Use one of the already implemented filters that are delivered together with the library.
      Currently, the library only supports the exponential moving average (EMA) filter.
      You can allocate the EMA filter instance with the help of the :c:macro:`ACCEL_TO_ANGLE_FILTER_EMA_DEFINE` macro that is available in the :file:`include/accel_to_angle/filter/ema.h` header.
    * Use a custom filter that is compatible with the library API.
      For an example on how to develop a custom filter, see the :ref:`lib_accel_to_angle_ema_api`.

#. Define and initialize the library context structure by calling the :c:macro:`ACCEL_TO_ANGLE_CTX_DEFINE` macro.
   The context structure must never be accessed or used directly by the application.
   If you want to use optional filtering step, pass the filter instance to the :c:macro:`ACCEL_TO_ANGLE_CTX_DEFINE` macro.
   Otherwise, you can use the `NULL` value.
#. (Optional) Reconfigure the filter with the :c:func:`accel_to_angle_filter_set` function to change a filter.
#. Fetch accelerometer data from the sensor and populate the sensor values array.
#. Call the :c:func:`accel_to_angle_calc` function to convert accelerometer data to rotation data.
#. Use the calculated rotation data in your application.
#. (Optional) Call the :c:func:`accel_to_angle_diff_check` function to check if the change in rotation exceeds thresholds for motion detection.
#. (Optional) Use the :c:func:`accel_to_angle_state_clean` function to reset the accumulated state of the motion difference and filter state (if used).

Limitations
***********

Due to the nature of mathematical calculations involved in converting accelerometer data to rotation data, the library can only calculate pitch and roll angles (without yaw).

Dependencies
************

This library has the following dependencies:

* Zephyr :ref:`zephyr:sensor`
* Standard C library :file:`math.h`

API documentation
*****************

The library uses the following API elements:

Accel to angle library
======================

| Header file: :file:`include/accel_to_angle/accel_to_angle.h`
| Source file: :file:`lib/accel_to_angle/accel_to_angle.c`

.. doxygengroup:: accel_to_angle

.. _lib_accel_to_angle_ema_api:

Accel to angle EMA filter
=========================

| Header file: :file:`include/accel_to_angle/filter/ema.h`
| Source file: :file:`lib/accel_to_angle/filter_ema.c`

.. doxygengroup:: accel_to_angle_filter_ema
