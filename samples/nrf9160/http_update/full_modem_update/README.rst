.. _http_full_modem_update_sample:

nRF9160: HTTP full modem update
###############################

.. contents::
   :local:
   :depth: 2

The HTTP full modem update sample shows how to perform a full firmware update of the modem.
The sample downloads a modem firmware signed by Nordic Semiconductor and then performs the firmware update of the modem.

Overview
********

An |external_flash_size| of free space is required to perform a full modem update.
Hence, only versions 0.14.0 and later of the nrf9160 DK support this sample as the prior versions do not have any external flash memory.

The sample proceeds as follows:

1. It connects to a remote HTTP server to download a signed version of the modem firmware, using the :ref:`lib_fota_download` library.
#. It writes the modem firmware to the external flash memory.
#. It prevalidates the update if the firmware supports the prevalidation process.
#. It then programs the update to the modem, using the :ref:`lib_fmfu_fdev` library.

The current version of this sample downloads two different versions of the firmware, namely 1.3.0 and 1.3.1.
The sample then selects the version which is currently not installed.

Requirements
************

The sample supports the following development kit, version 0.14.0 or higher:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

On the nRF9160 DK, you must set the control signal from the nRF52840 board controller MCU (**P0.19**) to *high* to let the nRF9160 communicate with the external flash memory.
To do so, enable the ``external_flash_pins_routing`` node in devicetree.
See :ref:`zephyr:nrf9160dk_board_controller_firmware` for details on building the firmware for the nRF52840 board controller MCU.

Configuration options
*********************

You can customize the firmware files downloaded by the sample through the following Kconfig options in the :file:`prj.conf` file:

.. _CONFIG_DOWNLOAD_HOST:

CONFIG_DOWNLOAD_HOST
   It sets the hostname of the server where the updates are located.

.. _CONFIG_DOWNLOAD_MODEM_0_FILE:

CONFIG_DOWNLOAD_MODEM_0_FILE
   It sets the file name of the first firmware.
   It supports files encoded in the serialized :file:`.cbor` format.
   See :ref:`lib_fmfu_fdev_serialization` for additional information.

.. _CONFIG_DOWNLOAD_MODEM_0_VERSION:

CONFIG_DOWNLOAD_MODEM_0_VERSION
   It sets the version of the first firmware.

.. _CONFIG_DOWNLOAD_MODEM_1_FILE:

CONFIG_DOWNLOAD_MODEM_1_FILE
   It sets the file name of the second firmware.
   It supports files encoded in the serialized :file:`.cbor` format.
   See :ref:`lib_fmfu_fdev_serialization` for additional information.

See :ref:`configure_application` for more information on how to customize configuration options.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/http_update/full_modem_update`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the ``nrf9160dk_nrf9160_ns`` build target.
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

This sample uses the following |NCS| libraries:

* :ref:`lib_fota_download`
* :ref:`lib_dfu_target`
* :ref:`lib_dfu_target_full_modem_update`
* :ref:`lib_fmfu_fdev`
* :ref:`secure_partition_manager`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It also uses the following Zephyr libraries:

  * :ref:`zephyr:flash_api`
  * :ref:`zephyr:logging_api`
  * :ref:`zephyr:gpio_api`
