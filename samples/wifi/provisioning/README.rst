.. _wifi_provisioning_sample:

Wi-Fi Provisioning Service Sample
#################################

.. contents::
   :local:
   :depth: 2

This sample demnstrates how to provision device with
Nordic Semiconductor's Wi-Fi chipsets over BLE.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an Android or iOS device with nRF Wi-Fi Provisioner
application installed.

Overview
********

The goal of Wi-Fi Provisioning Service is to provide a method that allows
user to provision Wi-Fi device, which typically lacks input/output capability.

The service can be largely divided into three parts. First, we have a component
that handles provisioning-related tasks and events. Second, a transport layer
is used to exchange information between the device and the configurator. Third,
as the Provisioning data need to be stored properly in RAM and flash, and will
be accessed by multiple threads, a separate component is provided to handle
these tasks.

Implementation
**************

Task and Event Handling
=======================

To exchange command and response, we need to define the format of message.
In this implementation, Protocol Buffers is used to achieve the goal.
It plays two roles. First, we define the format of message in language of
Protocol Buffers, and the outcome is .proto files. Second, Protocol Buffers
provides an unified message encoding/decoding scheme, so it allows us to
exchange platform-independent byte stream between device and configurator,
on which different programming language can be used.

The message Info is to allow the configurator to know the capability of device
before actual provisioning process happens. The configurator should read
this message before send any other message. Currently, there is only a field
of version inside.

The messages Request and Response are the main messages we use in provisioning
process. The Request message contains command and configuration. The device is
supposed to take proper action based on the command. If configuration is
provided, the device should also apply. The Response message is used to reply
to the Request. It has the command it is responding, a field that indicates
the result of related operation, and other data if any. The Request-Response
pair is exchanged in asynchronous matter. It means that as long as the Request
is decoded and parsed, and corresponding task is dispatched, the Response message
is sent immediately based on the result. The data associated with this command
may be sent to configurator after the Response message. Currently, we defined
5 commands.

=================== ================================================ =============================
Command             Task                                             Notes
=================== ================================================ =============================
GET_STATUS          to get the status of the device, like
                    provisioning state, Wi-Fi connection state, etc
START_SCAN          to trigger AP scan                               scan parameter is optional
STOP_SCAN           to stop AP scan
SET_CONFIG          to store Wi-Fi configuration and connect to      connection param is required
                    that AP
FORGET_CONFIG       to erase the configuration and disconnect
=================== ================================================ =============================

Based on the result, the Response message contains one of following response code.

=================== =================================================
Response code       Interpretation
=================== =================================================
SUCCESS             the operation is dispatched successfully
INVALID_ARGUMENT    the argument in Request is invalid
INVALID_PROTO       the Protocol Buffers message cannot be encoded or
                    decoded correctly
INTERNAL_ERROR      the task cannot be dispatched properly
=================== =================================================

The message Result is used to carry data sent in asynchronous way. It is
used in two scenarios. First, when the command is START_SCAN, and the scan
process is done, the information of AP will be sent to configurator using
Result message. Each Result contains one AP information. Another cases is
the command of SET_CONFIG. When the status of Wi-Fi changes (e.g., from
disconnected to associated), the new status will be sent to configurator
using Result message.

GET_STATUS procedure
--------------------
When request with Op Code of GET_STATUS is received, the device shall
retrieve current device status information, and the result is put into
the device_status field in the response message and sent to configurator.

The device_status contains four optional fields. The device may fill
part or all of them.

The state field describes the Wi-Fi connection state, whose possible
value is defined in common.proto. If the device meets an error when
retrieving this information, the status of response message is
INTERNAL_ERROR, and the whole device_status shall be discarded by the
configurator.

The provisioning_info field described the Wi-Fi provisioning information
stored in the device's non-volatile memory (NVM). If there is no stored
data, this field shall be absent. If the device meets an error when
reading stored value, the status of response message is INTERNAL_ERROR,
and the whole device_status shall be discarded by the configurator.

The connection_info field describes the details of Wi-Fi connection. If
the device meets an error when getting value, the status of response
message is INTERNAL_ERROR, and the whole device_status shall be
discarded by the configurator.

The scan_info field describes the parameter used for AP scan. If the
device meets an error when getting value, the status of response message
is INTERNAL_ERROR, and the whole device_status shall be discarded by the
configurator.

START_SCAN procedure
--------------------

