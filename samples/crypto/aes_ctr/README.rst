.. _crypto_aes_ctr:

Crypto: AES CTR
###########

.. contents::
   :local:
   :depth: 2

Overview
********

The AES sample shows how to perform AES encryption and decryption operations using the CTR mode without padding using an 128 bit AES key.

It does the following things:

* Encrypts a sample plaintext:
   * Initalizes the PSA apis
   * Imports the AES key for encryption to the PSA keystore
   * Generates a random IV
   * Performs the encryption
   * Removes the key from the PSA keystore

* Decrypts the sample plaintext:
   * Initalizes the PSA apis
   * Imports the AES key for decryption to the PSA keystore
   * Imports the IV produced during the encryption process
   * Performs the decryption
   * Removes the key from the PSA keystore

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_ctr`

.. include:: /includes/build_and_run.txt

Testing
=======================

The following process is applicable for testing the AES CTR example

1. Skip this step if you are using an RTT Viewer. By default, the sample is configured to use both RTT and UART for logging.
   Start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

   * Baud rate: 115.200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.
