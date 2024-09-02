.. _wifi_prov_readme:

Wi-Fi Provisioning Service
##########################

.. contents::
   :local:
   :depth: 2

This library implements a Bluetooth® GATT service for Wi-Fi® provisioning.
It is to be used with the :ref:`wifi_provisioning` sample.
The Wi-Fi Provisioning Service forms a complete reference solution, together with the mobile application.
For details, see the :ref:`wifi_provisioning` sample documentation.

Overview
********

The service is divided into three parts:

* GATT service interface: Defines GATT service and provides an interface for the configurator to exchange messages.
* Task and event handling component: Handles provisioning-related tasks and events.
* Configuration management component: Manages the provisioning data (in RAM and flash) accessed by multiple threads.

Service declaration
*******************

The Wi-Fi Provisioning Service is instantiated as a primary service.
Set the service UUID value as defined in the following table.

========================== ========================================
Service name               UUID
Wi-Fi Provisioning Service ``14387800-130c-49e7-b877-2881c89cb258``
========================== ========================================

Service characteristics
=======================

The UUID value of characteristics are defined in the following table.

========================== ========================================
Characteristic name        UUID
Information                ``14387801-130c-49e7-b877-2881c89cb258``
Operation Control Point    ``14387802-130c-49e7-b877-2881c89cb258``
Data Out                   ``14387803-130c-49e7-b877-2881c89cb258``
========================== ========================================

The characteristic requirements of the Wi-Fi Provisioning Service are shown in the following table.

+-----------------+-------------+-------------+-------------+-------------+
| Characteristic  | Requirement | Mandatory   | Optional    | Security    |
| name            |             | properties  | properties  | permissions |
+=================+=============+=============+=============+=============+
| Information     | Mandatory   | Read        |             | No security |
|                 |             |             |             | required    |
+-----------------+-------------+-------------+-------------+-------------+
| Operation       | Mandatory   | Indicate,   |             | Encryption  |
| Control         |             | Write       |             | required    |
| Point           |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+
| Operation       | Mandatory   | Read, Write |             | Encryption  |
| Control         |             |             |             | required    |
| Point           |             |             |             |             |
| - Client        |             |             |             |             |
| Characteristic  |             |             |             |             |
| Configuration   |             |             |             |             |
| descriptor      |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+
| Data Out        | Mandatory   | Notify      |             | Encryption  |
|                 |             |             |             | required    |
+-----------------+-------------+-------------+-------------+-------------+
| Data Out        | Mandatory   | Read, Write |             | Encryption  |
| - Client        |             |             |             | required    |
| Characteristic  |             |             |             |             |
| Configuration   |             |             |             |             |
| descriptor      |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+

The purpose of each characteristic is as follows:

* ``Information``: For client to get ``Info`` message from server.
* ``Operation Control Point``: For client to send ``Request`` message to server, and server to send ``Response`` message to client.
* ``Data Out``: For server to send ``Result`` message to the client.

Task and event handling
***********************

The Wi-Fi Provisioning Service uses `Protocol Buffers`_ to define platform-independent message formats.
These definitions are available as :file:`.proto` files in the code.

The service also uses `nanopb`_ to convert encoded platform-independent messages to and from C language.

The device performs actions according to messages it exchanges with a configurator (the peer that sends the message, such as a smartphone running the nRF Wi-Fi Provisioner app listed in :ref:`wifi_provisioning_app`).

.. _wifi_provisioning_message_types:
.. _ble_wifi_provision_message_types:

Message types
=============

The service uses four message types:

* ``Info``: Allows the configurator to know the capability of a device before the provisioning process happens.

  The configurator reads this message before sending any other messages.
  The info message contains a ``version`` field.

* ``Request``: Contains a command and configuration.

  The device takes action based on the command and applies the configuration if one is given.

  The following table details the five commands that the service uses.

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

  * ``SUCCESS``: The operation is dispatched successfully.
  * ``INVALID_ARGUMENT``: The argument is invalid.
  * ``INVALID_PROTO``: The message cannot be encoded or decoded.
  * ``INTERNAL_ERROR``: The operation cannot be dispatched properly.

  If the command is ``GET_STATUS``, the response includes some or all of the following fields:

  * ``state``: Describes the Wi-Fi connection state according to values defined in the :file:`common.proto` file.
  * ``provisioning_info``: Includes Wi-Fi provisioning information stored in the non-volatile memory (NVM) of the device.
  * ``connection_info``: Includes Wi-Fi connection details.
  * ``scan_info``: Includes the parameters used for the AP scan.

* ``Result``: Carries asynchronous data about the operation status.

  * If the command is ``START_SCAN``, the result message sent to the configurator contains information about the AP.
    Each ``Result`` contains information related to one AP.
  * If the command is ``SET_CONFIG``, when the Wi-Fi status changes (for example, from disconnected to connected), the configurator receives a result message with the new status.
    Meanwhile, the Wi-Fi credentials are stored in the non-volatile memory of the device.

See all definitions in the :file:`.subsys/bluetooth/services/wifi_prov/proto/common.proto` file.

Operations
==========

The message sequence is the same for all operations, with variations depending on the command:

1. The configurator sends an encoded ``Request`` message with a command to the device.
#. The device carries out the command.
#. The device sends a ``Response`` message to the configurator.
#. For some operations, the device also sends a ``Result`` message to the configurator with any additional data generated or reported.

See :ref:`wifi_provisioning_message_types` for more information on the commands and additional parameters.

Configuration management
************************

The configuration management component manages Wi-Fi configurations.
It uses the :ref:`lib_wifi_credentials` library to handle the configurations in flash.
The component has one slot in RAM to save the configurations.

You can save the configuration in flash or RAM during provisioning.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/wifi_provisioning.h`
| Source files: :file:`subsys/bluetooth/services/wifi_prov`

.. doxygengroup:: bt_wifi_prov
   :project: nrf
   :members:
