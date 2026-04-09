.. _bt_hci_raw:

HCI RAW channel
###############

.. contents::
   :local:
   :depth: 2

The HCI RAW channel API exposes the HCI interface to the remote entity.
The local Bluetooth controller is owned by the remote entity and the host Bluetooth stack is not used.
The RAW API provides direct access to packets that are sent and received by the Bluetooth HCI driver.

API Reference
*************

| Header file: :file:`include/bluetooth/host/hci_raw.h`

.. doxygengroup:: hci_raw
