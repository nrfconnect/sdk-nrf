.. _ug_nrf91_features:

Features of nRF91 Series
########################

.. contents::
   :local:
   :depth: 2

The nRF91 Series SiPs integrate an application MCU, a full LTE modem, an RF front end, and power management.
With built-in GNSS support, it is a great choice for asset tracking applications.

The following figure illustrates the conceptual layout when targeting an nRF91 Series Cortex-M33 application MCU with TrustZone:

.. figure:: images/nrf91_ug_overview.svg
   :alt: Overview of nRF91 application architecture

   Overview of nRF91 application architecture

Application MCU
***************

The application core is a full-featured Arm Cortex-M33 processor including DSP instructions and FPU.
Use this core for tasks that require high performance and for application-level logic.

The M33 TrustZone, one of Cortex-M Security Extensions (CMSE), divides the application MCU into Secure Processing Environment (SPE) and Non-Secure Processing Environment (NSPE).
When the MCU boots, it always starts executing from the secure area.
The secure bootloader chain starts the :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`, which configures a part of memory and peripherals to be non-secure, and then jumps to the user application located in the non-secure area.

For information about CMSE and the difference between the two environments, see :ref:`app_boards_spe_nspe`.

Secure bootloader chain
=======================

A secure bootloader chain protects your application against running unauthorized code, and it enables you to do device firmware updates (DFU).
See :ref:`ug_bootloader` for more information.

A bootloader chain is optional.
Not all of the nRF91 Series samples include a secure bootloader chain, but the ones that do use the :ref:`bootloader` sample and :doc:`MCUboot <mcuboot:index-ncs>`.

Trusted Firmware-M (TF-M)
=========================

Trusted Firmware-M provides a configurable set of software components to create a Trusted Execution Environment.
It has replaced Secure Partition Manager as the solution used by |NCS| applications and samples.
This means that when you build your application for ``_ns`` build targets, TF-M is automatically included in the build.
TF-M is a framework for functions and use cases beyond the scope of Secure Partition Manager.

For more information about the TF-M, see :ref:`ug_tfm`.
See also :ref:`tfm_hello_world` for a sample that demonstrates how to add TF-M to an application.

Application
===========

The user application runs in NSPE.
Therefore, it must be built for the ``nrf9161dk_nrf9161_ns``, ``nrf9160dk_nrf9160_ns``, or ``thingy91_nrf9160_ns`` build target.

The application image might require other images to be present.
Some samples include the :ref:`bootloader` sample (:kconfig:option:`CONFIG_SECURE_BOOT`) and :doc:`mcuboot:index-ncs` (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`).
Depending on the configuration, all these images can be built at the same time in a :ref:`multi-image build <ug_multi_image>`.

.. _lte_modem:

LTE modem
*********

The LTE modem handles LTE communication.
It is controlled through AT commands.
The AT commands are documented in the `nRF91x1 AT Commands Reference Guide`_  and `nRF9160 AT Commands Reference Guide`_.

The firmware for the modem is available as a precompiled binary.
You can download the firmware from the `nRF9161 product website (compatible downloads)`_ or `nRF9160 product website (compatible downloads)`_, depending on the SiP you are using.
The zip file contains the release notes, and both the full firmware and patches to upgrade from one version to another.
A delta patch can only upgrade the modem firmware from one specific version to another version (for example, v1.2.1 to v1.2.2).
If you need to perform a major version update (for example, v1.2.x to v1.3.x), you need an external flash with a minimum size of 4 MB.

Different versions of the LTE modem firmware are available, and these versions are certified for the mobile network operators having their own certification programs.
See the `Mobile network operator certifications`_ for more information.

.. note::

   Most operators do not require certifications other than GCF or PTCRB.
   For the current status of GCF and PTCRB certifications, see `nRF9161 certifications`_ or `nRF9160 certifications`_, depending on the SiP you are using.

.. _nrf91_update_modem_fw:
.. _nrf9160_update_modem_fw:

Modem firmware upgrade
======================

There are two ways to update the modem firmware:

