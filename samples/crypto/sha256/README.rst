.. _crypto_sha256:

Crypto: SHA-256
###############

.. contents::
   :local:
   :depth: 2

The SHA-256 sample shows how to calculate and verify hashes.

Overview
********

The sample follows these steps:

1. First, the sample initializes the Platform Security Architecture (PSA) API.

#. Then, it calculates and verifies a SHA-256 hash on a sample plaintext:

   a. The SHA-256 hash is calculated.
   #. The SHA-256 hash is verified.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_ns, nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160_ns, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. include:: /includes/tfm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/sha256`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
