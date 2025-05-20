.. _nrf5340_empty_app_core:

nRF5340: Empty firmware for application core
############################################

.. contents::
   :local:
   :depth: 2

You can use this sample to run an application on the network core of the nRF5340 when there is no need for the working application core.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample has a minimal Zephyr configuration: no multithreading, no clock, no MPU, no device drivers.
It does the following:

* During system initialization:

  * It allows the network core to access GPIO pins for LEDs and buttons.
    If more pins are required, you can add them to the :c:func:`network_gpio_allow` function.
  * It starts the network core.
    This is not done directly in the source code of the sample, but internally by Zephyr.

* In the :c:func:`main` function of the sample:

  * The application RAM is powered off to reduce power consumption.
  * The application core is suspended indefinitely.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf5340/empty_app_core`

.. include:: /includes/build_and_run.txt

Testing
=======

To test the sample, complete the following steps:

1. Program the sample to the application core.
#. Program Zephyr's :zephyr:code-sample:`blinky` sample to the network core.
#. Observe the LEDs on the kit.

Dependencies
************

This sample has the following `nrfx`_ dependencies:

* ``nrfx/nrf.h``
* ``nrfx/nrfx.h``

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/init.h``