Full upgrade
  You can use either a wired or a wireless connection to do a full upgrade of the modem firmware:

  * When using a wired connection, you can use either the `nRF Connect Programmer`_, which is part of `nRF Connect for Desktop`_, or the `nRF pynrfjprog`_ Python package.
    Both methods use the Simple Management Protocol (SMP) to provide an interface over UART, which enables the device to perform the update.

    * You can use the nRF Connect Programmer to perform the update, regardless of the images that are part of the existing firmware of the device.
      For example, you can update the modem on an nRF9160 DK using the instructions described in :ref:`nrf9160_updating_fw_modem` in the Developing with nRF9160 DK documentation.

    * You can also use the nRF pynrfjprog Python package to perform the update, as long as a custom application image integrating the ``lib_fmfu_mgmt`` subsystem is included in the existing firmware of the device.
      See the :ref:`fmfu_smp_svr_sample` sample for an example on how to integrate the :ref:`subsystem <lib_fmfu_mgmt>` in your custom application.

  * When using a wireless connection, the upgrade is applied over-the-air (OTA).
    See :ref:`nrf91_fota` for more information.

 See :ref:`nrfxlib:nrf_modem_bootloader` for more information on the full firmware updates of modem using :ref:`nrfxlib:nrf_modem`.

Delta patches
  Delta patches are upgrades that contain only the difference from the last version.
  See :ref:`nrfxlib:nrf_modem_delta_dfu` for more information on delta firmware updates of modem using :ref:`nrfxlib:nrf_modem`.
  When applying a delta patch, you must therefore ensure that this patch works with the current firmware version on your device.
  Delta patches are applied as firmware over-the-air (FOTA) upgrades.
  See :ref:`nrf91_fota` for more information.

.. _nrf91_ug_band_lock:
.. _nrf9160_ug_band_lock:

Band lock
=========

The modem can operate on a number of LTE bands.
To check which bands are supported by a particular modem firmware version, see the release notes for that version.

You can use band lock to restrict modem operation to a subset of the supported bands, which might improve the performance of your application.
To check which bands are certified in your region, visit `nRF9161 certifications`_ or `nRF9160 certifications`_, depending on the SiP you are using.

To set the LTE band lock, enable the :ref:`lte_lc_readme` library by setting the Kconfig option :kconfig:option:`CONFIG_LTE_LINK_CONTROL`  to ``y`` in your :file:`prj.conf` project configuration file.

Then, enable the LTE band lock feature and the band lock mask in the project configuration file, as follows::

   CONFIG_LTE_LOCK_BANDS=y
   CONFIG_LTE_LOCK_BAND_MASK="10000001000000001100"

The band lock mask allows you to set the bands on which you want the modem to operate.
Each bit in the :kconfig:option:`CONFIG_LTE_LOCK_BAND_MASK` option represents one band.
The maximum length of the string is 88 characters (bit string, 88 bits).

For more detailed information, see the `band lock section in the nRF9160 AT Commands Reference Guide`_ or the `band lock section in the nRF91x1 AT Commands Reference Guide`_, depending on the SiP you are using.

.. _nrf91_ug_network_mode:
.. _nrf9160_ug_network_mode:

System mode
===========

The system mode configuration of the modem is used to select which of the supported systems, :term:`LTE-M`, :term:`NB-IoT<Narrowband Internet of Things (NB-IoT)>` and :term:`GNSS<Global Navigation Satellite System (GNSS)>`, are enabled.

When using the :ref:`lte_lc_readme` library, all supported systems are enabled by default and the modem selects the used LTE system based on the LTE system mode preference.
You can change the enabled systems using the :kconfig:option:`CONFIG_LTE_NETWORK_MODE` Kconfig option and the LTE system mode preference using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.

When the :ref:`lte_lc_readme` library is not used, the modem starts in LTE-M mode.
You can change the system mode and the LTE system mode preference using the ``AT%XSYSTEMMODE`` AT command.

For more detailed information, see the `system mode section in the nRF9160 AT Commands Reference Guide`_ or the `system mode section in the nRF91x1 AT Commands Reference Guide`_, depending on the SiP you are using.

Modem library
*************

.. nrf91_modem_lib_start

