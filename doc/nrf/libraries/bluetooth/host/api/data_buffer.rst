.. _bt_data_buffers:

Data Buffers
#############

.. contents::
   :local:
   :depth: 2

The Data Buffers API defines the Bluetooth host buffer types used for HCI commands, events, ACL data, and ISO data.
It provides helpers for calculating buffer sizes and for converting between Bluetooth buffer types and H:4 packet types.
It also provides allocation APIs for incoming and outgoing Bluetooth data buffers.

API Reference
*************

| Header file: :file:`include/bluetooth/host/buf.h`

.. doxygengroup:: bt_buf
