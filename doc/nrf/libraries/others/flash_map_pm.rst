.. _lib_flash_map_pm:

Partition Manager flash map
###########################

.. contents::
   :local:
   :depth: 2

The Partition Manager flash map library provides a set of defines for reading flash partition information when using the :ref:`partition_manager` script.
This is mandatory for providing a unified interface for flash partitions when the Partition Manager is used.

Overview
********

In Zephyr, the Flash Map API provides an abstraction layer and a virtual address space on a flash device.
It allows to restrict access to specific flash areas, identified by a partition label and defined by device, offset, and size.
You can remap these areas between devices before compilation without the need to modify applications that already access them with the Flash Map API.
Applications or subsystems that access the flash area do not have to know the device on which the area is defined.
Such applications or subsystems only need to know the label of the flash area and they will be able to operate only within the flash area mapped to this label.
Regardless of where it is defined, each flash area is always mapped to range of addresses that starts at ``0`` and has the specified size.

By default, the header file gets this flash area information from :ref:`zephyr:dt-guide` (namely, :file:`zephyr.dts`).

In the |NCS|, when the Partition Manager is used, flash area information is not taken from devicetree.
Instead, it is taken from :file:`pm_config.h`.
In this configuration, :file:`flash_map_pm.h` acts as the Partition Manager's backend for the flash area interface.

Translation of partition labels
===============================

The library also provides a translation of partition labels used in upstream Zephyr.
This is because the labels used in the Partition Manage do not match them one to one.
The library makes sure that different labels point to logically identical flash areas and are compatible with each other.

Configuration
*************

The information given by this file depends on your Partition Manager configuration.

Usage
*****

The library is automatically used when you run the :ref:`partition_manager` script and include the :file:`storage/flash_map.h` file.

Dependencies
************

This library depends on :ref:`partition_manager`.

API documentation
*****************

The library does not expose any API of its own.

| Header file: :file:`zephyr/include/zephyr/storage/flash_map.h`
