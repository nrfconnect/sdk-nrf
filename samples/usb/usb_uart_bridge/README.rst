.. _usb_uart_bridge_sample:

USB-UART bridge
###############

The USB-UART bridge acts as a serial adapter, exposing 2 UART pairs to a USB host as 2 CDC ACM devices.


Requirements
************

* One of the following development boards:

  * |Thingy91|

* A USB host which can communicate with CDC ACM devices, like a Windows or Linux PC


Building and running
********************
.. |sample path| replace:: :file:`samples/usb/usb_uart_bridge`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the host via a USB cable
#. Observe that the CDC ACM devices enumerate on the USB host (Typically COM ports on Windows, /dev/tty* on Linux)
#. Use a serial client on the USB host to communicate over the board's UART pins


Dependencies
************

This sample uses the following Zephyr subsystems:

* :ref:`zephyr:usb_device_stack`
