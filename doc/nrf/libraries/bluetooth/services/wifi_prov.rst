.. _wifi_prov_readme:

Wi-Fi Provisioning Service
##########################

.. contents::
   :local:
   :depth: 2

This library implements a Bluetooth® GATT service for Wi-Fi® provisioning.
It is an implementation of the server side of the Wi-Fi provisioning protocol defined by Nordic Semiconductor.
It is to be used with the :ref:`wifi_provisioning` sample.
The Wi-Fi Provisioning Service forms a complete reference solution, together with the mobile application.
For details, see the :ref:`wifi_provisioning` sample documentation.

Overview
********

The service is divided into three parts:

* GATT service interface: Defines the GATT service and serves as the transport layer for the provisioning protocol.
* Task and event handling component: Implements the provisioning protocol.
* Configuration management component: Manages the provisioning data (in RAM and flash) accessed by multiple threads.

.. _wifi_provisioning_protocol:

Provisioning protocol
*********************

The protocol is designed to enable the provisioning of Wi-Fi products from Nordic Semiconductor.
It is an application layer protocol, and the underlying transport layer can be arbitrary.
It uses `Protocol Buffers`_ as the serialization scheme, to achieve platform independence.
It adopts a client-server model.

The target, which is the device having Wi-Fi products to be provisioned (for example, nRF5340 DK on nRF7002 DK), hosts the server.
The configurator, which is the device that holds the information to be passed to and applied on the target (for example, a smartphone), acts as the client.
The message is the basic unit for carrying information and is exchanged between the configurator and the target.

Message types
=============

The protocol defines four message types:

* ``Info``: The message is to indicate the ``version`` of the provisioning protocol implemented on the target.

  The following table details the fields of the ``Info`` message.

   =================== ======================= ======================== =======================================================================================
   Name                Data type               Status                   Notes
   =================== ======================= ======================== =======================================================================================
   version             uint32                  Required                 The value represents the version of provisioning protocol implemented on the target.
   =================== ======================= ======================== =======================================================================================

* ``Request``: The message is for the configurator to request a particular action on the target.

  The following table details the fields of the ``Request`` message.

   =================== ======================= ======================== =======================================================================================
   Name                Data type               Status                   Notes
   =================== ======================= ======================== =======================================================================================
   op_code             enum                    Optional                 The value represents the action to be taken by the target.
   scan_params         enum                    Optional                 The value represents the scan parameters used for a scan action.
   config              message                 Optional                 The value represents the Wi-Fi configuration used for a connection action.
   =================== ======================= ======================== =======================================================================================

  The following table details the five possible values of the ``op_code`` field.

   =================== ================================================ =============================
   Operation code      Description                                      Notes
   =================== ================================================ =============================
   GET_STATUS          Get the status of the device, such as
                       provisioning state, Wi-Fi connection state,
                       and more
   START_SCAN          Trigger an access point (AP) scan                ``scan_params`` is optional
   STOP_SCAN           Stop AP scan
   SET_CONFIG          Store Wi-Fi configuration and connect to an AP   ``config`` is required
   FORGET_CONFIG       Erase the configuration and disconnect
   =================== ================================================ =============================

* ``Response``: The message is for the target to provide the feedback on the result of an action to the configurator.

  The following table details the fields of the ``Response`` message.

   =================== ======================= ======================== =======================================================================================
   Name                Data type               Status                   Notes
   =================== ======================= ======================== =======================================================================================
   request_op_code     enum                    Optional                 The action the message is to respond.
   status              enum                    Optional                 The result of the action.
   device_status       message                 Optional                 The status of the Wi-Fi on the target.
   =================== ======================= ======================== =======================================================================================

  The ``status`` field can take one of following values:

  * ``SUCCESS``: The operation is dispatched successfully.
  * ``INVALID_ARGUMENT``: The argument is invalid.
  * ``INVALID_PROTO``: The message cannot be encoded or decoded.
  * ``INTERNAL_ERROR``: The operation cannot be dispatched properly.

