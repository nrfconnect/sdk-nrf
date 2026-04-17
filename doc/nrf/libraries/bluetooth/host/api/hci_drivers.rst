.. _bt_hci_drivers:

HCI Drivers
###########

.. contents::
   :local:
   :depth: 2

The HCI Driver API defines the interface between the Bluetooth host stack and an HCI transport driver.
It provides operations for opening and closing the transport, sending HCI packets, and performing an optional controller-specific setup.
These APIs are used by HCI driver implementations that connect the host to a Bluetooth controller.

API Reference
*************

| Header file: :file:`include/bluetooth/drivers/bluetooth.h`

.. doxygengroup:: bt_hci_api
