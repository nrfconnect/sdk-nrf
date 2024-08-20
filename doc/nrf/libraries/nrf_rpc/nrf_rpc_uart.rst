.. _nrf_rpc_uart:

nRF RPC UART transport
######################

.. contents::
   :local:
   :depth: 2

The nRF RPC UART transport allows you to use the :ref:`nrf_rpc` library to execute procedures on a remote processor that is connected with the local processor using the UART interface.
This transport is used in the :ref:`nrf_rpc_ps_samples`.

Frame encoding
**************

.. note::

   The current frame format is experimental and is a subject to change.

An nRF RPC packet that is sent using the nRF RPC UART transport is encoded within a frame whose format resembles the one used by the HDLC protocol.

Each frame shall start and end with the **delimiter** octet (``0b01111110`` = ``0x7e``).
Each two subsequent frames may be separated by more than one delimiter octet.

Each byte of the nRF RPC packet shall be encoded according to the following rules:

* If the byte matches one of the :ref:`special octets <nrf_rpc_uart_special_octets>`, it shall be encoded as the following two octets:

  * the **escape** octet (``0x7d``),
  * the input byte XORed with ``0x20``.

* Otherwise, the byte shall be encoded into a frame’s octet without changes.

The last two bytes of the frame contain the nRF RPC packet checksum, in little-endian byte order.
The checksum is calculated using the CRC16_CCITT function with the initial value ``0xffff``.

.. _nrf_rpc_uart_special_octets:

Special octets
==============

The following octets transmitted over the UART interface have a special meaning:

.. table::
   :align: center

   +-----------+-----------+
   | Value     | Meaning   |
   +===========+===========+
   | ``0x7d``  | escape    |
   +-----------+-----------+
   | ``0x7e``  | delimiter |
   +-----------+-----------+

Encoding example
================

If the following nRF RPC packet is sent using the nRF RPC UART transport:

.. code-block:: none

   80 01 ff 00 00 61 7e f6

Then the following octets are transmitted over the UART interface:

.. code-block:: none

   7e 80 01 ff 00 00 61 7d 5e f6 6d 72 7e

API documentation
*****************

| Header file: :file:`include/nrf_rpc/nrf_rpc_uart.h`
| Source file: :file:`subsys/nrf_rpc/nrf_rpc_uart.c`

.. doxygengroup:: nrf_rpc_uart
