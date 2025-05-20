.. _hw_unique_key_usage:

Hardware unique key
###################

.. contents::
   :local:
   :depth: 2

This sample shows how to use a hardware unique key (HUK) to derive an encryption key through psa_crypto APIs.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample goes through the following steps to derive a key and use it to encrypt a string:

1. Initializes the hardware, if applicable.

   When TF-M is enabled, the hardware is initialized automatically when booting.

#. Generates HUKs, if applicable.

   The sample generates a random HUK and writes it to the Key Management Unit (KMU) or flash.
   When no KMU is present, the device reboots to allow the immutable bootloader to feed the key into CryptoCell.
   When TF-M is enabled, the HUKs are generated automatically when booting TF-M, and no action is needed in the sample.

#. Generates a random initialization vector (IV).

#. Derives a key used for encryption.

   When TF-M is enabled, the key is derived using the psa_crypto APIs.
   Otherwise, the native nrf_cc3xx_platform APIs are used, and the key is imported into psa_crypto.

#. Encrypts a sample string using the ``key_id`` received from the psa_crypto.

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/keys/hw_unique_key`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. Reset the kit.
#. If not using TF-M, observe the following output:

   .. code-block:: console

      Writing random keys to KMU
      Success!

#. Observe the following output (the values for IV, key, and ciphertext will vary randomly):

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

   If an error occurs, the sample prints a message and raises a kernel panic.

Dependencies
************

This sample uses the following libraries:

* :ref:`lib_hw_unique_key`
* :ref:`nrf_security`
* :ref:`Trusted Firmware-M <ug_tfm>`, when using a board target with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (``*/ns`` :ref:`variant <app_boards_names>`).
