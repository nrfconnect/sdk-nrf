.. _gpio_frontend_sample:

GPIO frontend
#############

The GPIO frontend sample shows how to use GPIO logging fronted with PC host.
It uses the gpio_frontend module.

Overview
********

This sample uses (by default):

  * Pins 1.01 to 1.09 on PCA10056
  * Pins 0.22 to 0.30 on PCA10040 and PCA10090

A set of various logs is being sent every second.

Requirements
************

* One of the following development boards:

  * nRF9160 DK board (PCA10090)
  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)
* SALEAE Logic analyzer (Pro is recommended)

Building and running
********************
.. |sample path| replace:: :file:`samples/debug/gpio_frontend`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect a logic analyzer to the pins that are used for logging: P1.01 or P0.22 to Channel 0 and so on, up to P1.09 or P0.30 to Channel 8.
Remember to connect GND pins too.

2. In Saleae Logic, disable all channels except for 0 - 8. Disable all analogue channels. Set voltage to 3.3+ Volts.

3. Run debug.bat

4. After ZephyrLogParser is started, enter "gpio init" to initialize connection with Saleae Logic.

5. Enter "gpio start" to begin logs collection. Remember to restart the board manually directly after entering the command.

6. After all the logs are acquired, type "gpio parse".

#. Observe that:

   * Logs transfered with GPIO frontend are displayed on the console.


Dependencies
************

This sample uses the following |NCS| subsystems:

* gpio_frontend

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:logger`
