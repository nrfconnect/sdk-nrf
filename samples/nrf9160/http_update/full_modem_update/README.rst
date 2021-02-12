.. _http_full_modem_update_sample:

nRF9160: HTTP full modem update
###############################

.. contents::
   :local:
   :depth: 2

The HTTP full modem update sample demonstrates how to perform a full firmware update of the modem (as opposed to a delta update).
The sample downloads a modem firmware signed by Nordic and performs the firmware update of the modem.

Overview
********

An external flash memory with a minimum of 2MB of free space is required to perform a full modem update.
Hence, only versions 0.14.0 and later of the nrf9160 DK support this sample as the prior versions do not have any external flash memory.

The sample connects to a remote HTTP server to download a signed version of the modem firmware, using the :ref:`lib_fota_download` library.
It then writes the firmware to the external flash memory.
After the modem firmware has been stored in the external flash memory, the sample will use the :ref:`lib_fmfu_fdev` to pre-validate the update and program it to the modem.

Two different versions can be downloaded, depending on what version is currently installed.
The version which is not currently installed is selected.

Requirements
************

The sample supports the following development kit, version 0.14.0 or higher:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/application_update/full_modem_update`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the ``nrf9160dk_nrf9160ns`` build target.
Because of this, it automatically includes the :ref:`secure_partition_manager`.

Testing
=======

After programming the sample to the development kit, test it by performing the following steps:

1. Start the application and wait for a prompt for pressing a button.
#. Press the button to start the update procedure.
#. Once the download is completed, the modem update procedure will begin automatically.
#. Reset the development kit.
#. Observe that the LED pattern has changed.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_fota_download`
  * :ref:`lib_dfu_target`
  * :ref:`lib_dfu_target_full_modem_update`
  * :ref:`lib_fmfu_fdev`
  * :ref:`secure_partition_manager`

From nrfxlib
  * :ref:`nrfxlib:nrf_modem`

From Zephyr
  * :ref:`zephyr:flash_api`
  * :ref:`zephyr:logging_api`
  * :ref:`zephyr:gpio_api`
