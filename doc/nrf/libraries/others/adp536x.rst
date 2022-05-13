.. _lib_adp536x:

adp536x
#######

.. contents::
   :local:
   :depth: 2

The adp536x library provides an API to the ADP5360 power management IC.

Implementation
==============

The library accesses the features of the ADP5360 by conducting I2C transactions to its registers. Generic read, write and masked write functions are private to the library, and feature-specific functions are exposed to the user.

Supported features
==================

The library exposes a number of functions to set operating parameters of the ADP5360, as well as reading out status values.

Requirements
************

An I2C interface is required to communicate with the ADP5360.

Configuration
*************

The library is enabled by the :kconfig:option:`CONFIG_ADP536X` configuration option. The I2C bus to use can be set with :kconfig:option:`CONFIG_ADP536X_BUS_NAME`.

Usage
*****

Initialize the library with :c:func:`adp536x_init`, and use the API to configure and read from the ADP5360 as desired.

Dependencies
************

I2C-capable hardware

API documentation
*****************

.. code-block::

   | Header file: :file:`include/adp536x`
   | Source files: :file:`lib/adp536x/src`

   .. doxygengroup:: adp536x
      :project: nrf
      :members:
