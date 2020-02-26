.. _connectivity_bridge:

Connectivity bridge
###################

The Connectivity bridge application demonstrates the bridge functionality for the Thingy:91 hardware and is an enhancement to the :ref:`usb_uart_bridge_sample` sample.

Additionally, the application also provides an option of adding the bluetooth functionality by making use of the :ref:`nus_service_readme`.

Overview
********

The Connectivity bridge acts as a USB composite device, exposing two UART interfaces to a USB host as two CDC ACM devices.
The mapping of the UART interfaces are done in the following way:

.. list-table::
   :header-rows: 1
   :align: center

   * - UART Interface
     - CDC ACM port
   * - UART_0
     - CDC_0
   * - UART_1
     - CDC_1

See :ref:`thingy91_serialports` for information on the baud rate configuration for Thingy:91 serial ports.

The application adds the functionality of a USB Mass Storage device, which contains several utility files such as a :file:`README.txt` file.

The application also provides a Bluetooth LE UART Service, which can be enabled by the option ``CONFIG_BRIDGE_BLE_ENABLE``.
This service can be used for a wireless connection to one of the UART interfaces in the following way:

.. list-table::
   :header-rows: 1
   :align: center

   * - USB Interface
     - Service mapped
   * - UART_0
     - :ref:`nus_service_readme`

By default, the Bluetooth LE interface is off, as the connection is not encrypted or authenticated.
It can be turned on at runtime by setting the appropriate option in the :file:`Config.txt` file, which is located on the USB Mass storage Device.

Requirements
************

* One of the following development boards:

  * |Thingy91|

* A USB host which can communicate with CDC ACM devices, such as a Windows or Linux PC


Building and running
********************
.. |sample path| replace:: :file:`applications/connectivity_bridge`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the host via a USB cable.
#. Observe that the CDC ACM devices enumerate on the USB host (COM ports on Windows, /dev/tty* on Linux).
#. Use a serial client on the USB host to communicate over the board's UART pins.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`event_manager`
* :ref:`nus_service_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``include/uart.h``
   * ``include/usb.h``

* :ref:`zephyr:usb_device_stack`

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
