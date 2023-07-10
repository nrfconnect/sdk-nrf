.. _esl_service_readme:

Electronic Shelf Label Service (ESLS)
#####################################

.. contents::
   :local:
   :depth: 2

The Electronic Shelf Label Service allows you to control and update electronic shelf labels (ESL) using Bluetooth® wireless technology.

The ESL Service is used in the :ref:`peripheral_esl` sample.

Service UUID
************

The 16-bit vendor-specific service UUID is ``0x1857``.

Characteristics
***************

This service has the following nine characteristics.

ESL Address Characteristic (``0x2BF6``)
=======================================

Write
   Write data to the ESL Address Characteristic to configure a unique address to ESL device.

AP Sync Key Material Characteristic (``0x2BF7``)
================================================

Write
   The AP Sync Key Material is configured when an AP, acting as a client, writes its AP Sync Key Material value to the AP Sync Key Material characteristic.

ESL Response Key Material Characteristic (``0x2BF8``)
=====================================================

Write
   The ESL Response Key Material is configured when an AP, acting as a client, writes a value to the ESL Response Key Material characteristic.

ESL Current Absolute Time Characteristic (``0x2BF9``)
=====================================================

Write
   When a value is written to the ESL Current Absolute Time characteristic, the server sets its current system time to the value.


ESL Display Information Characteristic (``0x2BFA``)
===================================================

Read
   The ESL Display Information characteristic returns an array of one or more Display Data Structures when read by a client.

ESL Image Information Characteristic (``0x2BFB``)
=================================================

Read
   When read, the ESL Image Information characteristic returns the Max_Image_Index value equal to the numerically highest Image_Index value supported by the ESL.

ESL Sensor Information Characteristic (``0x2BFC``)
==================================================

Read
   The ESL Sensor Information characteristic returns an array of one or more Sensor Information Structures when read by a client.

ESL LED Information Characteristic (``0x2BFD``)
===============================================

Read
   The ESL LED Information characteristic returns an array of one or more octets when read by a client.

ESL Control Point Characteristic (``0x2BFE``)
=============================================

Write Without Response, Write, Notify
   The ESL Control Point characteristic allows a client to write commands to an ESL while connected.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/esl.h`
| Source file: :file:`subsys/bluetooth/services/esl/esl.c`

.. doxygengroup:: bt_esl
   :project: nrf
   :members:
