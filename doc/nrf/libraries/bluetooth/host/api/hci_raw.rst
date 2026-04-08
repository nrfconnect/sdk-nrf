.. _bt_hci_raw:

.. contents::
   :local:
   :depth: 2

HCI RAW channel
###############

The HCI RAW channel API exposes the HCI interface to the remote entity.
The local Bluetooth controller is owned by the remote entity and the host Bluetooth stack is not
used.
The RAW API provides direct access to packets that are sent and received by the Bluetooth HCI
driver.

API Reference
*************

.. doxygengroup:: hci_raw
