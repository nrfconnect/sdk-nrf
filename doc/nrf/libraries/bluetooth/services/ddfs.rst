.. _ddfs_readme:

Direction and Distance Finding Service (DDFS)
#############################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Low Energy GATT Direction and Distance Finding Service is a custom service that allows publication of distance, azimuth and elevation measurement data.
It also allows adjusting the measurement configuration parameters.

.. note::
   The current implementation is :ref:`experimental <software_maturity>`.

Service UUID
************

The 128-bit vendor-specific service UUID is ``21490000-494a-4573-98af-f126af76f490``.

.. list-table:: Characteristic UUIDs
    :widths: auto
    :header-rows: 1

    * - Characteristic
      - UUID
    * - Distance Measurement
      - ``21490001-494a-4573-98af-f126af76f490``
    * - Azimuth Measurement
      - ``21490002-494a-4573-98af-f126af76f490``
    * - Elevation Measurement
      - ``21490003-494a-4573-98af-f126af76f490``
    * - DDF Feature
      - ``21490004-494a-4573-98af-f126af76f490``
    * - Control Point
      - ``21490005-494a-4573-98af-f126af76f490``

Characteristics
***************

This service has the following characteristics.

Distance Measurement Characteristic
===================================

Notify:
    Enable notifications for the Distance Measurement Characteristic to receive measurements.

Azimuth Measurement Characteristic
==================================

Notify:
    Enable notifications for the Azimuth Measurement Characteristic to receive measurements.

Elevation Measurement Characteristic
====================================

Notify:
    Enable notifications for the Elevation Measurement Characteristic to receive measurements.

DDF Feature Characteristic
==========================

Read:
    Read the supported features.

Control Point Characteristic
============================

Write:
    Write data to the Control Point Characteristic to change the configuration.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/ddfs.h`
| Source file: :file:`subsys/bluetooth/services/ddfs.c`

.. doxygengroup:: bt_ddfs
   :project: nrf
   :members:
