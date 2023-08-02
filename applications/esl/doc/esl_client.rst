.. _esl_service_client_readme:

Electronic Shelf Label Service (ESLS) Client
############################################

.. contents::
   :local:
   :depth: 2

This module implements the Electronic Shelf Label Service client which acts as Access Point(AP) to use the ESL Service exposed by an ESL device,
and how the central AP communicates with ESLs connectionlessly when one or more ESLs are in the Synchronized state.

The ESL Service Client is used in the :ref:`esl_ap` application.

Service UUID
************

The 128-bit vendor-specific service UUID is ``0x1857``.

Characteristics
***************

This service has 9 characteristics.

ESL Address Characteristic (``0x2BF6``)
============================================================

Write
   Write data to the ESL Address Characteristic to configure unique address to ESL device.

AP Sync Key Material Characteristic (``0x2BF7``)
============================================================

Write
   The AP Sync Key Material is configured when an AP, acting as a Client, writes its AP Sync Key Material value to the AP Sync Key Material characteristic.

ESL Response Key Material Characteristic (``0x2BF8``)
============================================================

Write
   The ESL Response Key Material is configured when an AP, acting as a Client, writes a value to the ESL Response Key Material characteristic.

ESL Current Absolute Time Characteristic (``0x2BF9``)
============================================================

Write
   When a value is written to the ESL Current Absolute Time characteristic, the Server shall set its current system time to the value.


ESL Display Information Characteristic (``0x2BFA``)
============================================================

Read
   The ESL Display Information characteristic shall return an array of one or more Display Data Structures when read by a Client.

ESL Image Information Characteristic (``0x2BFB``)
============================================================

Read
   When read, the ESL Image Information characteristic shall return the Max_Image_Index value equal to the numerically highest Image_Index value supported by the ESL.

ESL Sensor Information Characteristic (``0x2BFC``)
============================================================

Read
   The ESL Sensor Information characteristic shall return an array of one or more Sensor Information Structures when read by a Client.

ESL LED Information Characteristic (``0x2BFD``)
============================================================

Read
   The ESL LED Information characteristic shall return an array of one or more octets when read by a Client.

ESL Control Point Characteristic (``0x2BFE``)
============================================================

Write Without Response, Write, Notify
   The ESL Control Point characteristic allows a Client to write commands to an ESL while in a connection.

API documentation
*****************

| Header file: :file:`../service/esl_client.h`
| Source file: :file:`../service/esl/esl_client.c`

.. doxygengroup:: bt_eslc
   :project: nrf
   :members:
