.. _lib_adp536x:

adp536x
#######

.. contents::
   :local:
   :depth: 2

The adp536x library provides an API to the ADP5360 :term:`power management integrated circuit (PMIC)`.

Implementation
==============

The library accesses the features of the ADP5360 PMIC by conducting Inter-Integrated Circuit (I2C) transactions with its registers.
Generic read, write, and masked write functions are private to the library.
The feature-specific functions are available to the user.

.. note::
   Regulator functionality (BUCK/BUCKBOOST) is exposed using the :ref:`Zephyr regulator API <regulator_api>`.

Supported features
==================

The library provides several functions to set operating parameters for the ADP5360 PMIC and for reading status values.

Requirements
************

An I2C interface is required to communicate with the ADP5360 PMIC.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_ADP536X` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

Initialize the library using the :c:func:`adp536x_init` function and use the API to configure and read from the ADP5360 PMIC.

Dependencies
************

This library uses the I2C-capable hardware.

API documentation
*****************

| Header file: :file:`include/adp536x`
| Source files: :file:`lib/adp536x/src`

Following define VBUS current limit values, charging current values, and overcharge protection threshold values respectively:

.. doxygengroup:: adp536x
      :project: nrf
      :members:
