.. _tfm_hello_world:

TF-M Hello World
################

.. contents::
   :local:
   :depth: 2

A simple sample based on Hello World that demonstrates adding Trusted Firmware-M (TF-M) to an application.

Overview
********

This sample uses the Platform Security Architecture (PSA) API to calculate a SHA-256 digest and the TF-M platform read service to read two FICR registers.
The PSA API call is handled by the TF-M secure firmware.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_ns, nrf9160dk_nrf9160_ns

Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_hello_world`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample, the following output is displayed in the console:

.. code-block:: console

    Hello World! nrf5340dk_nrf5340_cpuapp
    Generating random number
    0x8aefe6b7473f2e2c170bbd3eb39aa7679bc1e7693a11030b0a4c1c8ba41eb457
    Reading some secure memory that NS is allowed to read
    FICR->INFO.PART: 0x00005340
    FICR->INFO.VARIANT: 0x514b4141
    Hashing 'Hello World! nrf5340dk_nrf5340_cpuapp'
    SHA256 digest:
    0x12f0c84eecba8497cc0bec1ebc5a785df2ae027a2545921d6cdc0920c5aaefd7
    Configuring MCU selection for LFXO

Dependencies
*************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* ``modules/tee/tfm/``

This sample uses the following libraries:

* :ref:`lib_tfm_ioctl_api`
