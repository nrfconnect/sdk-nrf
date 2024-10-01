.. _ncs_release_notes_140:

|NCS| v1.4.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices (which are pre-production) and Zigbee and Bluetooth Mesh protocols are supported for development in v1.4.0 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.


Highlights
**********

* Added production support for nRF52833 and nRF52840 for Thread, including multiprotocol operation with Bluetooth Low Energy
* Added Network Co-Processor (NCP) architecture for Thread and Zigbee
* Added development support for multiprotocol operation of Zigbee and Bluetooth Low Energy
* Added support for nRF5340 Network Core DFU
* Added the standalone Remote procedure call (nRF RPC) library with a simple sample for nRF5340
* Azure IoT Hub and FOTA support for nRF9160
* Low power UDP sample for nRF9160
* Updates to nRF9160 Serial LTE modem application
* New Bluetooth Mesh samples and models for light control
* nRF Bluetooth LE Controller has been renamed to SoftDevice Controller
* The standalone SoftDevice Controller library is now the default Bluetooth LE Controller for Bluetooth samples (except the Bluetooth Mesh samples)

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.4.0**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

This version of the |NCS| has been tested with the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.2.2

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added:

  * :ref:`lib_azure_fota` library - This library provides a way to parse Azure IoT Hub device twin messages to obtain firmware upgrade information and perform FOTA downloads.
  * :ref:`lib_azure_iot_hub` library - This library provides an API to connect to an Azure IoT Hub instance and interact with it.
  * :ref:`azure_iot_hub` sample - This sample demonstrates the communication of an nRF9160-based device with an Azure IoT Hub instance.
  * nRF9160: Azure FOTA sample - This sample demonstrates how to perform an over-the-air firmware update of an nRF9160-based device using the Azure FOTA and Azure IoT Hub libraries.
  * :ref:`aws_iot` sample - This sample demonstrates the communication of an nRF9160-based device with the AWS IoT message broker over MQTT.
  * :ref:`udp` low power sample - This sample demonstrates the sequential transmission of UDP packets to a predetermined server identified by an IP address and a port.
  * :ref:`download_sample` sample - This sample demonstrates how to download files over the Internet from HTTP(S) and CoAP(S) servers.

* Updated:

  * ``at_cmd`` library library:

    * Reimplemented the library to enable asynchronous handling of commands and reduce memory usage.
    * Updated all commands to only match ``OK`` or ``ERROR`` in the response if they are at the end (in case those strings are part of the response, like in certificate).

  * BSD library:

    * Updated to version 0.8.1.
      See the :ref:`nrfxlib:nrf_modem_changelog` for detailed information.

  * :ref:`coap_utils_readme` library:

    * Added an optional ``addr`` parameter to the :c:func:`coap_init()` function.
      The parameter is meant for socket binding.

  * :ref:`connectivity_bridge` application:

    * Added automatic re-enabling of UART RX upon errors.
    * Improved the handling of configuration file.
    * Added an option to configure Bluetooth device name.

  * :ref:`lib_download_client` library:

    * Added CoAP block-wise transfer support, which can be enabled with :kconfig:option:`CONFIG_COAP`.
    * Updated functions that end with ``_connect()`` and ``_start()`` to parse complete URLs, with port and schema.
    * Removed ``port`` field in :c:struct:`download_client_cfg`.
      The port number can now be passed together with the URL.
    * Removed the ``CONFIG_DOWNLOAD_CLIENT_TLS`` Kconfig option.
      Now the choice between secure and non-secure HTTP or CoAP is determined by the schema, or by the security tag if the schema is missing.
    * Stopped using HTTP range requests when using HTTP, which improves bandwidth.
    * Updated the parsing of HTTP header fields to be case-insensitive.
    * Added support for Zephyr's :ref:`zephyr:shell_api`.
    * Added ``fragment_size`` parameter to :c:func:`fota_download_start` to allow to specify download fragment size at run time.

  * :ref:`lib_fota_download` library:

    * Added a missing call to :c:func:`dfu_target_done` if :c:func:`dfu_target_write` fails.
    * Added the error cause information to the :c:enumerator:`FOTA_DOWNLOAD_EVT_ERROR` event.

  * :ref:`lte_lc_readme` library:

    * Updated to parse PSM configuration only when the device is registered to a network.
      This will help avoid confusing error messages.
    * Added API for setting eDRX Paging Time Window (PTW).
    * Added support for Release Assistance Indication (RAI).
    * Added :c:func:`lte_lc_deinit()` function to the API.
      This function deinitializes the LTE LC module.
    * Reworked system mode handling as follows:

      * The preferred mode and optionally the fallback mode are now set through Kconfig.
      * The current mode is the mode read from the device and is changed using :c:func:`lte_lc_system_mode_set()`.
      * The target mode is the mode that is used when connecting to LTE network, that is when :c:func:`lte_lc_connect()` or  :c:func:`lte_lc_connect_async()` is called.
        The network is initialized with the configurable (and preferred) system mode.
        The mode is changed when :c:func:`lte_lc_system_mode_set()` is called or when connection establishment using preferred mode is unsuccessful and times out.

  * :ref:`liblwm2m_carrier_readme` library:

    * Added the snapshot of the release version 0.10.0.
      See the :ref:`liblwm2m_carrier_changelog` for detailed information.

