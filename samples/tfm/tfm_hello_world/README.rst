.. _tfm_hello_world:

TF-M Hello World
################

.. contents::
   :local:
   :depth: 2

A simple sample based on Hello World that demonstrates adding Trusted Firmware-M (TF-M) to an application.

Overview
********

This sample uses the Platform Security Architecture (PSA) API to calculate a SHA-256 digest.
The PSA API call is handled by the TF-M secure firmware.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns

Building and Running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_hello_world`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample, the following output is displayed in the console:

.. code-block:: console

    Hello World! nrf5340dk_nrf5340_cpuapp
    SHA256 digest:
    d87280aef6d36814af7cd864bc7cf28f
    759f8d33c97459dd28eacee7133cb60c

Dependencies
*************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* ``modules/tee/tfm/``
