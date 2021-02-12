.. _http_modem_delta_update_sample:

nRF9160: HTTP modem delta update
################################

.. contents::
   :local:
   :depth: 2

The HTTP modem delta update sample demonstrates how to do a delta update of the modem firmware.
A delta update is an update that contains only the code that has changed, not the entire firmware.

The sample uses the :ref:`lib_fota_download` library to download a file from a remote server and write it to the modem.

Overview
********

The sample connects to an HTTP server to download a signed modem delta image.
The delta images are part of the official modem firmware releases.
The sample will automatically download the correct delta image to go from the base version to the test version, or the test version to the base version, depending on the currently installed version.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/http_update/modem_delta_update`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the ``nrf9160dk_nrf9160ns`` build target.
Because of this, it automatically includes the :ref:`secure_partition_manager`.

Testing
=======

After programming the sample to your development kit,, test it by performing the following steps:

1. Note the LED pattern (1 or 2 LEDs).
#. Press button 1 to download the delta for the alternative firmware version.
#. Once the download has been completed, follow the reboot instructions printed to the UART console.
#. Observe that the LED pattern has changed (1 vs 2).
#. Start over from point 1, to perform the delta update back to the previous version.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_fota_download`
* :ref:`secure_partition_manager`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:flash_api`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:gpio_api`