* :ref:`supl_client` library and :ref:`agps_sample` sample:

    * Renamed the sample from nRF Connect for Cloud A-GPS.
    * Added a common A-GPS interface for SUPL and nRF Connect for Cloud A-GPS service.
    * Added sending of service information after a successful connection to `nRF Connect for Cloud`_ has been made.

* nRF9160: Asset Tracker application:

    * Added handling of sensor channel ``get`` commands received from `nRF Connect for Cloud`_.
    * Added event handler for :ref:`lte_lc_readme` events.
    * Added the detection feature when there is no SIM card in the slot.
    * Added support for Bosch BSEC library 1.4.8.0 (see Zephyr BME680 sample).
      This breaks compatibility with older versions of the library.
    * Added a timestamp for sensor or cloud data, or both.
    * Added the ``CONFIG_UI_LED_PWM_FREQUENCY`` Kconfig option for setting the LED PWM frequency.

* :ref:`gps_with_supl_support_sample` sample:

    * Updated the sample to allow disabling the **COEX0** pin when using the external antenna to lower noise from the LNA.
    * Updated the frequency range of the external GPS amplifier.
    * Added an option to give GPS prioritized radio access.
    * Added functionality that increases the GPS priority when GPS is blocked for more than a configurable amount of time.

* :ref:`lwm2m_client` sample:

    * Fixed an invalid Kconfig option (``CONFIG_FOTA_ERASE_PROGRESSIVELY``) that prevented progressive erase during FOTA.
    * Added :file:`overlay-nbiot.conf` with fine-tuned CoAP/LwM2M parameters for NB-IoT networks.
    * Fixed a bug where a FOTA socket was not closed after the download (PULL mode).
    * Added bootstrap procedure support to the sample.
    * Enabled the usage of the :ref:`lib_dfu_target` library for firmware updates, which allows to update both the application and the modem firmware.

* :ref:`serial_lte_modem` application:

    * Added support for the MQTT username and password.
    * Added reading of status of TCP proxy server/client when it is not started or connected yet.
    * Added support for partial reception of RX data (in TCP/IP proxy).
    * Added AT command ``#XSLMUART`` to change the UART baud rate.
    * Added data mode support for TCP/UDP proxy client/server.
    * Added support for the HTTP client service.
    * Added FOTA support.
    * Added TLS server support.

* nRF9160: Simple MQTT sample:

    * Added TLS support to the sample.

