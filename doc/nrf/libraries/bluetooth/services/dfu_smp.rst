.. _dfu_smp_readme:

GATT DFU SMP Service Client
###########################

.. contents::
   :local:
   :depth: 2

This library implements a Simple Management Protocol (SMP) Service Client that you can use in the context of Device Firmware Updates (DFU).
SMP is a basic transfer encoding for use with the `mcumgr`_ management protocol.
See `SMP over Bluetooth`_ for the service specification.

You can use the SMP Client library to interact with Zephyr's :zephyr:code-sample:`smp-svr`.

The SMP Client implements only the service.
It does not provide any functionality to process or interpret SMP commands and responses.

The SMP Service Client is used in the :ref:`bluetooth_central_dfu_smp` sample.

Service UUID
************

The 128-bit service UUID is ``8D53DC1D-1DB7-4CD3-868B-8A527460AA84`` (16-bit offset: ``0x0001``).

Characteristics
***************

This service has one characteristic that is used for both requests and responses.

SMP Characteristic (``DA2E7828-FBCE-4E01-AE9E-261174997C48``)
=============================================================

Write Without Response
   * Write an SMP command with data.

Notify
   * Response to the SMP command is sent using notification.
   * Multiple notifications may be generated if the response does not fit in a single MTU.

Usage
*****

.. note::
   Do not access any of the values in the :c:struct:`bt_dfu_smp` object structure directly.
   All values that should be accessed have accessor functions.
   The reason that the structure is fully defined is to allow the application to allocate the memory for it.

Command fragmentation
=====================

The client automatically splits an SMP command that exceeds the usable ATT payload into consecutive Write Without Response packets.
The server reassembles the packets into one SMP request and must enable the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY` Kconfig option.

The usable payload of each packet is the negotiated ATT MTU minus the three-byte ATT header.
A larger ATT MTU reduces the number of packets and might improve throughput.

Sending a command
=================

To send a command, use the :c:func:`bt_dfu_smp_command` function.
The command is provided as a raw binary buffer consisting of a :c:struct:`bt_dfu_smp_header` and the payload.
The client expects one SMP response for the complete command.

Processing the response
=======================

The response to a command is sent as a notification.
It is passed to the callback function that was provided when issuing the command with the :c:func:`bt_dfu_smp_command` function.

Use the :c:func:`bt_dfu_smp_rsp_state` function to access the data of the current part of the response.
As the response might be received in multiple notifications, use the :c:func:`bt_dfu_smp_rsp_total_check` function to verify if this is the last part of the response.
The offset size of the current part and the total size are available in fields of the :c:struct:`bt_dfu_smp_rsp_state` structure.

Resetting the client state
==========================

If the connection is lost while a command is outstanding, use the :c:func:`bt_dfu_smp_reset` function to clear the pending response state.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/dfu_smp.h`
| Source file: :file:`subsys/bluetooth/services/dfu_smp.c`

.. doxygengroup:: bt_dfu_smp
