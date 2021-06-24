.. _hw_unique_key_usage:

Hardware Unique Key
###################

.. contents::
   :local:
   :depth: 2

This sample shows how to use a hardware unique keys (HUKs) to derive a key and use it for encryption, through the psa_crypto APIs.

Overview
********

The sample does the following:

* Initialize the HW if applicable
  ** When TFM is enabled, it does this automatically on boot
* Generate HUKs if applicable
  ** When TFM is enabled, it does this automatically on boot
* Generate a random IV
* Derive a key
  ** When TFM is enabled, this is done using the psa_crypto APIs
  ** When TFM is not enabled, native APIs are used, and the key is imported into psa_crypto.
* Encrypt a sample string using the key_id received from psa_crypto.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160, nrf9160dk_nrf9160ns, nrf52840dk_nrf52840

Building and Running
********************

.. |sample path| replace:: :file:`samples/keys/hw_unique_key`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Observe that the following output (IV, key, and ciphertext will vary):

         .. code-block:: console

            Generating random IV
            IV:
            ab8e7c595d6de7d297a00b6c

            Deriving key
            Key:
            8d6e8ad32f5dffc10df1de38a2556ba0e01cf4ed56ac1294b9c57965cddc519a
            Key ID: 0x7fffffe0

            Encrypting
            Plaintext:
            "Lorem ipsum dolor sit amet. This will be encrypted."
            4c6f72656d20697073756d20646f6c6f722073697420616d65742e2054686973
            2077696c6c20626520656e637279707465642e
            Ciphertext (with authentication tag):
            ea696bf71e106f7c74adfc3296556f4f25ac2c999e453e28c52fb41085ef7b89
            3cbadee1a505cf3ce1901f4bc2fcca4fb86ec68e4b5f1344bb66ef5ce733f47a
            33788a

         If an error occurs, the sample will print a message and it will raise a kernel panic.

Dependencies
*************

This sample uses the following libraries:

* :ref:`lib_hw_unique_key`
* :ref:`nrf_security`
* :ref:`ug_tfm`
