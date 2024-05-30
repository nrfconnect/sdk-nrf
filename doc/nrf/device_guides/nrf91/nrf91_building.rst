.. _ug_nrf91_config_build:

Configuring and building with nRF91 Series
##########################################

.. contents::
   :local:
   :depth: 2

Configuring and building application for the nRF91 Series devices follows the processes described in the :ref:`building` section, with several exceptions specific to the nRF91 Series that are listed below.

.. _nrf9160_board_revisions:

nRF9160 DK board revisions
**************************

nRF9160 DK v0.13.0 and earlier are missing the following hardware features that are available on later versions of the DK:

* External flash memory
* I/O expander

To build without these features, specify the board revision when building your application.

.. note::
   If you do not specify a board revision, the firmware is built for the default revision (v0.14.0).

To specify the board revision, append it to the board argument when building.
The board revision is printed on the label of your DK, just below the PCA number.
For example, when building a non-secure application for nRF9160 DK v0.9.0, use ``nrf9160dk@0.9.0/nrf9160/ns`` as build target.

See Zephyr's :ref:`zephyr:application_board_version` and :ref:`zephyr:nrf9160dk_additional_hardware` pages for more information.

.. _nrf91_fota:
.. _nrf9160_fota:

FOTA updates
************

|fota_upgrades_def|
You can use FOTA updates to apply delta patches to the :ref:`lte_modem` firmware, full :ref:`lte_modem` firmware updates, and to replace the upgradable bootloader or the application.

.. note::
   Even though the Trusted Firmware-M and the application are two individually compiled components, they are treated as a single binary blob in the context of firmware updates.
   Any reference to the application in this section is meant to indicate the application including the Trusted Firmware-M.

To perform a FOTA update, complete the following steps:

1. Make sure that your application supports FOTA updates.

   To download and apply FOTA updates, your application must use the :ref:`lib_fota_download` library.
   This library determines the type of update by inspecting the header of the firmware and invokes the :ref:`lib_dfu_target` library to apply the firmware update.
   In its default configuration, the DFU target library is set to support all the types of FOTA updates except full modem firmware updates, but you can freely enable or disable the support for specific targets.
   In addition, the following requirements apply:

   * To upgrade the application, you must use :doc:`mcuboot:index-ncs` as the upgradable bootloader (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` must be enabled).
   * If you want to upgrade the upgradable bootloader, you must use the :ref:`bootloader` (:kconfig:option:`CONFIG_SECURE_BOOT` must be enabled).
   * If you want to update the modem firmware through modem delta updates, you do not need to use MCUboot or the immutable bootloader, because the modem firmware update is handled by the modem itself.
   * If you want to perform a full modem firmware update, an |external_flash_size| is required.

#. Create a binary file that contains the new image.

   .. note::
      This step does not apply for updates of the modem firmware.
      You can download delta patches and full binaries of the modem firmware from the `nRF9161 product website (compatible downloads)`_ or `nRF9160 product website (compatible downloads)`_, depending on the SiP you are using.

   |fota_upgrades_building|
   The :file:`app_update.bin` file must be uploaded to the server.

   To create binary files for a bootloader upgrade, make sure that the Kconfig options :kconfig:option:`CONFIG_SECURE_BOOT` and :kconfig:option:`CONFIG_BUILD_S1_VARIANT` are enabled and build MCUboot as usual.
   The build will create a binary file for each variant of the upgradable bootloader, one for each bootloader slot.
   See :ref:`upgradable_bootloader` for more information.

#. Make the binary file (or files) available for download.
   Upload the serialized :file:`.cbor` binary file or files to a web server that is compatible with the :ref:`lib_download_client` library.

The full FOTA procedure depends on where the binary files are hosted for download.

FOTA updates using nRF Cloud
============================

You can manage FOTA updates through a comprehensive management portal on `nRF Cloud`_, either fully hosted on nRF Cloud or accessible from a customer cloud using the `nRF Cloud REST API`_.
If you are using nRF Cloud, see the `nRF Cloud Getting Started FOTA documentation`_ for instructions.

Currently, delta modem firmware FOTA files are available in nRF Cloud under :guilabel:`Firmware Updates` in the :guilabel:`Device Management` tab on the left.
If you intend to obtain FOTA files from nRF Cloud, see the additional requirements in :ref:`lib_nrf_cloud_fota`.

You can upload custom application binaries to nRF Cloud for application FOTA updates.
After :ref:`nrf9160_gs_connecting_dk_to_cloud`, you can upload the files to your nRF Cloud account as a bundle after navigating to :guilabel:`Device Management` on the left and clicking :guilabel:`Firmware Updates`.

FOTA updates using other cloud services
========================================

FOTA updates can alternatively be hosted from a customer-developed cloud services such as solutions based on AWS and Azure.
If you are uploading the files to an Amazon Web Services Simple Storage Service (AWS S3) bucket, see the :ref:`lib_aws_fota` documentation for instructions.
Samples are provided in |NCS| for AWS (:ref:`aws_iot` sample) and Azure (:ref:`azure_iot_hub` sample).

Your application must be able to retrieve the host and file name for the binary file.
See the :ref:`lib_fota_download` library documentation for information about the format of this information, especially when providing two files for a bootloader upgrade.
You can hardcode the information in the application, or you can use a functionality like AWS jobs to provide the URL dynamically.

Samples and applications implementing FOTA
==========================================

* :ref:`http_modem_full_update_sample` sample - Performs a full firmware OTA update of the modem.
* :ref:`http_modem_delta_update_sample` sample - Performs a delta OTA update of the modem firmware.
* :ref:`http_application_update_sample` sample - Performs a basic application FOTA update.
* :ref:`aws_iot` sample - Performs a FOTA update using MQTT and HTTP, where the firmware download is triggered through an AWS IoT job.
* :ref:`azure_iot_hub` sample - Performs a FOTA update from the Azure IoT Hub.
* :ref:`asset_tracker_v2` application - Performs FOTA updates of the application, modem (delta), and boot (if enabled).
  It also supports nRF Cloud FOTA as well as AWS or Azure FOTA.
  You can configure only one at a time.