* :ref:`lib_nrf_cloud` library:

    * Added saving of a valid session flag to settings after all subscriptions have completed, so that the persistent session is only used when the flag is valid.
    * Replaced ``CONFIG_CLOUD_PERSISTENT_SESSIONS`` usage with Zephyr's :kconfig:option:`CONFIG_MQTT_CLEAN_SESSION`.
    * Made the MQTT client ID prefix configurable.
    * Added an option to set send time-out for the socket used by nRF Cloud library (:kconfig:option:`CONFIG_NRF_CLOUD_SEND_TIMEOUT`).

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

nRF5340 SoC
-----------

* Added:

  * :ref:`nc_bootloader` sample - This sample implements an immutable first stage bootloader that has the capability to update the application firmware on the network core of the nRF5340 System on Chip (SoC).
  * :ref:`nrf_rpc_entropy_nrf53` sample - This sample demonstrates how to use the entropy driver in a dual core device such as nRF5340 PDK.
  * :ref:`nrfxlib:nrf_rpc` - This standalone library enables inter-processor communication on Nordic Semiconductor devices.

* Zephyr's :zephyr:code-sample:`smp-svr` now works on nRF5340 PDK.

Bluetooth Low Energy
--------------------

* Added:

  * :ref:`bms_readme` - This module implements the Bond Management Service with the corresponding set of characteristics.
  * :ref:`peripheral_bms` sample - This sample demonstrates how to use the GATT Bond Management Service (BMS).
  * :ref:`direct_test_mode` sample - This sample demonstrates the Direct Test Mode functions described in Bluetooth Core Specification, Version 5.2, Vol. 6, Part F.
  * Alexa Gadgets Service
  * Bluetooth: Peripheral Alexa Gadgets sample - This sample demonstrates how a Bluetooth LE device can connect to an Amazon Echo device using the Alexa Gadgets Bluetooth Service and Profile.
  * :ref:`bluetooth_central_hr_coded` sample (external contribution) - This sample demonstrates how to create a connection as a central using LE Coded PHY.
  * :ref:`peripheral_hr_coded` sample (external contribution) - This sample demonstrates how to use the extended advertising API to create a connectable advertiser on LE Coded PHY.

* Updated:

  * Changed default security settings in :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples.
    These samples now require encryption for accessing characteristics.
  * Added connection attempts filter to :ref:`nrf_bt_scan_readme` library.
    The filter can be used for blocking peers that disconnected too many times.
  * Changed the naming conventions of Bluetooth services API by removing the ``_gatt_`` and ``_c_`` infixes and using profile names where applicable.

SoftDevice Controller (renamed from nRF Bluetooth LE Controller)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See the :ref:`nrfxlib:softdevice_controller_changelog` for detailed information.

* Renamed nRF Bluetooth LE Controller to SoftDevice Controller.
  API was updated accordingly.
* The standalone SoftDevice Controller library is now the default Bluetooth LE Controller for :ref:`ble_samples` except the Bluetooth Mesh samples.
* Implemented the remaining mandatory HCI commands to make the controller conformant to HCI standards.
* Reduced the image size when linking the final binary.
  Now, only the requested features are included.
  See :c:func:`sdc_support_adv()` and similar APIs for details.

nRF Desktop
-----------

* Added support for :ref:`MCUboot <mcuboot_wrapper>` that allows to use the following features:

  * :ref:`Serial recovery DFU though USB <nrf_desktop_bootloader_serial_dfu>`
  * :ref:`Background DFU <nrf_desktop_bootloader_background_dfu>` through :ref:`nrf_desktop_smp`
  * Background DFU with secondary slot on the :ref:`external flash <nrf_desktop_memory_layout>`

* Dongle updates:

  * Added support for connecting to multiple devices of the same type.
  * Added a configuration where the nRF52840 Dongle stores 6 bonds and allows up to 4 simultaneous connections.
  * Added support for one HID-class USB device instance per bonded peripheral device.
    The host can distinguish a source of a HID report.

* Reworked the :ref:`nrf_desktop_config_channel` module and updated the protocol:

  * Added device identification with HW ID.
  * Improved flow control for faster data transmission.

