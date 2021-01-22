.. _crypto_hkdf:

Crypto: HKDF
############

.. contents::
   :local:
   :depth: 2

Overview
********

The HMAC Key Derivation Function (HKDF) sample shows how to derive keys using a sample key, salt and additional data.

First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.

Then, sample performs key derivation:
   #. The input key is imported to the PSA crypto keystore.
   #. The output key is derived.

Afterwards, the sample performs cleanup:
   #. The input and the output keys are removed from the PSA crypto keystore.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/hkdf`

.. include:: /includes/build_and_run.txt

Testing
=======

Follow these steps to test the HKDF example:

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