* ``Result``: The message is for the target to provide feedback on the Wi-Fi status to the configurator asynchronously.

  The following table details the fields of the ``Result`` message.

   =================== ======================= ======================== =======================================================================================
   Name                Data type               Status                   Notes
   =================== ======================= ======================== =======================================================================================
   scan_record         message                 Optional                 The information of the SSID found in the scan action.
   state               enum                    Optional                 The state of the connection action.
   reason              enum                    Optional                 The reason for the connection failure.
   =================== ======================= ======================== =======================================================================================

These definitions are available as :file:`.proto` files in the library path.
See all definitions in the :file:`subsys/bluetooth/services/wifi_prov/proto/` folder.

Workflow
========

Multiple workflows are defined in the form of message exchange.

Determine provisioning protocol version
---------------------------------------

In this workflow, the configurator requests the ``Info`` message using a transport layer-specific method, and the target sends the ``Info`` message over the transport layer.

Get Wi-Fi status
----------------

In this workflow, the configurator sends a ``Request`` message, in which the ``op_code`` is set as ``GET_STATUS``, over the transport layer.
The target receives the message, retrieves the required information, and sets up a ``Response`` message.
In the ``Response`` message, the ``request_op_code`` is ``GET_STATUS``, the ``status`` indicates whether the operation is successful and the possible failure reason, and ``device_status`` carries the Wi-Fi status if the operation is successful.

Start SSID scan
---------------

In this workflow, the configurator sends a ``Request`` message, in which the ``op_code`` is set as ``START_SCAN``, over the transport layer.
The target receives the message, triggers the scan using the given parameters, and sets up a ``Response`` message.
In the ``Response`` message, the ``request_op_code`` is ``START_SCAN``, and the ``status`` indicates whether the operation is successful and the possible failure reason.

When the scan is finished, the target will set up a ``Result`` message for each SSID found during the scan, and the information will be in the ``scan_record`` field.

Stop SSID scan
--------------

In this workflow, the configurator sends a ``Request`` message, in which the ``op_code`` is set as ``STOP_SCAN``, over the transport layer.
The target receives the message, stops the Wi-Fi scan, and sets up a ``Response`` message.
In the ``Response`` message, the ``request_op_code`` is ``STOP_SCAN``, and the status indicates whether the operation is successful and the possible failure reason.

Connect to AP
-------------

In this workflow, the configurator sends a ``Request`` message, in which the ``op_code`` is set as ``SET_CONFIG``, over the transport layer.
The target receives the message, applies the Wi-Fi connection information, and sets up a ``Response`` message.
In the ``Response`` message, the ``request_op_code`` is ``SET_CONFIG``, and the ``status`` indicates whether the operation is successful and the possible failure reason.

When the connection state changes or an attempt fails, the target will set up a ``Result`` message.
The ``state`` field indicates the current state of the Wi-Fi, and ``reason`` field indicates the failure reason.

Disconnect from AP
------------------

In this workflow, the configurator sends a ``Request`` message, in which the ``op_code`` is set as ``FORGET_CONFIG``, over the transport layer.
The target receives the message, triggers the Wi-Fi disconnection, and sets up a ``Response`` message.
In the ``Response`` message, the ``request_op_code`` is ``FORGET_CONFIG``, and the ``status`` indicates whether the operation is successful and the possible failure reason.

When the connection state changes or an attempt fails, the target will set up a ``Result`` message, and the ``state`` field indicates the current state of the Wi-Fi, and the ``reason`` field indicates the failure reason.

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

It takes the functions exposed by the task and event handling part of reading the ``Info`` message and receiving the ``Request`` message as the callbacks of corresponding characteristics.
It provides functions for the task and event handling part to send ``Response`` and ``Result`` messages.

Task and event handling
***********************

The service uses `nanopb`_ to instantiate the protocol buffers-based, platform-independent messages in the C language.

It exposes the functions of reading the ``Info`` message and  receiving the ``Request`` message to transport layer.
It uses the function of sending ``Response`` and ``Result`` messages provided by the transport layer to send these messages.

Configuration management
************************

The configuration management component manages Wi-Fi configurations.
It uses the :ref:`Wi-Fi credentials <zephyr:lib_wifi_credentials>` library to handle the configurations in flash.
The component has one slot in RAM to save the configurations.

You can save the configuration in flash or RAM during provisioning.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/wifi_provisioning.h`
| Source files: :file:`subsys/bluetooth/services/wifi_prov`

.. doxygengroup:: bt_wifi_prov
