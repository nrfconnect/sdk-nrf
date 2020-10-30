.. _uart_cobs_sample:

UART Consistent Overhead Byte Stuffing
######################################

The UART Consistent Overhead Byte Stuffing (COBS) sample demonstrates the capabilities of the :ref:`doc_uart_cobs` module.
The sample shows automatic COBS encoding and decoding, managing multiple module users and use of RX/TX timeouts.

Overview
********

The sample implements a simple two-way communication between two devices.
Pressing a button on the development board causes that device to send a COBS-encoded message containing the string "PING" over UART.
After receiving "PING", the other device responds with a COBS-encoded message containing the string "PONG".

Requirements
************

* Either, two of the following development boards:
.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840
* Or one of the following development boards:
.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

* If using two :ref:`nRF52840 DK <ug_nrf52>`, the UART pins for each board must be wired together externally using the GPIO pins.
  Check the board configuration for information about which pins are used.
* If using one :ref:`nRF9160 DK <ug_nrf9160>`, the on-board devices are connected internally, so no external wiring is required.

User interface
**************

Button 1:
   * Send a COBS-encoded message containing "PING" over UART.

Building and running
********************
.. |sample path| replace:: :file:`samples/peripheral/uart_cobs`

.. include:: /includes/build_and_run.txt

.. note:: If using the :ref:`nRF9160 DK <ug_nrf9160>`, each of the ``nrf9160dk_nrf9160ns`` and ``nrf9160dk_nrf52840`` targets must be built and programmed to the board.

Testing
=======

After programming the sample to your development kit(s), test it by performing the following steps:

1. Connect to the logging output of each of the devices.
2. On one of the boards, press Button 1 as indicated in the log output.
3. Observe that the text 'PING: sending "PING"' is printed in the log output of that board.
4. Observe that the text 'PONG: received "PING"' and 'PONG: sending "PONG"' is printed in the log output of the other board.
5. Observe that the text 'PING: received "PONG"' is printed in the log output of the first board.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`doc_uart_cobs`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:device_model_api`
* :ref:`zephyr:polling_v2`