* Added USB wake-up support in the :ref:`nrf_desktop_usb_state`.
* Added :ref:`nrf_desktop_qos` that works on a peripheral device.
  This module provides information about Bluetooth LE channel quality on peripherals.
* Bugfixes:

  * [DESK-1087] Fixed invalid peer counting in :ref:`nrf_desktop_led_state`.
  * [DESK-1084] Fixed reset handling in :ref:`nrf_desktop_usb_state` when in standby.
  * [DESK-1072] Fixed a bug where reports were not working on USB mouse after host reboot.
  * [DESK-1067] Fixed a bug where triggering erase advertising when mouse was sleeping would cause a module error.
  * [DESK-1014] Fixed a bug where a device would not go to idle if there was nothing to schedule.
  * [DESK-1011] Fixed an unhandled USB event.
  * [DESK-1008] Fixed a non-compliant report descriptor on dongle.
  * [DESK-974] Fixed a bug where boot reports were not sent.
  * [DESK-973] Fixed a bug where Dongle - Keyboard connection would deteriorate badly with :ref:`split Link Layer <nrf_desktop_bluetooth_guide_configuration_ll>`.
  * [DESK-971] Fixed a bug where user was unable to bond keyboard with a macOS host again.
  * [DESK-969] Fixed a bug where the bond switching in Gaming Mouse would stop working.
  * [DESK-967] Fixed a bug where :ref:`nrf_desktop_config_channel` module would impact the report rate as it should use write without response for sending data to peripheral.
  * [DESK-965] Fixed a bug where direct advertising to non-Zephyr centrals would not work.

Bluetooth Mesh
--------------

* Added:

  * :ref:`bluetooth_mesh_light_lc` sample - This sample demonstrates how to set up a light control Mesh server model application and control a dimmable LED with the Bluetooth Mesh, using the Generic OnOff models.
  * :ref:`bluetooth_mesh_sensor_client` - This sample demonstrates how to set up a basic Bluetooth Mesh sensor client model application that gets sensor data from one sensor server model.
  * :ref:`bluetooth_mesh_sensor_server` - This sample demonstrates how to set up a basic Mesh sensor server model application that provides sensor data to one Mesh sensor client model.
  * :ref:`bt_mesh_time_readme` - These models allow network-wide time and date synchronization.
  * :ref:`bt_mesh_light_ctl_readme` - These models allow remote control and configuration of CTLs on a mesh device.
  * :ref:`bt_mesh_scene_readme` - These models allow storing the model state of the entire mesh network as a *scene*, which can be recalled at a later time.
  * Added support for Mesh Device Properties v2.0.
  * Added :kconfig:option:`CONFIG_BT_TINYCRYPT_ECC` option in :file:`prj.conf` files for samples that support nRF5340 (:ref:`bluetooth_mesh_light` or :ref:`bluetooth_mesh_light_switch`).

* Updated:

  * Changed :ref:`bt_mesh_light_ctrl_readme`, so that the light control regulator now uses floating point.
  * Removed the necessity of setting several Kconfig options when using nRF5340 PDK with :ref:`bluetooth_mesh_light` or :ref:`bluetooth_mesh_light_switch`.

* Fixed several bugs and improved documentation.

nRF IEEE 802.15.4 radio driver
------------------------------

* Added support for multiprotocol with :ref:`nrfxlib:softdevice_controller`.

Thread
------

* Added:

  * Production support for nRF52833 and nRF52840 DKs.
  * Dynamic multiprotocol support with Bluetooth LE.
  * Support for :ref:`thread_architectures_designs_cp_ncp` architecture.
  * Support for Vendor hooks.
  * :ref:`ot_coprocessor_sample` sample - This sample demonstrates the usage of OpenThread Network Co-Processor architecture inside the Zephyr environment.
    It has the following characteristics:

    * Extendable with vendor hooks
    * Hardware cryptography acceleration support
    * Support for :ref:`Spinel logging <ug_logging_backends>` as a default logger backend
    * UART hardware flow control (HWFC) enabled by default

  * Initial support for :ref:`Thread 1.2 functionalities <thread_ug_supported_features_v12>` related to SED implementation and reduction of power consumption.

