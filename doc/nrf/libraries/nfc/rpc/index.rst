.. _nfc_rpc:

NFC Remote Procedure Call
#########################

.. contents::
   :local:
   :depth: 2

The :ref:`NFC <ug_nfc>` Remote Procedure Call (RPC) solution is a set of libraries that allows using the NFC stack running entirely on a separate device or CPU.

Overview
********

The solution allows calling the NFC API (both :ref:`Type 2 Tag <lib_nfc_t2t>` and :ref:`Type 4 Tag <lib_nfc_t4t>`) on a different CPU or device.
This is accomplished by running the full NFC functionality on one device and serializing the API from another device.
Use this solution when you do not want your firmware to include the NFC stack, for example to offload the application CPU, save memory, or to be able to build your application in a different environment.

Implementation
==============

The NFC RPC solution consists of the following components:

  * NFC RPC Client and common software libraries.
    These libraries serialize the NFC API and enable RPC communication, and need to be part of the user application.
  * NFC RPC Server, common libraries, and the Type 4 Tag library, Type 2 Tag library, or both.
    These libraries enable communication with NFC RPC Client, and need to run on a device or CPU that has an NFC radio hardware peripheral.

You can add support for serializing NFC-related custom APIs by implementing your own client and server procedures.
You can use the following files as examples:

  * :file:`subsys/nfc/rpc/client/nfc_rpc_t2t_client.c`
  * :file:`subsys/nfc/rpc/server/nfc_rpc_t2t_server.c`

Requirements
************

These configuration options must be enabled to use the library:

  * :kconfig:option:`CONFIG_NFC_RPC`
  * :kconfig:option:`CONFIG_NFC_RPC_CLIENT` - for the NFC RPC client
  * :kconfig:option:`CONFIG_NFC_RPC_SERVER` - for the NFC RPC server

These configuration options related to NFC Data Exchange Format must be enabled on the client for the Type 4 Tag:

  * :kconfig:option:`CONFIG_NFC_NDEF`
  * :kconfig:option:`CONFIG_NFC_NDEF_MSG`
  * :kconfig:option:`CONFIG_NFC_NDEF_RECORD`
  * :kconfig:option:`CONFIG_NFC_NDEF_TEXT_RECORD`

These configuration options related to NFC must be enabled on the server:

  * :kconfig:option:`CONFIG_NFC_T2T_NRFXLIB` - for Type 2 Tag
  * :kconfig:option:`CONFIG_NFC_T4T_NRFXLIB` - for Type 4 Tag

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`nrf_rpc_protocols_serialization_client`
* :ref:`nrf_rpc_protocols_serialization_server`

Limitations
***********

The library currently supports serialization of the following:

  * :ref:`nrfxlib:type_2_tag`
  * :ref:`nrfxlib:type_4_tag`

The behavior of NFC with RPC is almost the same as without it, with the following exceptions:

  * Some NFC API functions get data by pointer, for example :c:func:`nfc_t4t_ndef_rwpayload_set`.
    After calling the functions on the client, the data is sent to the server.
    Any manipulation of data over the pointer will not affect the server's data instance.
  * Even though the maximum payload for Type 4 Tag can be 65520 bytes, the real length is limited by :c:macro:`NDEF_FILE_SIZE`.

Dependencies
************

The library has the following dependencies:

  * :ref:`nrf_rpc`
  * :ref:`nfc`

.. _nfc_rpc_api:

API documentation
*****************

This library does not define a new NFC API.

| Header files: :file:`nrfxlib/nfc/include/nfc_t2t_lib.h`, :file:`nrfxlib/nfc/include/nfc_t4t_lib.h`
| Source files: :file:`subsys/nfc/rpc/`
