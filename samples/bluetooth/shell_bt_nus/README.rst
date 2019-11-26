.. _shell_bt_nus:

Bluetooth: NUS shell transport
##############################

The Nordic UART Service (NUS) shell transport sample demonstrates how to use the :ref:`shell_bt_nus_readme` to receive shell commands from a remote device.

Overview
********

When the connection is established, you can connect to the sample through the :ref:`nus_service_readme` by using a host application.
You can then send shell commands that are executed on the device that runs the sample, and see the logs.
See :ref:`shell_bt_nus_host_tools` for more information about the host tools that are available in |NCS| for communicating with the sample.

Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

* A second nRF52 Development Kit board (PCA10040) for connecting with bt_nus_shell.py.
  Alternatively, you can use :ref:`ble_console_readme` for connecting (Linux only).


Building and running
********************

This sample can be found under :file:`samples/bluetooth/shell_bt_nus` in the
|NCS| folder structure.

See :ref:`gs_programming` for information about how to build and program the
application.

Testing
=======

After programming the sample to your board, test it by connecting to it and sending some shell commands.

.. include:: ../../../include/shell/shell_bt_nus.rst
   :start-after: testing_start
   :end-before: testing_end

Alternatively, use the :ref:`ble_console_readme` for testing.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`shell_bt_nus_readme`
* :ref:`nus_service_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
  * ``samples/bluetooth/gatt/bas.h``
* :ref:`zephyr:logger`
