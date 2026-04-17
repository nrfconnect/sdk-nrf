.. _bt_gap:

Generic Access Profile (GAP)
############################

.. contents::
   :local:
   :depth: 2

The Generic Access Profile (GAP) defines fundamental Bluetooth procedures such as advertising, scanning, device discovery, and connection establishment.
In this API set, the main GAP procedures are declared in the :file:`include/bluetooth/host/bluetooth.h` file.
Related device address helpers are declared in :file:`include/bluetooth/host/addr.h`.
GAP defines and assigned numbers are declared in :file:`include/bluetooth/host/gap.h`.

API Reference
*************

GAP
===

| Header file: :file:`include/bluetooth/host/bluetooth.h`

.. doxygengroup:: bt_gap

Device address
==============

| Header file: :file:`include/bluetooth/host/addr.h`

.. doxygengroup:: bt_addr

Defines and assigned numbers
============================

| Header file: :file:`include/bluetooth/host/gap.h`

.. doxygengroup:: bt_gap_defines
