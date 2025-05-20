.. _http_modem_delta_update_sample:

Cellular: HTTP modem delta update
#################################

.. contents::
   :local:
   :depth: 2

The HTTP modem delta update sample demonstrates how to do a delta update of the modem firmware.
A delta update is an update that contains only the code that has changed, not the entire firmware.

The sample uses the :ref:`lib_fota_download` library to download a file from a remote server and write it to the modem.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample connects to an HTTP server to download a signed modem delta image.
The delta images are part of the official modem firmware releases.
The sample automatically downloads the correct delta image to go from the base version to the test version, or the test version to the base version, depending on the currently installed version.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/http_update/modem_delta_update`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit,, test it by performing the following steps:

1. Note the LED pattern (1 or 2 LEDs).
#. Press **Button 1** on the nRF91 Series DK or type "download" in the terminal emulator to start downloading the delta modem firmware update.
   Wait for the download to complete.
#. Press **Button 1** on the nRF91 Series DK or type "apply" in the terminal emulator to apply the delta modem firmware update.
   Once the modem update procedure is complete, the modem is reinitialized to run the new firmware.
#. Observe that the LED pattern has changed (1 vs 2).
#. Start over from point 1, to perform the delta update back to the previous version.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_fota_download`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:flash_api`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:gpio_api`
* :ref:`zephyr:shell_api`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
