.. _nsms_readme:

Nordic Status Message Service (NSMS)
####################################

.. contents::
   :local:
   :depth: 2

The Nordic Status Message Service (NSMS) is a custom service that provides the name of the status and a string describing the current status.
The status can be either notified or directly read (without waiting for notification) to check the current status whenever the device is connected.

The NSMS service is used in the :ref:`peripheral_status` sample.

Multiple instances of the service can be implemented to show the status of different parts of the system.
The instances can be distinguished by the client by its name provided in the Characteristic User Description (CUD).

Service UUID
************

The 128-bit vendor-specific service UUID is 57a70000-9350-11ed-a1eb-x0242ac120002.

Characteristics
***************

This service has one characteristic with the CCC and CUD descriptors.
CUD holds the name of the Status Message and helps to distinguish between different instances of the NSMS on the same device.

Status Characteristic (57a70001-9350-11ed-a1eb-0242ac120002)
============================================================

Notify
  Enable notifications for the Status Characteristic to receive notifications when the status message changes.

Read
  Read the current status.

Configuration
*************

To use this library, enable the :kconfig:option:`CONFIG_BT_NSMS` Kconfig option.

API documentation
*****************

| Header file: :file:`include/nsms.h`
| Source file: :file:`subsys/bluetooth/services/nsms.c`

.. doxygengroup:: bt_nsms
   :project: nrf
   :members:
