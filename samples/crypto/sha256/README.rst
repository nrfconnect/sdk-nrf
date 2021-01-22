.. _crypto_sha256:

Crypto: SHA256
##############

.. contents::
   :local:
   :depth: 2

Overview
********

The SHA256 sample shows how to calculate and verify hashes.

First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.

Then, the sample calculates/verifies a SHA256 hash on a sample plaintext:
   #. The SHA256 hash is calculated.
   #. The SHA256 hash is verified.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/sha256`

.. include:: /includes/build_and_run.txt

Testing
=======

Follow these steps to test the SHA256 example:

1. Start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

   * Baud rate: 115.200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