The |NCS| applications for the nRF91 Series devices that communicate with the nRF91 Series modem firmware must include the :ref:`nrfxlib:nrf_modem`.
The :ref:`nrfxlib:nrf_modem` is released as an OS-independent binary library in the :ref:`nrfxlib` repository and it is integrated into |NCS| through an integration layer, ``nrf_modem_lib``.

The Modem library integration layer fulfills the integration requirements of the Modem library in |NCS|.
For more information on the integration, see :ref:`nrf_modem_lib_readme`.

.. nrf91_modem_lib_end

.. _modem_trace:

Modem trace
===========

The modem traces of the nRF91 Series modem can be captured using the Cellular Monitor.
For more information on how to collect traces using Cellular Monitor, see the `Cellular Monitor`_ documentation.
To enable the modem traces in the modem and to forward them to the :ref:`modem_trace_module` over UART, include the ``nrf91-modem-trace-uart`` snippet while building your application as described in :ref:`nrf91_modem_trace_uart_snippet`.

.. note::
   For the :ref:`serial_lte_modem` application and the :ref:`at_client_sample` sample, you must also run ``AT%XMODEMTRACE=1,2`` to manually activate the predefined trace set.

You can set the trace level using the AT command ``AT%XMODEMTRACE``.
For more information, see the `modem trace activation %XMODEMTRACE`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 modem trace activation %XMODEMTRACE_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

See :ref:`modem_trace_module` for other backend options.
If the existing trace backends are not sufficient, it is possible to implement custom trace backends.
For more information on the implementation of a custom trace backend, see :ref:`adding_custom_modem_trace_backends`.

.. _nrf91_fota:
.. _nrf9160_fota:

FOTA upgrades
*************

|fota_upgrades_def|
FOTA upgrades can be used to apply delta patches to the :ref:`lte_modem` firmware, full :ref:`lte_modem` firmware upgrades, and to replace the upgradable bootloader or the application.

.. note::
   Even though the Trusted Firmware-M and the application are two individually compiled components, they are treated as a single binary blob in the context of firmware upgrades.
   Any reference to the application in this section is meant to indicate the application including the Trusted Firmware-M.

To perform a FOTA upgrade, complete the following steps:

