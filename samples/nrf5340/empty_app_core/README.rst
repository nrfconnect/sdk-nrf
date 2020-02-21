.. _nrf5340_empty_app_core:

nRF5340: Empty firmware for application core
############################################

This sample can be used to run an application on the network core of the nRF5340 when there is no need for the working application core.

Overview
********

The sample has minimal Zephyr configuration: no multithreading, no clock, no MPU, no device drivers.
It does the following things:

* During system initialization:

  * It allows the network core to access GPIO pins for LEDs and buttons.
    If more pins are required, you can add them to the :cpp:func:`network_gpio_allow()` function.
  * It starts the network core.
    This is not done directly in the source code of the sample, but internally by Zephyr.

* In the :cpp:func:`main()` function of the sample:

  * The application RAM is powered off to reduce power consumption.
  * The application core is suspended indefinitely.

Requirements
************
The following development board:

  * |nRF5340DK|

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf5340/empty_app_core`

.. include:: /includes/build_and_run.txt

Testing
=======
1. Program this sample to the application core.
#. Program the Zephyr's :ref:`zephyr:blinky-sample` to the network core.
#. Observe the LEDs on the board.

Dependencies
************

This sample uses the following `nrfx`_ dependencies:

  * ``nrfx/nrf.h``
  * ``nrfx/nrfx.h``

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/init.h``
