.. _nrf5340_remote_shell:

nRF5340: Remote IPC shell
#########################

.. contents::
   :local:
   :depth: 2

You can use this sample to run the remote shell on the nRF5340 network core through the IPC service.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample shows how to run remote shell on the remote CPU using IPC service backend for shell.
It collects shell data from the IPC endpoint and forwards it to the terminal over USB transport or UART.
If the shell runs on the network core, you might want to use a peripheral that the network core does not have, for example USB.
You can use this sample to demonstrate how to forward shell data from the network core to the application core and connect the shell terminal through the application core.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf5340/remote_shell`

.. include:: /includes/build_and_run.txt

.. _remote_shell_testing:

Testing
=======

To test the sample, complete the following steps:

1. Program the sample to the application core.
#. Program the :ref:`radio_test` sample to the network core.
#. Plug the DK into a USB host device.
   The DK is visible as a COM port (Windows) or ttyACM device (Linux) after you connect the development kit over USB.
#. |connect_terminal|

Dependencies
************

This sample the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/device.h``
  * ``include/drivers/uart.h``
  * ``include/zephyr.h``
  * ``include/sys/ring_buffer.h``
  * ``include/sys/atomic.h``
  * ``include/usb/usb_device.h``
