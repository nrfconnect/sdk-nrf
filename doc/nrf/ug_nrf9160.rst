.. _ug_nrf9160:

Working with nRF9160 DK
#######################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for developing on the nRF9160 System in Package (SiP) using the nRF9160 DK (PCA10090), and offers :ref:`samples <nrf9160_samples>` dedicated to this device.

See the `nRF9160 DK Hardware`_ guide for detailed information about the nRF9160 DK hardware.
To get started with the nRF9160 DK, follow the steps in the `nRF9160 DK Getting Started`_ guide.

.. _nrf9160_ug_intro:

Introduction
************

The nRF9160 SiP integrates an application MCU, a full LTE modem, an RF front end, and power management.
With built-in GPS support, it is dedicated to asset tracking applications.

For more details on the SiP, see the `nRF9160 product website`_ and the `nRF9160 Product Specification`_.

.. figure:: images/nrf9160_ug_overview.svg
   :alt: Overview of nRF91 application architecture

   Overview of nRF91 application architecture

The figure illustrates the conceptual layout when targeting an nRF9160 Cortex-M33 application MCU with TrustZone.

Application MCU
===============

The M33 TrustZone divides the application MCU into secure and non-secure domains.
When the MCU boots, it always starts executing from the secure area.
The secure bootloader chain starts the :ref:`nrf9160_ug_secure_partition_manager` or the :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`, which configures a part of memory and peripherals to be non-secure and then jumps to the main application located in the non-secure area.

In Zephyr, :ref:`zephyr:nrf9160dk_nrf9160` is divided into two different build targets:

* ``nrf9160dk_nrf9160`` for firmware in the secure domain
* ``nrf9160dk_nrf9160ns`` for firmware in the non-secure domain

Make sure to select a suitable build target when building your application.

Secure bootloader chain
-----------------------

A secure bootloader chain protects your application against running unauthorized code, and it enables you to do device firmware updates (DFU).
See :ref:`ug_bootloader` for more information.

A bootloader chain is optional.
Not all of the nRF9160 samples include a secure bootloader chain, but the ones that do use the :ref:`bootloader` sample and :doc:`mcuboot:index`.

.. _nrf9160_ug_secure_partition_manager:

Secure Partition Manager
------------------------

All nRF9160 samples require the :ref:`secure_partition_manager` sample.
It provides a reference implementation of a Secure Partition Manager firmware.
This firmware is required to set up the nRF9160 DK so that it can run user applications in the non-secure domain.

The Secure Partition Manager sample is automatically included in the build for the ``nrf9160dk_nrf9160ns`` build target.
To disable the automatic inclusion of the Secure Partition Manager sample, set the option :kconfig:`CONFIG_SPM` to "n" in the project configuration.

Trusted Firmware-M (TF-M) support
---------------------------------

You can use Trusted Firmware-M (TF-M) as an alternative to :ref:`secure_partition_manager` for running an application from the non-secure area of the memory.

Support for TF-M in |NCS| is currently experimental.
TF-M is a framework which will be extended for new functions and use cases beyond the scope of SPM.

If your application does not depend on the secure services developed in SPM and does not use them, TF-M can replace SPM as the secure firmware component in your application.

For more information and instructions on how to do this, see :ref:`ug_tfm`.

Application
-----------

The user application runs in the non-secure domain.
Therefore, it must be built for the ``nrf9160dk_nrf9160ns`` build target.

The application image might require other images to be present.
Depending on the configuration, all these images can be built at the same time in a :ref:`multi-image build <ug_multi_image>`.

All nRF9160 samples include the :ref:`secure_partition_manager` sample, which can be enabled or disabled with the :kconfig:`CONFIG_SPM` option.
Some also include the :ref:`bootloader` sample (:kconfig:`CONFIG_SECURE_BOOT`) and :doc:`mcuboot:index` (:kconfig:`CONFIG_BOOTLOADER_MCUBOOT`).


LTE modem
=========

The LTE modem handles LTE communication.
It is controlled through `AT commands <AT Commands Reference Guide_>`_.

The firmware for the modem is available as a precompiled binary.
You can download the firmware from the `nRF9160 product website (compatible downloads)`_.
The zip file contains both the full firmware and patches to upgrade from one version to another.

Different versions of the LTE modem firmware are available, and these versions are certified for the mobile network operators who have their own certification programs.
See the `Mobile network operator certifications`_ for more information.

.. note::

   Most operators do not require other certifications than GCF or PTCRB.
   For the current status of GCF and PTCRB certifications, see `nRF9160 certifications`_.

There are two ways to update the modem firmware:

Full upgrade
  You can use either a wired or a wireless connection to do a full upgrade of the modem firmware:

  * When using a wired connection, you can use either the `nRF Connect Programmer`_, which is part of `nRF Connect for Desktop`_, or the `nRF pynrfjprog`_ Python package.
    Both methods use the Simple Management Protocol (SMP) to provide an interface over UART, which enables the device to perform the update.

    * You can use the nRF Connect Programmer to perform the update, regardless of the images that are part of the existing firmware of the device.
      See `Updating the nRF9160 DK cellular modem`_ in the nRF Connect Programmer user guide for more details.

    * You can also use the nRF pynrfjprog Python package to perform the update, as long as a custom application image integrating the ``lib_fmfu_mgmt`` subsystem is included in the existing firmware of the device.
      See the :ref:`fmfu_smp_svr_sample` sample for an example on how to integrate the :ref:`subsystem <lib_fmfu_mgmt>` in your custom application.

  * When using a wireless connection, the upgrade is applied over-the-air (OTA).
    See :ref:`nrf9160_ug_fota` for more information.

 See :ref:`nrfxlib:full_dfu`, for more information on the full firmware update of modem using :ref:`nrfxlib:nrf_modem`.

Delta patches
  Delta patches are upgrades that contain only the difference from the last version.
  A delta patch can only upgrade the modem firmware from one specific version to another version.
  See :ref:`nrfxlib:nrf_modem_delta_dfu` for more information on delta firmware update of modem using :ref:`nrfxlib:nrf_modem`.
  When applying a delta patch, you must therefore ensure that this patch works with the current firmware version on your device.
  Delta patches are applied as firmware over-the-air (FOTA) upgrades.
  See :ref:`nrf9160_ug_fota` for more information.

Modem library
=============

The |NCS| applications for the nRF9160 DK that communicate with the nRF9160 modem firmware must include the Modem library.
The :ref:`nrfxlib:nrf_modem` is released as an OS-independent binary library in the :ref:`nrfxlib` repository and it is integrated into |NCS| via an integration layer, ``nrf_modem_lib``.

The Modem library integration layer fulfills the integration requirements of the Modem library in |NCS|.
For more information on the integration, see :ref:`nrf_modem_lib_readme`.


.. _nrf9160_ug_band_lock:

Band lock
*********

The band lock is a functionality of the application that lets you send an AT command to the modem instructing it to operate only on specific bands.
The band lock is handled by the LTE Link Control driver.
By default, the functionality is disabled in the driver's Kconfig file.

The modem can operate in the following E-UTRA Bands: 1, 2, 3, 4, 5, 8, 12, 13, 17, 18, 19, 20, 25, 26, 28, and 66.

You can use the band lock to restrict modem operation to a subset of the supported bands, which might improve the performance of your application.
To check which bands are certified in your region, visit `nRF9160 Certifications`_.

To set the LTE band lock, enable the *LTE Link Control Library* in your project configuration file ``prj.conf``, using::

   CONFIG_LTE_LINK_CONTROL=y

Then, enable the LTE band lock feature and the band lock mask in the configuration file of your project, as follows::

   CONFIG_LTE_LOCK_BANDS=y
   CONFIG_LTE_LOCK_BAND_MASK="10000001000000001100"

The band lock mask allows you to set the bands on which you want the modem to operate.
Each bit in the :kconfig:`CONFIG_LTE_LOCK_BAND_MASK` option represents one band.
The maximum length of the string is 88 characters (bit string, 88 bits).

The band lock is a non-volatile setting that must be set before activating the modem.
It disappears when the modem is reset.
To prevent this, you can set the modem in *power off* mode, by either:

* Sending the AT command ``AT+CFUN=0`` directly.
* Calling the :c:func:`lte_lc_power_off` function while the *LTE Link Control Library* is enabled.

Both these options save the configurations and historical data in the Non-Volatile Storage before powering off the modem.

As a recommendation, turn off the band lock after the connection is established and let the modem use the historical connection data to optimize the network search, in case the device is disconnected or moved.

For more detailed information, see the `band lock section in the AT Commands reference document`_.

.. _nrf9160_ug_network_mode:

Network mode
************

The modem supports LTE-M (Cat-M1) and Narrowband Internet of Things (NB-IoT or LTE Cat-NB).
By default, the modem starts in LTE-M mode.

When using the LTE Link Control driver, you can select LTE-M with :kconfig:`CONFIG_LTE_NETWORK_MODE_LTE_M` or NB-IoT with :kconfig:`CONFIG_LTE_NETWORK_MODE_NBIOT`.

To start in NB-IoT mode without the driver, send the following command before starting the modem protocols (by using ``AT+CFUN=1``)::

   AT%XSYSTEMMODE=0,1,0,0

To change the mode at runtime, set the modem to LTE RF OFF state before reconfiguring the mode, then set it back to normal operating mode::

   AT+CFUN=4
   AT%XSYSTEMMODE=0,1,0,0
   AT+CFUN=1

If the modem is shut down gracefully before the next boot (by using ``AT+CFUN=0``), it keeps the current setting.

For more detailed information, see the `system mode section in the AT Commands reference document`_.

.. |An nRF9160-based device| replace:: An nRF9160 DK
.. |an nRF9160-based device| replace:: an nRF9160 DK

.. _nrf9160_gps_lte:

.. nrf9160_gps_lte_start

Concurrent GPS and LTE
======================

|An nRF9160-based device| supports GPS in LTE-M and NB-IoT.
Concurrent operation of GPS with optional power-saving features, such as extended Discontinuous Reception (eDRX) and Power Saving Mode (PSM), is also supported, and recommended.

The following figure shows how the data transfer occurs in |an nRF9160-based device| with power-saving in place.

.. figure:: /images/power_consumption.png
   :alt: Power consumption

See `Energy efficiency`_ for more information.

Asset Tracker enables the concurrent working of GPS and LTE in eDRX and PSM modes when the device is in `RRC idle mode`_.
The time between the transition of a device from RRC connected mode (data transfer mode) to RRC idle mode is dependent on the network.
Typically, the time ranges between 5 seconds to 70 seconds after the last data transfer on LTE.
Sensor and GPS data is sent to the cloud only during the data transfer phase.

.. nrf9160_gps_lte_end

.. _nrf9160_ug_fota:

FOTA upgrades
*************

|fota_upgrades_def|
FOTA upgrades can be used to apply delta patches to the `LTE modem`_ firmware, full `LTE modem`_ firmware upgrades, and to replace the upgradable bootloader or the application.

.. note::
   Even though the Secure Partition Manager and the application are two individually compiled components, they are treated as a single binary blob in the context of firmware upgrades.
   Any reference to the application in this section is meant to indicate the application including the Secure Partition Manager.

To perform a FOTA upgrade, complete the following steps:

1. Make sure that your application supports FOTA upgrades.
      To download and apply FOTA upgrades, your application must use the :ref:`lib_fota_download` library.
      This library deduces the type of upgrade by inspecting the header of the firmware and invokes the :ref:`lib_dfu_target` library to apply the firmware upgrade.
      In its default configuration, the DFU target library is set to support all the types of FOTA upgrades except full modem firmware upgrades, but you can freely enable or disable the support for specific targets.

      In addition, the following requirements apply:

      * |fota_upgrades_req_mcuboot|
      * If you want to upgrade the upgradable bootloader, the :ref:`bootloader` must be used (:kconfig:`CONFIG_SECURE_BOOT`).
      * If you want to upgrade the modem firmware through modem delta updates, neither MCUboot nor the immutable bootloader are required, because the modem firmware upgrade is handled by the modem itself.
      * If you want to perform a full modem firmware upgrade, an |external_flash_size| is required.

#. Create a binary file that contains the new image.

      .. note::
         This step does not apply for upgrades of the modem firmware.
         You can download delta patches and full binaries of the modem firmware from the `nRF9160 product website (compatible downloads)`_.

      |fota_upgrades_building|
      The :file:`app_update.bin` file is the file that should be uploaded to the server.

      To create binary files for a bootloader upgrade, make sure that :kconfig:`CONFIG_SECURE_BOOT` and :kconfig:`CONFIG_BUILD_S1_VARIANT` are enabled and build MCUboot as usual.
      The build will create a binary file for each variant of the upgradable bootloader, one for each bootloader slot.
      See :ref:`upgradable_bootloader` for more information.

#. Make the binary file (or files) available for download.
     Upload the serialized :file:`.cbor` binary file or files to a web server that is compatible with the :ref:`lib_download_client` library.
     One way of doing this is to upload the files to an Amazon Web Services Simple Storage Service (AWS S3) bucket.
     See the :ref:`lib_aws_fota` documentation for instructions.

     Your application must be able to retrieve the host and file name for the binary file.
     See :ref:`lib_fota_download` for information about the format of this information, especially when providing two files for a bootloader upgrade.
     You can hardcode the information in the application, or you can use functionality like AWS jobs to provide the URL dynamically.

The full FOTA procedure depends on where the binary files are hosted for download.

You can refer to the following implementation samples:

* :ref:`http_full_modem_update_sample` - performs a full firmware OTA update of the modem.
* :ref:`http_modem_delta_update_sample` - performs a delta OTA update of the modem firmware.
* :ref:`http_application_update_sample` - performs a basic application FOTA update.
* :ref:`aws_fota_sample` - performs a FOTA update via MQTT and HTTP, where the firmware download is triggered through an AWS IoT job.

Board controller
****************

The nRF9160 DK contains an nRF52840 SoC that is used to route some of the nRF9160 SiP pins to different components on the DK, such as LEDs and buttons, and to specific pins of the nRF52840 SoC itself.
For a complete list of all the routing options available, see the `nRF9160 DK board control section in the nRF9160 DK User Guide`_.

The nRF52840 SoC on the DK comes preprogrammed with a firmware.
If you need to restore the original firmware at some point, download the nRF9160 DK board controller FW from the `nRF9160 DK product page`_.
To program the HEX file, use nrfjprog (which is part of the `nRF Command Line Tools`_).

If you want to route some pins differently from what is done in the preprogrammed firmware, program the :ref:`zephyr:hello_world` sample instead of the preprogrammed firmware.
Build the sample (located under ``ncs/zephyr/samples/hello_world``) for the nrf9160dk_nrf52840 board.
To change the routing options, enable or disable the corresponding devicetree nodes for that board as needed.
See :ref:`zephyr:nrf9160dk_board_controller_firmware` for detailed information.

Board revisions
***************

nRF9160 DK v0.14.0 and later has additional hardware features that are not available on earlier versions of the DK:

* External flash memory
* I/O expander

To make use of these features, specify the board revision when building your application.

.. note::
   You must specify the board revision only if you use features that are not available in all board revisions.
   If you do not specify a board revision, the firmware is built for the default revision (v0.7.0).
   Newer revisions are compatible with the default revision.

To specify the board revision, append it to the build target when building.
For example, when building a non-secure application for nRF9160 DK v1.0.0, use ``nrf9160dk_nrf9106ns@1.0.0`` as build target.

When building with |SES|, specify the board revision as additional CMake option (see :ref:`cmake_options` for instructions).
For example, for nRF9160 DK v1.0.0, add the following CMake option::

  -DBOARD=nrf9160dk_nrf9160ns@1.0.0

See :ref:`zephyr:application_board_version` and :ref:`zephyr:nrf9160dk_additional_hardware` for more information.


.. _nrf9160_ug_drivs_libs_samples:

Available drivers, libraries, and samples
*****************************************

See the :ref:`drivers`, :ref:`libraries`, and :ref:`nRF9160 samples <nrf9160_samples>` sections and the respective repository folders for up-to-date information.