* Enabled Thread v1.1 certification by inheritance.
  For more information about certification, see :ref:`ug_thread_cert`.
* Updated:

  * :ref:`ot_cli_sample` sample:

    * Switched to RNG peripheral as an entropy source.
    * Added support for :ref:`Thread Certification Test Harness <ug_thread_cert>`.
    * Added hardware cryptography acceleration support.
    * Enabled UART HWFC by default.
    * Added an overlay for Thread v1.2 support.

  * :ref:`coap_client_sample` sample:

    * Switched to RNG peripheral as an entropy source.
    * Optimized power in SED mode.
    * Added hardware cryptography acceleration support.
    * Added :ref:`multiprotocol support <coap_client_sample_multi_ext>`.
    * Enabled UART HWFC by default.

  * :ref:`coap_server_sample` sample:

    * Switched to RNG peripheral as an entropy source.
    * Added hardware cryptography acceleration support.
    * Enabled UART HWFC by default.

Zigbee
------

* Added:

  * :ref:`lib_zigbee_fota` (DFU) support for nRF52840.
  * Support for Command Line Interface (:ref:`lib_zigbee_shell`).
    This is a port from the nRF5 SDK for Thread and Zigbee.
  * Support for :ref:`Network Co-Processor (NCP) architecture <ug_zigbee_platform_design_ncp>`.
    The NCP host package for Zigbee can be downloaded from https://developer.nordicsemi.com/

* Updated:

  * :ref:`zigbee_light_switch_sample` sample with :ref:`multiprotocol extension <zigbee_light_switch_sample_nus>` based on Bluetooth LE :ref:`nus_service_readme`.
  * Updated :ref:`zboss` to version ``v3_3_0_5+10_06_2020``.
    See :ref:`nrfxlib:zboss_changelog` for detailed information.

Common
======

The following changes are relevant for all device families.

Crypto
------