When request with Op Code of START_SCAN is received, the device shall
send a command of start scan to Wi-Fi subsystem, and depending on the result,
a response message is sent to configurator.

If the scan_params in request message is present, it is used to control
the scan. If the operation is successfully dispatched, the device shall
send response message with status of STATUS_SUCCESS. If invalid argument
is provided, the status shall be STATUS_INVALID_ARGUMENT. If internal
error occurs, the status shall be STATUS_INTERNAL_ERROR.

If the operation is successful, the device shall send the scan result.
The message shall be in type of Result defined in result.proto and encoded.
The information of access point shall be in the scan_record field.
In each sending function call, it should put only one scan result.

STOP_SCAN procedure
-------------------

When request with Op Code of STOP_SCAN is received, the device shall
send a command of stop scan to Wi-Fi subsystem, and depending on the result,
a response message is sent to configurator.

If the operation is successful, the device shall send response message
with status of STATUS_SUCCESS. If internal error occurs, the status
shall be STATUS_INTERNAL_ERROR.

SET_CONFIG procedure
--------------------

When request with Op Code of SET_CONFIG is received, the device shall
send a command of connecting to the Access Point specified in the config
field to Wi-Fi subsystem, and depending on the result, a response message
is sent to configurator.

If the operation is successfully dispatched, the device shall send
response message with status of STATUS_SUCCESS. If invalid argument is
provided, the status shall be STATUS_INVALID_ARGUMENT. If internal error
occurs, the status shall be STATUS_INTERNAL_ERROR.

If the operation is successful, the device shall send state update to configurator.
The message shall be in type of Result defined in result.proto and encoded.
The updated state of Wi-Fi should be in state field. If the state is CONNECTION_FAILED,
the device may fill reason field, whose value depends on the failure reason. If the state is
CONNECTED, the device shall store the credentials in NVM.

FORGET_CONFIG procedure
-----------------------

When request with Op Code of FORGET_CONFIG is received, the device shall
remove stored credentials from NVM, and depending on the result, a response message
is sent to configurator.

If the operation is successful, the device shall send response message
with status of STATUS_SUCCESS. And the device shall disconnect from
access point if its state is connected. If internal error occurs, the
status shall be STATUS_INTERNAL_ERROR.

You may find the details of definition in .proto files under path of ./common/proto.

Message Delivery
================

To exchange messaged mentioned above, some sort of transport method is required. Besides,
it should also provide a certain level of security. In our implementation, Bluetooth Low
Energy (BLE) is chosen. It is widely deployed and available on most smartphones, which
play the role of configurator. Meanwhile, we leverage its capability of information
confidentiality and integrity. BLE itself has pairing function, and can provide different
level of security. GATT allows us to assign different security level to different
characteristic under the same service, making fine tune of access control possible.

Service Declaration
-------------------

The Wi-Fi Provisioning Service is instantiated as a Primary Service. The service UUID
shall be set to the UUID value as defined in tables below.

========================== ====================================
Service Name               UUID
Wi-Fi Provisioning Service 14387800-130c-49e7-b877-2881c89cb258
========================== ====================================

Service characteristics
-----------------------

The UUID value of characteristics are defined in table below.

========================== ====================================
Characteristic Name        UUID
Information                14387801-130c-49e7-b877-2881c89cb258
Operation Control Point    14387802-130c-49e7-b877-2881c89cb258
Data Out                   14387803-130c-49e7-b877-2881c89cb258
========================== ====================================

The characteristic requirements of the Wi-Fi Provisioning Service are
shown in table below.

+-----------------+-------------+-------------+-------------+-------------+
| Characteristic  | Requirement | Mandatory   | Optional    | Security    |
| Name            |             | Properties  | Properties  | Permissions |
+=================+=============+=============+=============+=============+
| Information     | M           | Read        |             | No Security |
|                 |             |             |             | required    |
+-----------------+-------------+-------------+-------------+-------------+
| Operation       | M           | Indicate,   |             | Encryption  |
| Control         |             | Write       |             | Required    |
| Point           |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+
| Operation       | M           | Read, Write |             | Encryption  |
| Control         |             |             |             | Required    |
| Point           |             |             |             |             |
| - Client        |             |             |             |             |
| Characteristic  |             |             |             |             |
| Configuration   |             |             |             |             |
| descriptor      |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+
| Data Out        | M           | Notify      |             | Encryption  |
|                 |             |             |             | Required    |
+-----------------+-------------+-------------+-------------+-------------+
| Data Out        | M           | Read, Write |             | Encryption  |
| - Client        |             |             |             | Required    |
| Characteristic  |             |             |             |             |
| Configuration   |             |             |             |             |
| descriptor      |             |             |             |             |
+-----------------+-------------+-------------+-------------+-------------+

