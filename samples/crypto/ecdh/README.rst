.. _crypto_ecdh:

Crypto: ECDH
############

The ECDH sample shows how to perform an Elliptic-curve Diffie–Hellman key exchange to allow two parties to obtain a shared secret.

Overview
********

The sample follows these steps to perform an ECDH key exchange:

1. First, the sample performs initialization:

   a. The Platform Security Architecture (PSA) API is initialized.
   #. Two random Elliptic Curve Cryptography (ECC) key pairs are generated and imported into the PSA crypto keystore.

#. Then, ECDH key exchange is performed using the generated key pairs.

#. Afterwards, the sample performs cleanup:

   a. The key pairs are removed from the PSA crypto keystore.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_ns, nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160_ns, nrf9160dk_nrf9160, nrf52840dk_nrf52840

.. include:: /includes/tfm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdh`

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
