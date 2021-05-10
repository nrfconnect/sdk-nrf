.. _protected_storage:

PSA Protected Storage
#####################

.. contents::
    :local:
    :depth: 2

This sample demonstrates how the Protected Storage (PS) API can be used for storing data to non-volatile storage.

Overview
********

Using the Platform Security Architecture (PSA) PS API, this sample stores data to non-volatile storage.
The sample shows how data can be stored to and read from UIDs, and how overwrite protection can be enabled using flags.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuappns, nrf9160dk_nrf9160ns

Building and Running
********************

.. |sample path| replace :: :file:`samples/tfm/protected_storage`

.. include:: /includes/build_and_run.txt

Note that because of the overwrite protection, the board needs to be completely erased before programming for the sample to work.

Testing
=======

After programming the sample, the following output is displayed in the console:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v2.5.0-2791-g5585355dde0c  ***
    TF-M Protected Storage sample started. PSA Protected Storage API Version 1.0
    Writing data to UID1: The quick brown fox jumps over the lazy dog
    Info on data stored in UID1:
    - Size: 16
    - Capacity: 0x42
    Read and compare data stored in UID1
    Data stored in UID1: The quick brown fox jumps over the lazy dog
    Overwriting data stored in UID1 with: Lorem ipsum dolor sit amet
    Removing UID1
    Writing data to UID2 with overwrite protection: The quick brown fox jumps over the lazy dog
    Attempting to write 'The quick brown fox jumps over the lazy dog' to UID2
    Got expected error (PSA_ERROR_NO_PERMITTED) when writing to protected UID

Dependencies
************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* ``modules/tee/tfm/``
