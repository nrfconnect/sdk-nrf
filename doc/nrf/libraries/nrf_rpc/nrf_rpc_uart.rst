.. _nrf_rpc_uart:

nRF RPC UART transport
######################

.. contents::
   :local:
   :depth: 2

The nRF RPC UART transport allows you to use the :ref:`nrf_rpc` library to execute procedures on a remote processor that is connected with the local processor using the UART interface.
This transport is used in the :ref:`nrf_rpc_ps_samples`.

Configuration
*************

Use the :kconfig:option:`CONFIG_NRF_RPC_UART_TRANSPORT` Kconfig option to enable the nRF RPC UART transport.

Define the ``nordic,rpc-uart`` property in a devicetree overlay file.
The property is used to select the UART peripheral that is used by the API serialization libraries within the |NCS|, such as :ref:`ble_rpc` or :ref:`nfc_rpc`.

.. code-block:: devicetree

   / {
      chosen {
         nordic,rpc-uart = &uart1;
      };
   };

Frame encoding
**************

.. note::

   The current frame format is experimental and is a subject to change.

An nRF RPC packet that is sent using the nRF RPC UART transport is encoded within a frame whose format resembles the one used by the HDLC protocol:

* Each frame shall start and end with the **delimiter** octet (``0b01111110`` = ``0x7e``).
* Each two subsequent frames may be separated by more than one delimiter octet.
* Each byte of the nRF RPC packet shall be encoded according to the following rules:

  * If the byte matches one of the :ref:`special octets <nrf_rpc_uart_special_octets>`, it shall be encoded as the following two octets:

    * the **escape** octet (``0x7d``),
    * the input byte XORed with ``0x20``.

  * Otherwise, the byte shall be encoded into a frame’s octet without changes.

* The last two bytes of the frame contain the nRF RPC packet checksum, in little-endian byte order.
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

Reliability
***********

When the :kconfig:option:`CONFIG_NRF_RPC_UART_RELIABLE` Kconfig option is selected, the nRF RPC UART transport enables the reliability feature.

The reliability feature introduces the following changes to the transport protocol:

* The receiver of a valid frame acknowledges the frame by replying to the sender with the frame's checksum field.
* If a sender has not received an acknowledgment within a certain time, it retransmits the frame.
  The time (in milliseconds) is defined using the :kconfig:option:`CONFIG_NRF_RPC_UART_ACK_WAITING_TIME` Kconfig option.
* If the sender has not received an acknowledgment after a certain number of attempts, it gives up and reports the transmission error.
  The number of attempts is defined using the :kconfig:option:`CONFIG_NRF_RPC_UART_TX_ATTEMPTS` Kconfig option.
* The frame's checksum field is composed of two values:

  * the most significant bit is the sequence bit that is flipped by the sender for each new transmission.
  * the remaining bits are 15 least significant bits of the nRF RPC packet checksum.

* If the received frame has the same checksum field as the previous one, it is rejected as a duplicate.

API documentation
*****************

| Header file: :file:`include/nrf_rpc/nrf_rpc_uart.h`
| Source file: :file:`subsys/nrf_rpc/nrf_rpc_uart.c`

.. doxygengroup:: nrf_rpc_uart