1. Make sure that your application supports FOTA upgrades.

   To download and apply FOTA upgrades, your application must use the :ref:`lib_fota_download` library.
   This library determines the type of upgrade by inspecting the header of the firmware and invokes the :ref:`lib_dfu_target` library to apply the firmware upgrade.
   In its default configuration, the DFU target library is set to support all the types of FOTA upgrades except full modem firmware upgrades, but you can freely enable or disable the support for specific targets.
   In addition, the following requirements apply:

   * To upgrade the application, you must use :doc:`mcuboot:index-ncs` as the upgradable bootloader (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` must be enabled).
   * If you want to upgrade the upgradable bootloader, you must use the :ref:`bootloader` (:kconfig:option:`CONFIG_SECURE_BOOT must be enabled`).
   * If you want to upgrade the modem firmware through modem delta updates, you do not need to use MCUboot or the immutable bootloader, because the modem firmware upgrade is handled by the modem itself.
   * If you want to perform a full modem firmware upgrade, an |external_flash_size| is required.

#. Create a binary file that contains the new image.

   .. note::
      This step does not apply for upgrades of the modem firmware.
      You can download delta patches and full binaries of the modem firmware from the `nRF9161 product website (compatible downloads)`_ or `nRF9160 product website (compatible downloads)`_, depending on the SiP you are using.

   |fota_upgrades_building|
   The :file:`app_update.bin` file is the file that should be uploaded to the server.

   To create binary files for a bootloader upgrade, make sure that :kconfig:option:`CONFIG_SECURE_BOOT` and :kconfig:option:`CONFIG_BUILD_S1_VARIANT` are enabled and build MCUboot as usual.
   The build will create a binary file for each variant of the upgradable bootloader, one for each bootloader slot.
   See :ref:`upgradable_bootloader` for more information.

#. Make the binary file (or files) available for download.
   Upload the serialized :file:`.cbor` binary file or files to a web server that is compatible with the :ref:`lib_download_client` library.

The full FOTA procedure depends on where the binary files are hosted for download.

FOTA upgrades using nRF Cloud
=============================

FOTA upgrades can be managed through a comprehensive management portal on `nRF Cloud`_, either fully hosted on nRF Cloud or accessible from a customer cloud using the `nRF Cloud REST API`_.
If you are using nRF Cloud, see the `nRF Cloud Getting Started FOTA documentation`_ for instructions.

Currently, delta modem firmware FOTA files are available in nRF Cloud under :guilabel:`Firmware Updates` in the :guilabel:`Device Management` tab on the left.
If you intend to obtain FOTA files from nRF Cloud, see the additional requirements in :ref:`lib_nrf_cloud_fota`.

You can upload custom application binaries to nRF Cloud for application FOTA updates.
After :ref:`nrf9160_gs_connecting_dk_to_cloud`, you can upload the files to your nRF Cloud account as a bundle after navigating to :guilabel:`Device Management` on the left and clicking :guilabel:`Firmware Updates`.

FOTA upgrades using other cloud services
========================================

FOTA upgrades can alternatively be hosted from a customer-developed cloud services such as solutions based on AWS and Azure.
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
  It also supports nRF Cloud FOTA as well as AWS or Azure FOTA. Only one must be configured at a time.

.. _nrf91_ug_gnss:
.. _nrf9160_ug_gnss:

GNSS
****

An nRF91 Series device is a highly versatile device that integrates both cellular and GNSS functionality.
Note that GNSS functionality is only available on the SICA variant and not on the SIAA or SIBA variants.
For an nRF9160 SiP, see `nRF9160 SiP revisions and variants`_ for more information.

There are many GNSS constellations (GPS, BeiDou, Galileo, GLONASS) available but GPS is the most mature technology.
An nRF91 Series device supports both GPS L1 C/A (Coarse/Acquisition) and QZSS L1C/A at 1575.42 MHz.
This frequency band is ideal for penetrating through layers of the atmosphere (troposphere and ionosphere) and suitable for various weather conditions.
GNSS is designed to be used with a line of sight to the sky.
Therefore, the performance is not ideal when there are obstructions overhead or if the receiver is indoors.

Customers who are developing their own hardware with the nRF9160 are strongly recommended to use the `nRF9160 Antenna and RF Interface Guidelines`_ as a reference.
See `GPS interface and antenna`_ for more details on GNSS interface and antenna.

.. note::

   Starting from |NCS| v1.6.0 (Modem library v1.2.0), the GNSS socket is deprecated and replaced with the :ref:`GNSS interface <gnss_interface>`.

Obtaining a fix
===============

GNSS provides lots of useful information including 3D location (latitude, longitude, altitude), time, and velocity.

The time to obtain a fix (also referred to as Time to First Fix (TTFF)) will depend on the time when the GNSS receiver was last turned on and used.

Following are the various GNSS start modes:

* Cold start - GNSS starts after being powered off for a long time with zero knowledge of the time, current location, or the satellite orbits.
* Warm start - GNSS has some coarse knowledge of the time, location, or satellite orbits from a previous fix that is more than around 37 minutes old.
* Hot start - GNSS fix is requested within an interval of around 37 minutes from the last successful fix.

Each satellite transmits its own `ephemeris`_ data and common `almanac`_ data:

* Ephemeris data - Provides information about the orbit of the satellite transmitting it. This data is valid for four hours and becomes inaccurate after that.
* Almanac data - Provides coarse orbit and status information for each satellite in the constellation. Each satellite broadcasts almanac data for all satellites.

The data transmission occurs at a slow data rate of 50 bps.
The orbital data can be received faster using A-GNSS.

Due to the clock bias on the receiver, there are four unknowns when looking for a GNSS fix - latitude, longitude, altitude, and clock bias.
This results in solving an equation system with four unknowns, and therefore a minimum of four satellites must be tracked to acquire a fix.

.. _nrf91_gps_lte:
.. _nrf9160_gps_lte:

Concurrent GNSS and LTE
=======================

The GNSS operation in an nRF91 Series device is time-multiplexed with the LTE modem.
Therefore, the LTE modem must either be completely deactivated or in `RRC idle mode <Radio Resource Control_>`_ or `Power Saving Mode (PSM)`_ when using the GNSS receiver.
For more information, see the `nRF9161 GPS receiver Specification`_ or the `nRF9160 GPS receiver Specification`_, depending on the SiP you are using.

Enhancements to GNSS
====================

When GNSS has not been in use for a while or if the device is in relatively weak signaling conditions, it might take longer to acquire a fix.
To improve this, Nordic Semiconductor has implemented the following methods for acquiring a fix in a shorter time:

* A-GNSS or P-GPS or a combination of both
* Low accuracy mode

Assisted GNSS (A-GNSS)
----------------------

A-GNSS is commonly used to improve the Time to first fix (TTFF) by using a connection (for example, over cellular) to the Internet to retrieve the almanac and ephemeris data.
A connection to an Internet server that has the almanac and ephemeris data is several times quicker than using the slow 50 bps data link to the GNSS satellites.
There are many options to retrieve this A-GNSS data.
Two such options are using `nRF Cloud`_ and SUPL.
|NCS| provides example implementations for both these options.
The A-GNSS solution available through nRF Cloud has been optimized for embedded devices to reduce protocol overhead and data usage.
This, in turn, results in the download of reduced amount of data, thereby reducing data transfer time, power consumption, and data costs.
Starting from modem firmware v2.0.0, GNSS supports assistance data also for QZSS satellites.
nRF Cloud can provide assistance data for both GPS and QZSS.
See :ref:`nrfxlib:gnss_int_agps_data` for more information about the retrieval of A-GNSS data.

Predicted GPS (P-GPS)
---------------------

P-GPS is a form of assistance, where the device can download up to two weeks of predicted satellite ephemerides data.
Normally, devices connect to the cellular network approximately every two hours for up-to-date satellite ephemeris information or they download the ephemeris data from the acquired satellites.
P-GPS enables devices to determine the exact orbital location of the satellite without connecting to the network every two hours with a trade-off of reduced accuracy of the calculated position over time.
Note that P-GPS requires more memory compared to regular A-GNSS.

Also, note that due to satellite clock inaccuracies, not all functional satellites will have ephemerides data valid for two weeks in the downloaded P-GPS package.
This means that the number of satellites having valid predicted ephemerides reduces in number roughly after ten days.
Hence, the GNSS module needs to download the ephemeris data from the satellite broadcast if no predicted ephemeris is found for that satellite to be able to use the satellite.

.. note::
   |gnss_tradeoffs|

nRF Cloud compared with SUPL library
------------------------------------

* The :ref:`lib_nrf_cloud_agnss` library is more efficient to use when compared to the :ref:`SUPL <supl_client>` library, and the latter takes a bit more memory on the device.
* With nRF Cloud, the data is encrypted, whereas SUPL uses plain socket.
* nRF Cloud also supports assistance for QZSS satellites, while SUPL is limited to GPS.
* No licenses are required from external vendors to use nRF Cloud, whereas for commercial use of SUPL, you must obtain a license.
* The :ref:`lib_nrf_cloud_agnss` library is highly integrated into `Nordic Semiconductor's IoT cloud platform`_.

Low Accuracy Mode
-----------------

Low accuracy mode allows the GNSS receiver to accept a looser criterion for a fix with four or more satellites or by using a reference altitude to allow for a fix using only three satellites.
This has a tradeoff of reduced accuracy.
This reference altitude can be from a recent valid normal fix or it can be artificially injected.
See :ref:`nrfxlib:gnss_int_low_accuracy_mode` for more information about low accuracy mode and its usage.

Samples using GNSS in |NCS|
===========================

There are many examples in |NCS| that use GNSS.
Following is a list of the samples and applications with some information about the GNSS usage:

* The :ref:`asset_tracker_v2` application uses nRF Cloud for A-GNSS, P-GPS, or a combination of both.
  The application obtains GNSS fixes and transmits them to nRF Cloud along with sensor data.
* The :ref:`serial_lte_modem` application uses AT commands to start and stop GNSS and supports nRF Cloud A-GNSS and P-GPS.
  The application displays tracking and GNSS fix information in the serial console.
* The :ref:`gnss_sample` sample does not use assistance by default but can be configured to use nRF Cloud A-GNSS, P-GPS, or a combination of both.
  The sample displays tracking and fix information as well as NMEA strings in the serial console.
