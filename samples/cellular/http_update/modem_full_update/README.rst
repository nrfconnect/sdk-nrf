.. _http_modem_full_update_sample:

Cellular: HTTP full modem update
################################

.. contents::
   :local:
   :depth: 2

The HTTP full modem update sample shows how to perform a full firmware update of the modem.
The sample downloads the modem firmware signed by Nordic Semiconductor and updates the firmware.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

Overview
********

An |external_flash_size| of free space is required to perform a full modem update.
Hence, for the nRF9160 DK, version 0.14.0 or higher is supported by the sample as the earlier versions do not have any external flash memory.
The nRF9161 DK has external flash memory.

The sample proceeds as follows:

1. It connects to a remote HTTP server to download a signed version of the modem firmware, using the :ref:`lib_fota_download` library.
#. It writes the modem firmware to the external flash memory.
#. It prevalidates the update if the firmware supports the prevalidation process.
#. It then programs the update to the modem, using the :ref:`lib_fmfu_fdev` library.

The current version of this sample downloads two different versions of the firmware.
The sample then selects the version to install based on the currently installed version.

Configuration
*************

|config|

Configuration options
=====================

You can customize the firmware files downloaded by the sample through the following Kconfig options in the :file:`prj.conf` file:

.. _CONFIG_DOWNLOAD_HOST:

CONFIG_DOWNLOAD_HOST
   Sets the hostname of the server where the updates are located.

.. _CONFIG_DOWNLOAD_MODEM_0_FILE:

CONFIG_DOWNLOAD_MODEM_0_FILE
   Sets the file name of the first firmware.
   It supports files encoded in the serialized :file:`.cbor` format.
   See :ref:`lib_fmfu_fdev_serialization` for additional information.

.. _CONFIG_DOWNLOAD_MODEM_0_VERSION:

CONFIG_DOWNLOAD_MODEM_0_VERSION
   Sets the version of the first firmware.

.. _CONFIG_DOWNLOAD_MODEM_1_FILE:

CONFIG_DOWNLOAD_MODEM_1_FILE
   Sets the file name of the second firmware.
   It supports files encoded in the serialized :file:`.cbor` format.
   See :ref:`lib_fmfu_fdev_serialization` for additional information.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/http_update/full_modem_update`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. Start the application and wait for a prompt for pressing a button.
#. Press the **Button 1** button or type "download" in the terminal emulator to start downloading the update.
   Wait for the download to complete.
#. Press the **Button 2** button or type "apply" in the terminal emulator to apply the update.
   Once the modem update procedure is complete, the modem is reinitialized to run the new firmware.
#. Observe that the LED pattern has changed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_fota_download`
* :ref:`lib_dfu_target`
* :ref:`lib_dfu_target_full_modem_update`
* :ref:`lib_fmfu_fdev`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It also uses the following Zephyr libraries:

  * :ref:`zephyr:flash_api`
  * :ref:`zephyr:logging_api`
  * :ref:`zephyr:gpio_api`
  * :ref:`zephyr:shell_api`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