* Added:

  * nRF Oberon v3.0.7.
    See the :ref:`nrfxlib:crypto_changelog_oberon` for detailed information.
  * nrf_cc3xx_platform v0.9.4, with the following highlights:

    * Renamed include files from :file:`nrf_cc310_platform_xxxx.h` to :file:`nrf_cc3xx_platform_xxxx.h`.
    * Added experimental support for Arm CryptoCell CC312 available on nRF5340 devices.
    * Added APIs to store cryptographic keys in the KMU hardware peripheral available on nRF9160 and nRF5340 devices.
      For details, see :file:`crypto/nrf_cc310_platform/include/nrf_cc3xx_platform_kmu.h` in `sdk-nrfxlib`_.
    * Added APIs to generate CSPRNG.
      For details, see :file:`crypto/nrf_cc310_platform/include/nrf_cc3xx_platform_ctr_drbg.h` in `sdk-nrfxlib`_.

    See the :ref:`crypto_changelog_nrf_cc3xx_platform` for detailed information.
  * nrf_cc3xx_mbedcrypto version v0.9.4, with the following highlights:

    * Added experimental support for Arm CryptoCell CC312 available on nRF5340 devices.
    * Added APIs to derive cryptographic key material from KDR on nRF52840 and nRF9160 devices.
      For details, see :file:`crypto/nrf_cc310_mbedcrypto/include/mbedtls/cc3xx_kmu.h` in `sdk-nrfxlib`_.
    * Added APIs to use keys stored in KMU on nRF9160 and nRF5340 devices.
      For details, see :file:`crypto/nrf_cc310_mbedcrypto/include/mbedtls/cc3xx_kmu.h` in `sdk-nrfxlib`_.

    See the :ref:`crypto_changelog_nrf_cc3xx_mbedcrypto` for detailed information.
  * RNG support in nRF5340 application core (using Secure Partition Manager's Secure Services and nrf_cc312_platform library).

* Updated:

  * Renamed all APIs with ``cc310`` in the name to ``cc3xx`` because of added support for CC312.
    This change also affects :ref:`nrf_security`'s Kconfig options, where ``_CC310_`` was replaced with ``_CC3XX_`` in option names.
    The ``nrf_cc310_bl`` still uses the ``cc310`` naming scheme.
  * Updated :ref:`nrf_security` to use mbedTLS v2.23.0.
  * Disabled the CMAC glue layer, as it was causing issues.
    Now CMAC is provided by a single selected backend (through Kconfig).

Date-Time
---------

* Added:

  * Functions to clear current time: :c:func:`date_time_clear` and :c:func:`date_time_timestamp_clear`.

* Updated:

  * :c:func:`date_time_set` now returns an error code.
  * ``date_time_update()`` is now :c:func:`date_time_update_async` and returns an error code.

Drivers
-------

* Added:

  * :ref:`uart_nrf_sw_lpuart` - This driver implements the standard asynchronous UART API.
    The following samples related to this driver were also added:

    * :ref:`lpuart_sample` - This sample demonstrates the capabilities of the low power UART driver module.
    * :ref:`bluetooth-hci-lpuart-sample` - This sample demonstrates using the low power UART driver for HCI UART communication.

NFC
---

* Added:

  * :ref:`nfc_tnep_ch_readme` service - This library handles the exchange of Connection Handover Messages between an NFC Forum Tag and an NFC Forum Poller device.
  * :ref:`central_nfc_pairing` sample - This sample demonstrates Bluetooth LE out-of-band pairing using an NFC Reader ST25R3911B and the NFC TNEP protocol.
  * :ref:`peripheral_nfc_pairing` sample - This sample demonstrates Bluetooth LE out-of-band pairing using an NFC tag and the NFC TNEP protocol.

* Updated NFC samples to support non-secure domain builds for nRF5340.

nrfx
----

See the `Changelog for nrfx 2.3.0`_ for detailed information.

MCUboot
=======

* Updated MCUboot to facilitate using it as the second stage bootloader:

  * Added minimal configuration overlay file for MCUboot that makes it fit within 16 kB when MCUboot is used as the second stage bootloader.
    Updated :ref:`documentation <ug_bootloader_adding>` with specific instructions on how and when to use this configuration.

sdk-mcuboot
-----------

The `sdk-mcuboot`_ fork in |NCS| contains all commits from the upstream MCUboot repository up to and including ``5a6e18148d``, plus some |NCS| specific additions.

The following list summarizes the most important changes inherited from upstream MCUboot:

  * Fixed an issue where after erasing an image, an image trailer might be left behind.
  * Added a ``CONFIG_BOOT_INTR_VEC_RELOC`` option to relocate interrupts to application.
  * Fixed single image compilation with serial recovery.
  * Added support for single-image applications (see ``CONFIG_SINGLE_IMAGE_DFU``).
  * Added a ``CONFIG_BOOT_SIGNATURE_TYPE_NONE`` option to disable the cryptographic check of the image.
  * Reduced the minimum number of members in SMP packet for serial recovery.
  * Introduced direct execute-in-place (XIP) mode (see ``CONFIG_BOOT_DIRECT_XIP``).
  * Fixed kramdown CVE-2020-14001.
  * Modified the build system to let the application use a private key that is located in the user project configuration directory.
  * Added support for nRF52840 with ECC keys and CryptoCell.
  * Allowed to set VTOR to a relay vector before chain-loading the application.
  * Allowed using a larger primary slot in swap-move.
    Before, both slots had to be the same size, which imposed an unused sector in the secondary slot.
  * Fixed bootstrapping in swap-move mode.
  * Fixed an issue that caused an interrupted swap-move operation to potentially brick the device if the primary image was padded.
  * Various fixes, enhancements, and changes needed to work with the latest Zephyr version.

Build system
============

* Updated :ref:`partition_manager`:

  * Added RAM partitioning through the partition manager.
  * Added the ``ncs_add_partition_manager_config`` function that allows out-of-tree users to specify partition manager configuration files.
  * Added a warning if no static partition manager configuration is provided when one image (or more) is not built from source in a multi-image build.

* Enabled choosing a build strategy for Zephyr's :zephyr:code-sample:`bluetooth_hci_ipc` sample when it is built as a child image.
  See :ref:`ug_multi_image` for details.
* Improved multi-core builds by disassociating domain names from board names.
* Bugfixes:

  * Fixed a bug where :file:`merged_domains.hex` would be generated in single domain builds.
  * Fixed a bug where :file:`zephyr/merged.hex` would not be updated when rebuilding a project.

Zephyr
======

sdk-zephyr
----------

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The `sdk-zephyr`_ fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``7a3b253ced``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 7a3b253ced ^v2.3.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^7a3b253ced

The current |NCS| release is based on Zephyr v2.4.0.
See the :ref:`Zephyr v2.4.0 release notes <zephyr:zephyr_2.4>` for the list of changes.

Additions specific to |NCS|
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following list contains |NCS| specific additions:

* Added support for the |NCS|'s :ref:`partition_manager`, which can be used for flash partitioning.
* Added the following network socket and address extensions to the :ref:`zephyr:bsd_sockets_interface` interface to support the functionality provided by the BSD library:

  * AF_LTE
  * NPROTO_AT
  * NPROTO_PDN
  * NPROTO_DFU
  * SOCK_MGMT
  * SO_RCVTIMEO
  * SO_BINDTODEVICE
  * SOL_PDN
  * SOL_DFU
  * SO_PDN_CONTEXT_ID
  * SO_PDN_STATE
  * SOL_DFU
  * SO_DFU_ERROR
  * TLS_SESSION_CACHE
  * SO_SNDTIMEO
  * MSG_TRUNC
  * SO_SILENCE_ALL
  * SO_IP_ECHO_REPLY
  * SO_IPV6_ECHO_REPLY

* Added support for enabling TLS caching when using the :ref:`zephyr:mqtt_socket_interface` library.
  See :c:macro:`TLS_SESSION_CACHE`.
* Updated the nrf9160ns DTS to support accessing the CryptoCell CC310 hardware from non-secure code.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* :ref:`Documentation versions <index>` - you can now switch between different versions of the documentation by selecting the version in the upper left-hand corner
* :ref:`gs_recommended_versions` - added
* :ref:`known_issues` - added
* :ref:`sample` - updated to include configuration information and to clarify the instructions for using the template
* :ref:`lib_bluetooth_services` - renamed several :file:`.rst` files for Bluetooth services
* :ref:`gs_testing` - updated with information about :ref:`testing_rtt`
* :ref:`ble_samples` and :ref:`app_event_manager_sample` sample - removed the outdated nRF51 DK entry from Requirements

nRF9160
-------

* :ref:`ug_nrf9160` - updated the :ref:`nrf9160_ug_band_lock` section; also updated with information about certification of different modem firmware versions and added a link to nRF9160 compatibility matrix
* :ref:`serial_lte_modem` - updated and extended with testing instructions and AT command reference
* :ref:`lte_sensor_gateway` - updated with information about how to use low power UART for communicating with the controller
* nRF9160: Asset Tracker - added a note about external antenna performance and updated the dependencies section with the listing of modules abstracted using LwM2M carrier OS abstraction layer
* :ref:`lwm2m_client` - updated with sections about DTLS support and bootstrap support
* :ref:`lwm2m_carrier`  - updated the dependencies section with the listing of modules abstracted via LwM2M carrier OS abstraction layer
* nRF9160: Simple MQTT - updated with configuration and testing sections

nRF5340
-------

* :ref:`subsys_pcd` library - added to the DFU library to provide functionality for transferring DFU images from the application core to the network core on the nRF5340 SoC
* :ref:`ug_nrf5340` - updated with information about network core (FOTA) upgrades for nRF5340 SoC

nRF Desktop
-----------

* :ref:`nrf_desktop_nrf_profiler_sync` - updated configuration and implementation details
* :ref:`nrf_desktop_cpu_meas` - updated configuration and implementation details
* :ref:`nrf_desktop_smp` - updated configuration and implementation details
* :ref:`nrf_desktop_config_channel` - updated transport configuration and listener configuration

Bluetooth Mesh
--------------

* :ref:`mesh_concepts` - added
* :ref:`mesh_architecture` - added
* :ref:`bt_mesh_dk_prov` - updated with clarifications about the UUID usage

Thread
------

* :ref:`ug_thread` - updated by reorganizing structure and adding new pages

  * :ref:`ug_thread_configuring` - added as a separate page (was a section of :ref:`ug_thread`); updated with information about :ref:`Enabling OpenThread in nRF Connect SDK <ug_thread_configuring_basic>`, :ref:`ug_thread_configuring_crypto`, :ref:`thread_ug_thread_specification_options`, :ref:`thread_ug_feature_sets`
  * :ref:`ug_thread_intro` - added as a container for conceptual pages about OpenThread

    * :ref:`thread_ug_supported_features` - added as a separate page (was a section of :ref:`ug_thread`)
    * :ref:`ug_thread_architectures` - added
    * :ref:`openthread_stack_architecture` - removed as a separate page; now a section of :ref:`ug_thread_architectures`
    * :ref:`thread_ot_memory` - added
    * :ref:`thread_ot_commissioning` - updated to include content from :ref:`thread_ot_commissioning_configuring_on-mesh`, which also received updates and not includes information valid for both Thread CLI and NCP samples
    * :ref:`thread_ot_commissioning_configuring_on-mesh` - removed as a separate page; now a section of :ref:`thread_ot_commissioning`

  * :ref:`ug_thread_tools` - added as a separate page (was a section of :ref:`ug_thread`); updated the list of available tools and added more information about :ref:`ug_thread_tools_tbr` and `wpantund`_.

Zigbee
------

* :ref:`lib_zigbee_signal_handler` - added
* :ref:`zigbee_light_switch_sample` - added a note about :file:`overlay.conf`
* :ref:`ug_zigbee` - updated by reorganizing structure and adding new pages

  * Zigbee overview - added as a separate page (was a section of :ref:`ug_zigbee`)
  * :ref:`ug_zigbee_architectures` - added
  * :ref:`ug_zigbee_configuring` - added as a separate page (was a section of :ref:`ug_zigbee`)
  * :ref:`ug_zigbee_configuring_libraries` - added
  * :ref:`ug_zigbee_tools` - added as a separate page (was a section of :ref:`ug_zigbee`)

NFC
---

* :ref:`lib_nfc_ndef` - updated and added the following subpages:

  * :ref:`nfc_ch`
  * :ref:`nfc_ndef_ch_rec_parser_readme`

Libraries
---------

* :ref:`lib_ram_pwrdn` - added
* :ref:`shell_bt_nus_readme` - updated to show how to run the :file:`shell_bt_nus.py` script
* :ref:`lib_eth_rtt` - added
* :ref:`lib_aws_iot` - updated with additional information about enabling connection polling
* :ref:`lib_download_client` - moved :ref:`cert_dwload` to :ref:`modem_key_mgmt`
* :ref:`lib_nrf_cloud` - updated cloud API usage section
* :ref:`lib_at_host` - added

User guides
-----------

* :ref:`ug_logging` - added
* :ref:`ug_multiprotocol_support` - added

nrfxlib
-------

* :ref:`nrf_security_readme` - updated :ref:`nrf_security_backend_config` and modified structure extensively to improve maintainability (restructured with better sections and headings)
* :ref:`nrfxlib:mpsl` - updated with :ref:`nrfxlib:mpsl_fem`

Known issues
************

See `known issues for nRF Connect SDK v1.4.0`_ for the list of issues valid for this release.