Information
-----------

This characteristic is used to get Info message.
when read, the Information characteristic gives an encoded
Protocol Buffers message in type of Info defined in version.proto. It is
used by client to determine which version of Wi-Fi provisioning service
is running on this server.

Operation Control Point
-----------------------

This characteristic is used to send Request message and get Response message.
The client shall send an encoded message in type of Request defined in
request.proto, and the server shall send an encoded message in type of
Response defined in response.proto.

Data Out
--------

This characteristic is used to send notifications to the client in format of
Result message.

Advertisement
-------------

After power up, the device will first check if it is provisioned or not.
If so, it will load saved credentials and try to connect the AP
specified in the credential. After 30 seconds (timeout will occur if the
attempt fails), it will start BLE advertising. If it is not provisioned,
it will advertise immediately.

The advertising data contains several fields to facilitate possible
provisioning attempt. It will contain the UUID of Wi-Fi provisioning
service, and a 4-byte service data.

+----------+----------+-----------+----------+
|  Byte 1  |  Byte 2  |   Byte 3  |  Byte 4  |
+==========+==========+===========+==========+
| Version  |         Flag         |   RSSI   |
+----------+----------------------+----------+

Version: 8-bit unsigned integer. The value is the version of Wi-Fi
provisioning service.

Flag: 16-bit little endian field. Byte 2 (first byte on air) is LSB, and
Byte 3 (second byte on air) is MSB.

- Bit 0: provisioning status. 1 if device is provisioned, 0 otherwise.

- Bit 1: connection status. 1 if Wi-Fi is connected, 0 otherwise.

- Bit 2 - 15: RFU

RSSI: 8-bit signed integer. If Wi-Fi is connected, it is the signal
strength. Otherwise, it is meaningless.

Depending on the provisioning status, the advertising interval differs.
If it is not provisioned, the interval is 100ms. If it is provisioned,
the interval is 1 second.

Configuration Management
========================

Once the device receives Wi-Fi configuration, it should keep the data in proper location,
and save or delete the record according to incoming command. Therefore, the configuration
management component is to encapsulate these procedures and expose some APIs. It provides
two functions. First, it manages the stored configuration, and make sure the data in RAM
and data in flash are in sync. Second, it provides API to add and remove configuration
safely in a multi-thread environment.

When used in Wi-Fi provisioning task, the main thread will initialize it and try to get
saved configuration. It is applied if there is any. Then, the handler may read, write,
and remove configuration based on the command. And when certain configuration makes Wi-Fi
connection successful, the API that commit this configuration is called to mark it as valid.

This component needs to save data in non-volatile memory (NVM), so related Kconfig options
are enabled. In nRF5x chips, internal flash is used as NVM. As the flash has limited
writing-erasing cycles, developers can disable NVM operations by disabling
CONFIG_WIFI_PROVISIONING_NVM_SETTINGS when building the application. Or the user can also
choose to save the configuration in both RAM and flash, or in RAM only when during
provisioning.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/provisioning`

.. include:: /includes/build_and_run_ns.txt

If you want to generate .pb.h and .pb.c files when building the project,
CONFIG_WIFI_PROVISIONING_PROTO_BUILD_TIME_GENERATION needs to be y. If you
want to use pre-generated .pb.h and .pb.c files, it needs to be n. This Kconfig
option is disabled by default.

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. After initialization, you can see output like::

       wifi_prov: Advertising successfully started

#. Open nRF Wi-Fi Provisioner application, and enbale Bluetooth if it is disabled.
#. Search for and connect to the device you want provision. If connection and pairing
   is done, you can see output like::

       wifi_prov: Connected
       wifi_prov: Pairing Complete

#. After connection and pairing, you can conduct AP scan, input password if required,
   and trigger Wi-Fi connection attempt.
#. If connection succeeds, you can see output like::

       wpa_supp: wlan0: CTRL-EVENT-CONNECTED - Connection
       to <AP MAC Address> completed [id=0 id_str=]

Dependencies
============

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_security`

This sample also uses modules that can be found in the following locations
in the |NCS| folder structure:

* ``modules/lib/nanopb``

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``

* :ref:`zephyr:settings_api`:

  * ``include/settings/settings.h``
