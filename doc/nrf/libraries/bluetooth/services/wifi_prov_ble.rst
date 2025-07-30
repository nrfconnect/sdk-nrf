.. _wifi_prov_readme:
.. _lib_wifi_prov_ble:

Wi-Fi provisioning Bluetooth LE transport
#########################################

.. contents::
   :local:
   :depth: 2

This library implements the Bluetooth® GATT transport layer for the Wi-Fi® provisioning service.
It provides the Bluetooth LE-specific implementation of the transport interface defined by the core Wi-Fi provisioning library.
The Bluetooth LE transport layer is designed to work with the core Wi-Fi provisioning library located at :file:`subsys/net/lib/wifi_prov/` directory.

Overview
********

The Bluetooth LE transport layer is responsible for:

* GATT service interface - Defines the GATT service and serves as the transport layer for the provisioning protocol.
* Bluetooth LE-specific message handling - Implements the transport functions required by the core library.
* Connection management - Manages Bluetooth LE connections and handles characteristic notifications and indications.

Configuration
*************

To use this library, enable the :kconfig:option:`CONFIG_BT_WIFI_PROV` Kconfig option.

Service declaration
*******************

The Wi-Fi Provisioning Service is instantiated as a primary service.
Set the service UUID value as defined in the following table:

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

* ``Information`` - For client to get ``Info`` message from server.
* ``Operation Control Point`` - For client to send ``Request`` message to server, and server to send ``Response`` message to client.
* ``Data Out`` - For server to send ``Result`` message to the client.

Transport interface implementation
**********************************

The Bluetooth LE transport layer implements the transport interface defined by the core Wi-Fi provisioning library:

* :c:func:`wifi_prov_send_rsp` - Sends Response messages through Bluetooth LE indications on the Operation Control Point characteristic.
* :c:func:`wifi_prov_send_result` - Sends Result messages through Bluetooth LE notifications on the Data Out characteristic.

The transport layer also handles:

* Receiving request messages from the Operation Control Point characteristic.
* Providing info messages through the Information characteristic.
* Managing Bluetooth LE connection state and characteristic subscriptions.

Dependencies
************

The Bluetooth LE transport layer depends on:

* :ref:`lib_wifi_prov_core`
* Bluetooth stack (:kconfig:option:`CONFIG_BT`)
* nanopb for protobuf message handling


API documentation
*****************

| Header file: :file:`include/net/wifi_prov_core/wifi_prov_core.h`
| Source files: :file:`subsys/bluetooth/services/wifi_prov`

.. doxygengroup:: bt_wifi_prov
