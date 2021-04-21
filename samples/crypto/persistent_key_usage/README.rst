.. _crypto_persistent_key:

Crypto: AES CTR
###############

.. contents::
   :local:
   :depth: 2

Overview
********

The persistent key sample shows how to generate a persistent key using the PSA APIs. Persistent keys
are stored in the Internal Trusted Storage (ITS) of the device and retain their value between resets.
A persistent key becomes unusuable when the psa_destroy_key function is called.

In this sample we create an AES 128-bit key (CTR block cipher mode) for demonstration purposes.
Persistent keys can be of any type supported by the PSA APIs.

First, the sample performs initalization:
   #. The Platform Security Architecture (PSA) API is initialized.

Then, the sample generates a persistent AES key:
   #. A persistent AES 128-bit key is generated.

Afterwards, the sample performs cleanup:
   #. The AES key is removed from the PSA crypto keystore.


.. note:: There is a read only type of persistent keys which cannot be destroyed with psa_destroy_key.
          The PSA_KEY_PERSISTENCE_READ_ONLY persistence macro is used for read only keys. The key id
          of read only key will be writable again after we perform a full erase of the device memory
          using the `west -v flash --erase`  command.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows:  nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns


Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_ctr`

.. include:: /includes/build_and_run.txt

Testing
=======

Follow these steps to test the AES CTR example:

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
