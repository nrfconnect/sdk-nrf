.. _wifi_provisioning_details:

Implementation
##############

.. contents::
   :local:
   :depth: 2

This page explains the implementation of the Wi-Fi Provisioning Service sample.

Task and event handling
***********************

The Wi-Fi Provisioning Service sample uses `Protocol Buffers`_ to define platform-independent message formats.
These definitions are available as :file:`.proto` files in the sample code.

The sample also uses `nanopb`_ to convert encoded platform-independent messages to and from C language.

The device performs actions according to messages it exchanges with a configurator (the peer that sends the message, such as a smartphone running the nRF Wi-Fi Provisioner app listed in :ref:`wifi_provisioning_app`).

.. _wifi_provisioning_message_types:

Message types
=============

The sample uses four message types:

* ``Info``: Allows the configurator to know the capability of a device before the provisioning process happens.

  The configurator reads this message before sending any other messages.
  The info message contains a ``version`` field.

* ``Request``: Contains a command and configuration.

  The device takes action based on the command and applies the configuration if one is given.

  The following table details the five commands that the sample uses.

   =================== ================================================ =============================
   Command             Description                                      Notes
   =================== ================================================ =============================
   GET_STATUS          Get the status of the device, such as
                       provisioning state, Wi-Fi connection state,
                       and more
   START_SCAN          Trigger an access point (AP) scan                ``scan_params`` is optional
   STOP_SCAN           Stop AP scan
   SET_CONFIG          Store Wi-Fi configuration and connect to an AP   ``config`` is required
   FORGET_CONFIG       Erase the configuration and disconnect
   =================== ================================================ =============================

* ``Response``: Contains the command to which it is responding, the operation result, and any other data.

  The device sends any additional data associated with this command to the configurator in a ``Result`` message.

  The ``Response`` message contains one of following response codes:

  * ``SUCCESS``: the operation is dispatched successfully.
  * ``INVALID_ARGUMENT``: the argument is invalid.
  * ``INVALID_PROTO``: the message cannot be encoded or decoded.
  * ``INTERNAL_ERROR``: the operation cannot be dispatched properly.

  If the command is ``GET_STATUS``, the response includes some or all of the following fields:

  * ``state``: describes the Wi-Fi connection state according to values defined in the :file:`common.proto` file.
  * ``provisioning_info``: includes Wi-Fi provisioning information stored in the non-volatile memory (NVM) of the device.
  * ``connection_info``: includes Wi-Fi connection details.
  * ``scan_info``: includes the parameters used for the AP scan.

* ``Result``: Carries asynchronous data about the operation status.

  * If the command is ``START_SCAN``, the result message sent to the configurator contains information about the AP.
    Each ``Result`` contains information related to one AP.
  * If the command is ``SET_CONFIG``, when the Wi-Fi status changes (for example, from disconnected to connected), the configurator receives a result message with the new status.
    Meanwhile, the Wi-Fi credentials are stored in the non-volatile memory of the device.

See all definitions in :file:`./common/proto`.

Operations
==========

The message sequence is the same for all operations, with variations depending on the command:

1. The configurator sends an encoded ``Request`` message with a command to the device.
#. The device carries out the command.
#. The device sends a ``Response`` message to the configurator.
#. For some operations, the device also sends a ``Result`` message to the configurator with any additional data generated or reported.

See :ref:`wifi_provisioning_message_types` for more information on the commands and additional parameters.

Message delivery
****************

Exchanging messages requires a secure transport method.
The Wi-Fi Provisioning Service sample uses Bluetooth® Low Energy (LE).
You can assign different security levels to components under one service to fine-tune access control.

Service declaration
===================

The Wi-Fi Provisioning Service is instantiated as a primary service.
Set the service UUID value as defined in the following table.

========================== ====================================
Service name               UUID
Wi-Fi Provisioning Service 14387800-130c-49e7-b877-2881c89cb258
========================== ====================================

Service characteristics
=======================

The UUID value of characteristics are defined in the following table.

========================== ====================================
Characteristic name        UUID
Information                14387801-130c-49e7-b877-2881c89cb258
Operation Control Point    14387802-130c-49e7-b877-2881c89cb258
Data Out                   14387803-130c-49e7-b877-2881c89cb258
========================== ====================================

The characteristic requirements of the Wi-Fi Provisioning Service are shown in the following table.

+-----------------+-------------+-------------+-------------+-------------+
| Characteristic  | Requirement | Mandatory   | Optional    | Security    |
| Name            |             | Properties  | Properties  | Permissions |
+=================+=============+=============+=============+=============+
| Information     | Mandatory   | Read        |             | No Security |
|                 |             |             |             | required    |
+-----------------+-------------+-------------+-------------+-------------+
| Operation       | Mandatory   | Indicate,   |             | Encryption  |
| Control         |             | Write       |             | Required    |
| Point           |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+
| Operation       | Mandatory   | Read, Write |             | Encryption  |
| Control         |             |             |             | Required    |
| Point           |             |             |             |             |
| - Client        |             |             |             |             |
| Characteristic  |             |             |             |             |
| Configuration   |             |             |             |             |
| descriptor      |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+
| Data Out        | Mandatory   | Notify      |             | Encryption  |
|                 |             |             |             | Required    |
+-----------------+-------------+-------------+-------------+-------------+
| Data Out        | Mandatory   | Read, Write |             | Encryption  |
| - Client        |             |             |             | Required    |
| Characteristic  |             |             |             |             |
| Configuration   |             |             |             |             |
| descriptor      |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+

The purpose of each characteristic is as follows:

* ``Information``: for client to get ``Info`` message from server.
* ``Operation Control Point``: for client to send ``Request`` message to server, and server to send ``Response`` message to client.
* ``Data Out``: for server to send ``Result`` message to the client.

Advertisement
=============

After powerup, the device checks if it is provisioned. If it is, it loads saved credentials and tries to connect to the specified access point.

Then, the device advertises over Bluetooth LE.

The advertising data contains several fields to facilitate provisioning.
It contains the UUID of the Wi-Fi Provisioning Service and four bytes of service data, as described in the following table.

+----------+----------+----------+----------+
|  Byte 1  |  Byte 2  |  Byte 3  |  Byte 4  |
+==========+==========+==========+==========+
| Version  | Flag(LSB)| Flag(MSB)|   RSSI   |
+----------+----------+----------+----------+

* Version: 8-bit unsigned integer. The value is the version of the Wi-Fi Provisioning Service sample.

* Flag: 16-bit little endian field. Byte 2 (first byte on air) is LSB, and Byte 3 (second byte on air) is MSB.

  * Bit 0: provisioning status. The value is ``1`` if the device is provisioned, otherwise it is ``0``.

  * Bit 1: connection status. The value is ``1`` if Wi-Fi is connected, otherwise it is ``0``.

  * Bit 2 - 15: RFU

* RSSI: 8-bit signed integer. The value is the signal strength. It is valid only if the Wi-Fi is connected.

The advertising interval depends on the provisioning status.
If the device is not provisioned, the interval is 100 ms. If it is provisioned, the interval is 1 second.

Configuration management
************************

The configuration management component manages Wi-Fi configurations.
It uses the wifi_credentials library to handle the configurations in flash.
The component has one slot in RAM to save the configurations.

You can save the configuration in flash or RAM during provisioning.
