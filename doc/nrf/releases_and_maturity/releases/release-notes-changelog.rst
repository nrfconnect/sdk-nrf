.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.1.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and other components that implement or support protocol functionality.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.1.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Added macOS 26 support (Tier 3) to the table listing :ref:`supported operating systems for proprietary tools <additional_nordic_sw_tools_os_support>`.
* Updated:

  * The required `SEGGER J-Link`_ version to v8.66.
  * Steps on the :ref:`install_ncs` page for installing the |NCS| and toolchain together.
    With this change, the separate steps to install the toolchain and the SDK were merged into a single step.

Board support
=============

* Added support for the nRF7002-EB II Wi-Fi shield for use with the nRF54LM20 DK board target.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

* Added support for extra images in DFU packages (multi-image binary and ZIP).
  This allows applications to extend the built-in DFU functionality with additional firmware images beyond those natively supported by the |NCS|, for example, firmware for external devices.
  See :ref:`lib_dfu_extra` for details.
* Added an option to restore progress after a power failure when using DFU multi-image with MCUboot.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

* Added the :ref:`ug_nrf54l_otp_map` page, showing the OTP memory allocation for the nRF54L Series devices.

Developing with nRF54H Series
=============================

* Updated the location of the merged binaries in direct-xip mode from :file:`build/zephyr` to :file:`build`.

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Thingy:91 X
===========================

|no_changes_yet_note|

Developing with Thingy:91
=========================

* Updated the title of the page about updating the Thingy:91 firmware using the Cellular Monitor app to :ref:`thingy91_update_firmware`.
* Removed the page about updating the Thingy:91 firmware using the Programmer app.
  Its contents are now available in the app documentation on the `Programming Nordic Thingy prototyping platforms`_ page.
  The :ref:`thingy91_partition_layout` section has been moved to the :ref:`thingy91_update_firmware` page.

Developing with Thingy:53
=========================

|no_changes_yet_note|

Developing with PMICs
=====================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

|no_changes_yet_note|

Developing with custom boards
=============================

|no_changes_yet_note|

Developing with coprocessors
============================

|no_changes_yet_note|

Security
========

* Added:

  * CRACEN and nrf_oberon driver support for nRF54LM20 and nRF54LV10.
    For the list of supported features and limitations, see the :ref:`ug_crypto_supported_features` page.

  * Support for disabling Internal Trusted Storage (ITS) on nRF54L series devices when using
    :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO` with Trusted Firmware-M (TF-M) through the
    :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` Kconfig option.

  * Support for AES in counter mode and CBC mode using CRACEN for the :zephyr:board:`nrf54lm20dk`.

* Updated:

  * The :ref:`security_index` page with a table that lists the versions of security components implemented in the |NCS|.
  * The :ref:`secure_storage_in_ncs` page with updated information about the secure storage configuration in the |NCS|.
    Also renamed the page from "Trusted storage in the |NCS|."
  * The :ref:`ug_crypto_supported_features` page with the missing entries for the HMAC key type (:kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HMAC`).
  * The :ref:`ug_nrf54l_crypto_kmu_supported_key_types` section specific for the nRF54L Series devices to list the supported algorithms for each key type.

Mbed TLS
--------

* Updated to version 3.6.5.

Trusted Firmware-M
------------------

* Updated:

  * The TF-M version to 2.2.0.
  * Documentation to clarify the support for TF-M on devices emulated using the nRF54L15 DK.
    nRF54L05 does not support TF-M.
    nRF54L10 supports TF-M experimentally.

* Removed several documentation pages from the :ref:`tfm_wrapper` section that were misleading or not relevant for understanding the TF-M integration in the |NCS|.
  The section now includes only pages that provide background information about TF-M design that are relevant for the |NCS|.

Protocols
=========

|no_changes_yet_note|

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth Mesh
--------------

* Updated the NLC profile configuration system:

  * Introduced individual profile configuration options for better user control.
  * Deprecated the :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_CONF` and :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_DEFAULT` Kconfig options.
    Existing configurations continue to work but you should migrate to individual profile options.

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

* Added the :ref:`esb_monitor_mode` feature.

Gazell
------

|no_changes_yet_note|

Matter
------

* Added documentation for leveraging Matter Compliant Platform certification through the Derived Matter Product (DMP) process.
  See :ref:`ug_matter_platform_and_dmp`.
* Updated to using the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option as the C library instead of :kconfig:option:`CONFIG_NEWLIB_LIBC`, in compliance with Zephyr requirements.
* Removed the ``CONFIG_CHIP_SPI_NOR`` and ``CONFIG_CHIP_QSPI_NOR`` Kconfig options.
* Released the `Matter Quick Start app`_ v1.0.0 as part of nRF Connect for Desktop.

Matter fork
+++++++++++

* Removed dependencies on Nordic DK-specific configurations in Matter configurations.
  See the `Migration guide for nRF Connect SDK v3.2.0`_ for more information.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

* Updated:

  * The :ref:`thread_sed_ssed` documentation to clarify the impact of the SSED configuration on the device's power consumption and provide a guide for :ref:`thread_ssed_fine_tuning` of SSED devices.
  * The platform configuration to use the :kconfig:option:`CONFIG_PICOLIBC` Kconfig opiton as the C library instead of :kconfig:option:`CONFIG_NEWLIB_LIBC`, in compliance with Zephyr requirements.

Wi-Fi®
------

|no_changes_yet_note|

Applications
============

* Removed the Serial LTE modem application.
  Instead, use `Serial Modem`_, an |NCS| add-on application.

Connectivity bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

* Added support for the nRF54LM20 DK working with both Thread and Wi-Fi protocol variants.
  For the Wi-Fi protocol variant, the nRF54LM20 DK works with the nRF7002-EB II shield attached.

* Updated:

  * The application to store a portion of the application code related to the nRF70 Series Wi-Fi firmware in the external flash memory by default.
    This change breaks the DFU between the previous |NCS| versions and the upcoming release.
    To fix this, you need to disable storing the Wi-Fi firmware patch in external memory.
    See the :ref:`migration guide <migration_3.2_required>` for more information.
  * By moving code from :file:`samples/matter/common/src/bridge` to :file:`applications/matter_bridge/src/core` and :file:`applications/matter_bridge/src/ble` directories.

nRF5340 Audio
-------------

* Added:

  * A way to store servers in RAM on the unicast client (gateway) side.
    The storage does a compare on server address to match multiple servers in a unicast group.
    This means that if another device appears with the same address, it will be treated as the same server.
  * Experimental support for stereo in :ref:`unicast server application<nrf53_audio_unicast_server_app_configuration_stereo>`.
  * The :ref:`Audio application API documentation <audio_api>` page.
  * The :ref:`config_audio_app_options` page.
  * The API documentation in the header files listed on the :ref:`audio_api` page.
  * Ability to connect by address as a unicast client.

* Updated:

  * The unicast client (gateway) application has been rewritten to support N channels.
  * The unicast client (gateway) application now checks if a server has a resolvable address.
    If this has not been resolved, the discovery process will start in the identity resolved callback.
  * The power measurements to be disabled by default in the default debug versions of the applications.
    To enable power measurements, see :ref:`nrf53_audio_app_configuration_power_measurements`.
  * The audio application targeting the :zephyr:board:`nrf5340dk` to use pins **P1.5** to **P1.9** for the I2S interface instead of **P0.13** to **P0.17**.
    This change was made to avoid conflicts with the onboard peripherals on the nRF5340 DK.
  * The documentation pages with information about the :ref:`SD card playback module <nrf53_audio_app_overview_architecture_sd_card_playback>` and :ref:`how to enable it <nrf53_audio_app_configuration_sd_card_playback>`.
  * The API documentation in the header files listed on the :ref:`audio_api` page.

* Removed the LC3 QDID from the :ref:`nrf53_audio_feature_support` page.
  The QDID is now listed in the `nRF5340 Bluetooth DNs and QDIDs Compatibility Matrix`_.

nRF Desktop
-----------

  * Updated:

    * The memory layouts for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to make more space for the application code.
      This change in the partition map of every nRF54LM20 configuration is a breaking change and cannot be performed using DFU.
      As a result, the DFU procedure will fail if you attempt to upgrade the application firmware based on one of the |NCS| v3.1 releases.
    * The application and MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
      The application image signature is verified with the CRACEN hardware peripheral.
    * The MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the KMU-based key storage.
      The public key used by MCUboot for validating the application image is securely stored in the KMU hardware peripheral.
      To simplify the programming procedure, the application is configured to use the automatic KMU provisioning.
      The KMU provisioning is performed by the west runner as a part of the ``west flash`` command when the ``--erase`` or ``--recover`` flag is used.
    * Application configurations to avoid using the deprecated Kconfig options :ref:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE <config_desktop_app_options>`.
      The configurations rely on Kconfig options specific to HID providers instead.
      The HID keypress queue sizes for HID consumer control (:ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>`) and HID system control (:ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>`) reports were decreased to ``10``.
    * Application configurations integrating the USB legacy stack (:ref:`CONFIG_DESKTOP_USB_STACK_LEGACY <config_desktop_app_options>`) to suppress build warnings related to deprecated APIs of the USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
      The configurations enable the :kconfig:option:`CONFIG_DEPRECATION_TEST` Kconfig option to suppress the deprecation warnings.
      The USB legacy stack is still used by default.
    * MCUboot configurations that support serial recovery over USB CDC ACM to enable the :kconfig:option:`CONFIG_DEPRECATION_TEST` Kconfig option to suppress deprecation warnings.
      The implementation of serial recovery over USB CDC ACM still uses the deprecated APIs of the USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
    * Configurations of the ``nrf52840dongle/nrf52840`` board target to align them after the ``bare`` variant of the board was introduced in Zephyr.
      The application did not switch to the ``bare`` board variant to keep backwards compatibility.
    * The :ref:`nrf_desktop_hid_state` to allow for delayed registration of HID report providers.
      Before the change was introduced, subscribing to a HID input report before the respective provider was registered triggered an assertion failure.
    * HID transports (:ref:`nrf_desktop_hids`, :ref:`nrf_desktop_usb_state`) to use the early :c:struct:`hid_report_event` subscription (:c:macro:`APP_EVENT_SUBSCRIBE_EARLY`).
      This update improves the reception speed of HID input reports in HID transports.
    * The :ref:`nrf_desktop_motion` implementations to align internal state names for consistency.
    * The :ref:`nrf_desktop_motion` implementation that generates simulated motion.
      Improved the Zephyr shell (:kconfig:option:`CONFIG_SHELL`) integration to prevent potential race conditions related to using preemptive execution context for shell commands.
    * The :c:struct:`motion_event` to include information if the sensor is still active or goes to idle state waiting for user activity (:c:member:`motion_event.active`).
      The newly added field is filled by all :ref:`nrf_desktop_motion` implementations.
      The :ref:`nrf_desktop_hid_provider_mouse` uses the newly added field to improve the synchronization of motion sensor sampling.
      After the motion sensor sampling is triggered, the provider waits for the result before submitting a subsequent HID mouse input report.
    * The :ref:`nrf_desktop_hid_state_pm` to skip submitting the :c:struct:`keep_alive_event` if the :c:enum:`POWER_MANAGER_LEVEL_ALIVE` power level is enforced by any application module through the :c:struct:`power_manager_restrict_event`.
      This is done to improve performance.
    * The documentation of the :ref:`nrf_desktop_hid_state` and default HID report providers to simplify getting started with updating HID input reports used by the application or introducing support for a new HID input report.
    * The default value of the :kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL_NORMAL_PRIORITY_TIMEOUT_US` Kconfig option to ``0``.
      This is done to start using high MPSL timeslot priority quicker and speed up non-volatile memory operations.
    * The number of preemptive priorities (:kconfig:option:`CONFIG_NUM_PREEMPT_PRIORITIES`).
      The Kconfig option value was increased to ``15`` (default value from Zephyr).
      The priority of ``10`` is used by default for preemptive contexts (for example, :kconfig:option:`CONFIG_BT_GATT_DM_WORKQ_PRIO` and :kconfig:option:`CONFIG_BT_LONG_WQ_PRIO`).
      The previously used Kconfig option value of ``11`` leads to using the same priority for the mentioned preemptive contexts as the lowest available application thread priority (used for example, by the log processing thread).
    * Application image configurations to explicitly specify the LED driver used by the :ref:`nrf_desktop_leds` (:kconfig:option:`CONFIG_CAF_LEDS_GPIO` or :kconfig:option:`CONFIG_CAF_LEDS_PWM`).
      Also, disabled unused LED drivers enabled by default to reduce memory footprint.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the application to change the default libc from the :ref:`zephyr:c_library_newlib` to the :ref:`zephyr:c_library_picolibc` to align with the |NCS| and Zephyr.

* Removed support for the ``thingy53/nrf5340/cpuapp/ns`` build target.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added the :ref:`bluetooth_path_loss_monitoring` sample.

* Added the :ref:`samples_test_app` application to demonstrate how to use the Bluetooth LE Test GATT Server and test Bluetooth LE functionality in peripheral samples.

* Updated the network core image applications for the following samples from the :zephyr:code-sample:`bluetooth_hci_ipc` sample to the :ref:`ipc_radio` application for multicore builds:

  * :ref:`bluetooth_conn_time_synchronization`
  * :ref:`bluetooth_iso_combined_bis_cis`
  * :ref:`bluetooth_isochronous_time_synchronization`
  * :ref:`bt_scanning_while_connecting`
  * :ref:`channel_sounding_ras_initiator`
  * :ref:`channel_sounding_ras_reflector`

  The :ref:`ipc_radio` application is commonly used for multicore builds in other |NCS| samples and projects.
  Hence, this is to align with the common practice.

* Removed support for the ``thingy53/nrf5340/cpuapp/ns`` build target from the following samples:

   * :ref:`peripheral_lbs`
   * :ref:`peripheral_status`
   * :ref:`peripheral_uart`

* Disabled legacy pairing in the following samples:

   * :ref:`central_nfc_pairing`
   * :ref:`power_profiling`

   Support for legacy pairing remains exclusively for :ref:`peripheral_nfc_pairing` sample to retain compatibility with older Andorid devices.

* :ref:`direct_test_mode` sample:

  * Updated by simplifying the 2-wire UART polling.
    This is done by replacing the hardware timer with the ``k_sleep()`` function.

Bluetooth Mesh samples
----------------------

* Added:

  * Support for external flash settings for the ``nrf52840dk/nrf52840``, ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54l15dk/nrf54l05/cpuapp`` board targets in all Bluetooth Mesh samples.
  * Support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target in all Bluetooth Mesh samples.

* :ref:`ble_mesh_dfu_distributor` sample:

   * Added:

    * Support for external flash memory for the ``nrf52840dk/nrf52840`` and the ``nrf54l15dk/nrf54l15/cpuapp`` as the secondary partition for the DFU process.

* :ref:`ble_mesh_dfu_target` sample:

  * Added:

    * Support for external flash memory for the ``nrf52840dk/nrf52840`` and the ``nrf54l15dk/nrf54l15/cpuapp`` as the secondary partition for the DFU process.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added polling toggle to **Button 1** (**Button 0** on nRF54 DKs) to start/stop the periodic Sensor Get loop, ensuring the functionality is available on all supported devices including single-button hardware.

  * Updated:

    * To demonstrate the Bluetooth :ref:`ug_bt_mesh_nlc` HVAC Integration profile.

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

    * Button functions.
      Assignments are shifted down one index to accommodate the new polling toggle.
      The descriptor action has been removed from button actions but is still available through mesh shell commands.

  * Removed support for the ``nrf52dk/nrf52832``, since it does not have enough RAM space after NLC support was added.

* :ref:`bluetooth_mesh_sensor_server`:

  * Updated:

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

* :ref:`bluetooth_mesh_light_dim`:

  * Updated:

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

* :ref:`bluetooth_mesh_light_lc`:

  * Updated:

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated:

    * The memory layout for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to make more space for the application code.
      This change in the nRF54LM20 partition map is a breaking change and cannot be performed using DFU.
      As a result, the DFU procedure will fail if you attempt to upgrade the sample firmware based on one of the |NCS| v3.1 releases.
    * The application and MCUBoot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
      Note, that the Fast Pair subsystem still uses the Oberon software library.
      The application image signature is verified with the CRACEN hardware peripheral.
    * The MCUBoot configuration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the KMU-based key storage.
      The public key used by MCUboot for validating the application image is securely stored in the KMU hardware peripheral.
      To simplify the programming procedure, the samples are configured to use the automatic KMU provisioning.
      The KMU provisioning is performed by the west runner as a part of the ``west flash`` command when the ``--erase`` or ``--recover`` flag is used.

* :ref:`fast_pair_input_device` sample:

  * Updated the application configuration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
    Note, that the Fast Pair subsystem still uses the Oberon software library.

Cellular samples
----------------

* Added:

  * The :ref:`nrf_cloud_coap_cell_location` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for nRF Cloud's cellular location service.
  * The :ref:`nrf_cloud_coap_fota_sample` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for FOTA updates.
  * The :ref:`nrf_cloud_coap_device_message` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for device messages.
  * The :ref:`nrf_cloud_mqtt_device_message` sample to demonstrate how to use the `nRF Cloud MQTT API`_ for device messages.

* Removed the SLM Shell sample.
  Use the `Serial Modem Host Shell`_ sample instead.

* :ref:`nrf_cloud_rest_cell_location` sample:

  * Added runtime setting of the log level for the nRF Cloud logging feature.

* Updated the following samples to use the new ``SEC_TAG_TLS_INVALID`` definition:

  * :ref:`modem_shell_application`
  * :ref:`http_application_update_sample`
  * :ref:`http_modem_delta_update_sample`
  * :ref:`http_modem_full_update_sample`

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for environment evaluation using the ``link enveval`` command.
    * Support for NTN NB-IoT to the ``link sysmode`` and ``link edrx`` commands.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed an issue where sporadically the application was stuck waiting for the device to connect to the internet.
    This was due to wrong :ref:`Connection Manager <zephyr:conn_mgr_overview>` initialization.

Cryptography samples
--------------------

* Added:

  * Support for the ``nrf54lv10dk/nrf54lv10a/cpuapp`` and ``nrf54lv10dk/nrf54lv10a/cpuapp/ns`` board targets to all samples (except :ref:`crypto_test`).
  * Support for the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board target in all supported cryptography samples.
  * The :ref:`crypto_kmu_cracen_usage` sample.

* Updated documentation of all samples for style consistency: added lists of cryptographic features used by each sample and sample output in the testing section.

* :ref:`crypto_persistent_key` sample:

  * Added support for the ``nrf54h20dk/nrf54h20/cpuapp`` board target, demonstrating use of Internal Trusted Storage (ITS) on the nRF54H20 DK.

* :ref:`crypto_aes_ctr` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

* :ref:`crypto_tls` sample:

  * Updated sample-specific Kconfig configuration structure and documentation.

* :ref:`crypto_aes_ctr` sample:

  * Added support for ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

* :ref:`crypto_aes_cbc` sample:

  * Added support for ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

* :ref:`crypto_persistent_key` sample:

  * Added support for the ``nrf54h20dk/nrf54h20/cpuapp`` board target, demonstrating use of Internal Trusted Storage (ITS) on the nRF54H20 DK.

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

* :ref:`dect_shell_application` sample:

  * Added:

    * ``dect perf`` command client: LBT (Listen Before Talk) support with configurable LBT period and busy threshold.

  * Updated:

    * PCC and PDC printings improved to show SNR and RSSI-2 values with actual dB/dBm resolutions.
    * ``dect perf`` command: improved operation schedulings to avoid scheduling conflicts and fix to TX the results in server side.
    * ``dect ping`` command: improved operation schedulings to avoid scheduling conflicts.

DFU samples
-----------

* Added:

  * The :ref:`dfu_multi_image_sample` sample to demonstrate how to use the :ref:`lib_dfu_target` library.
  * The :ref:`ab_sample` sample to demonstrate how to implement the A/B firmware update strategy using :ref:`MCUboot <mcuboot_index_ncs>`.
  * The :ref:`fw_loader_ble_mcumgr` sample that provides a minimal configuration for firmware loading using SMP over Bluetooth LE.
    This sample is intended as a starting point for developing custom firmware loader applications that work with the MCUboot bootloader.
  * The :ref:`single_slot_sample` sample to demonstrate how to maximize the available space for the application with MCUboot using firmware loader mode (single-slot layout).

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

* Added the :ref:`esb_monitor` sample to demonstrate how to use the :ref:`ug_esb` protocol in Monitor mode.

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Added:

  * The :ref:`matter_temperature_sensor_sample` sample that demonstrates how to implement and test a Matter temperature sensor device.
  * The :ref:`matter_contact_sensor_sample` sample that demonstrates how to implement and test a Matter contact sensor device.
  * The ``matter_custom_board`` toggle paragraph in the Matter advanced configuration section of all Matter samples that demonstrates how add and configure a custom board.
  * Support for the Matter over Wi-Fi on the nRF54LM20 DK with the nRF7002-EB II shield attached to all Matter over Wi-Fi samples.

* Updated:

  * All Matter over Wi-Fi samples and applications to store a portion of the application code related to the nRF70 Series Wi-Fi firmware in the external flash memory by default.
    This change breaks the DFU between the previous |NCS| versions and the |NCS| v3.2.0.
    To fix this, you need to disable storing the Wi-Fi firmware patch in external memory.
    See the :ref:`migration guide <migration_3.2_required>` for more information.
  * All Matter samples that support low-power mode to use the :ref:`lib_ram_pwrdn` feature with the nRF54LM20 DK.
    This change resulted in decreasing the sleep current consumption by more than two uA.

* :ref:`matter_lock_sample` sample:

   * Added a callback for the auto-relock feature.
     This resolves the :ref:`known issue <known_issues>` KRKNWK-20691.

Networking samples
------------------

* Added support for the nRF7002-EB II with the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target in the following samples:

  * :ref:`aws_iot`
  * :ref:`net_coap_client_sample`
  * :ref:`download_sample`
  * :ref:`http_server`
  * :ref:`https_client`
  * :ref:`mqtt_sample`
  * :ref:`udp_sample`

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

|no_changes_yet_note|

PMIC samples
------------

|no_changes_yet_note|

Protocol serialization samples
------------------------------

|no_changes_yet_note|

SDFW samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

SUIT samples
------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_hello_world` sample:

  * Added support for the ``nrf54lv10dk/nrf54lv10a/cpuapp/ns`` board target.
  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board target.

* :ref:`tfm_secure_peripheral_partition`

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board target.

Thread samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* Removed support for the nRF7002-EB II with the ``nrf54h20dk/nrf54h20/cpuapp`` board target from the following samples:

  * :ref:`wifi_station_sample`
  * :ref:`wifi_scan_sample`
  * :ref:`wifi_shell_sample`
  * :ref:`wifi_radio_test`
  * :ref:`ble_wifi_provision`
  * :ref:`wifi_provisioning_internal_sample`

Other samples
-------------

* Added the :ref:`secondary_boot_sample` sample that demonstrates how to build and boot a secondary application image on the nRF54H20 DK.

* :ref:`nrf_profiler_sample` sample:

  * Added a new testing step demonstrating how to calculate event propagation statistics.
    Also added the related test preset for the :file:`calc_stats.py` script (:file:`nrf/scripts/nrf_profiler/stats_nordic_presets/nrf_profiler.json`).

* :ref:`app_event_manager_profiling_tracer_sample` sample:

  * Added a new testing step demonstrating how to calculate event propagation statistics.
    Also added the related test preset for the :file:`calc_stats.py` script (:file:`nrf/scripts/nrf_profiler/stats_nordic_presets/app_event_manager_profiler_tracer.json`).

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Flash drivers
-------------

* Added a Kconfig option to configure timeout for normal priority MPSL request (:kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL_NORMAL_PRIORITY_TIMEOUT_US`) in MPSL flash synchronization driver (:file:`nrf/drivers/mpsl/flash_sync/flash_sync_mpsl.c`).
  After the timeout specified by this Kconfig option, a higher timeslot priority is used to increase the priority of the flash operation.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

   * :ref:`nrf_security_readme` library:

      * The ``CONFIG_CRACEN_PROVISION_PROT_RAM_INV_DATA`` Kconfig option has been renamed to :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`hids_readme` library:

  * Updated the report length of the HID boot mouse to ``3``.
    The :c:func:`bt_hids_boot_mouse_inp_rep_send` function only allows to provide the state of the buttons and mouse movement (for both X and Y axes).
    No additional data can be provided by the application.

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

* Added new library :ref:`lib_dfu_extra`.

Gazell libraries
----------------

|no_changes_yet_note|

Security libraries
------------------

|no_changes_yet_note|

Modem libraries
---------------

* Removed:

  * The AT command parser library.
    Use the :ref:`at_parser_readme` library instead.
  * The AT parameters library.
  * The Modem SLM library.
    Use the `Serial Modem Host`_ library instead.

* :ref:`lte_lc_readme` library:

  * Added:

    * Support for environment evaluation.
    * Support for NTN NB-IoT system mode.
    * eDRX support for NTN NB-IoT.
    * Support for new modem events :c:enumerator:`LTE_LC_MODEM_EVT_RF_CAL_NOT_DONE`, :c:enumerator:`LTE_LC_MODEM_EVT_INVALID_BAND_CONF`, and :c:enumerator:`LTE_LC_MODEM_EVT_DETECTED_COUNTRY`.
    * Description of new features supported by mfw_nrf91x1 and mfw_nrf9151-ntn in receive only functional mode.
    * Sending of the ``LTE_LC_EVT_PSM_UPDATE`` event with ``tau`` and ``active_time`` set to ``-1`` when registration status is ``LTE_LC_NW_REG_NOT_REGISTERED``.
    * New registration statuses and functional modes for the ``mfw_nrf9151-ntn`` modem firmware.

  * Updated:

    * The type of the :c:member:`lte_lc_evt.modem_evt` field to :c:struct:`lte_lc_modem_evt`.
    * Replaced modem events ``LTE_LC_MODEM_EVT_CE_LEVEL_0``, ``LTE_LC_MODEM_EVT_CE_LEVEL_1``, ``LTE_LC_MODEM_EVT_CE_LEVEL_2`` and ``LTE_LC_MODEM_EVT_CE_LEVEL_3`` with the :c:enumerator:`LTE_LC_MODEM_EVT_CE_LEVEL` modem event.
    * The order of the ``LTE_LC_MODEM_EVT_SEARCH_DONE`` modem event, and registration and cell related events.
      See the :ref:`migration guide <migration_3.2_required>` for more information.

  * Fixed an issue where band lock, RAI notification subscription, and DNS fallback address were lost when the modem was put into :c:enumerator:`LTE_LC_FUNC_MODE_POWER_OFF` functional mode.

* :ref:`nrf_modem_lib_readme` library:

  * Added the :c:func:`nrf_modem_lib_trace_peek_at` function to the :c:struct:`nrf_modem_lib_trace_backend` interface to peek trace data at a byte offset without consuming it.
    Support for this API has been added to the flash trace backend.

  * Removed the deprecated ``CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ZEPHYR`` kconfig option.

* :ref:`pdn_readme` library:

  * Fixed:

    * An issue where wrong APN rate control event was sent.
    * An issue where a malformed +CGEV notification was not handled correctly.

  * Removed the deprecated ``pdn_dynamic_params_get()`` function.
    Use the :c:func:`pdn_dynamic_info_get` function instead.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* Added missing brackets that caused C++ compilation to fail in the following libraries:

  * :ref:`lib_nrf_cloud_pgps`
  * :ref:`lib_nrf_cloud_fota`

* Updated the following libraries to use the new ``SEC_TAG_TLS_INVALID`` definition for checking whether a security tag is valid:

  * :ref:`lib_aws_fota`
  * :ref:`lib_fota_download`
  * :ref:`lib_ftp_client`

* Removed the Download client library.
  Use the :ref:`lib_downloader` library instead.

* :ref:`lib_nrf_provisioning` library:

  * Added a blocking call to wait for a functional-mode change, relocating the logic from the app into the library.

  * Updated:

    * By making internal scheduling optional.
      Applications can now trigger provisioning manually using the :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` Kconfig option.
    * By moving root CA provisioning to modem initialization callback to avoid blocking and ensure proper state.
    * By expanding the event handler to report more provisioning events, including failures.
    * By making the event handler callback mandatory to notify the application of failures and prevent silent errors.
    * By unifying the device‐mode and modem‐mode callbacks into a single handler for cleaner integration.
    * The documentation and sample code accordingly.

  * Fixed multiple bugs and enhanced error handling.

* :ref:`lib_nrf_cloud_rest` library:

  * Deprecated the library.
    Use the :ref:`lib_nrf_cloud_coap` library instead.

* :ref:`lib_nrf_cloud_fota` library:

  * Fixed occasional message truncation notifying that the download was complete.

* :ref:`lib_nrf_cloud_log` library:

  * Updated by adding a missing CONFIG prefix.

* :ref:`lib_nrf_cloud` library:

  * Added the :c:func:`nrf_cloud_obj_location_request_create_timestamped` function to make location requests for past cellular or Wi-Fi scans.
  * Updated by refactoring the folder structure of the library to separate the different backend implementations.

* :ref:`lib_downloader` library:

  * Fixed an issue where HTTP download would hang if the application had not set the socket receive timeout and data flow from the server stopped.
    The HTTP transport now sets the socket receive timeout to 30 seconds by default.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* :ref:`nrf_profiler` library:

  * Updated the documentation by separating out the :ref:`nrf_profiler_script` documentation.

* :ref:`lib_ram_pwrdn` library:

  * Added support for the nRF54LM20A SoC.

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

* Added:

  * The :ref:`esb_sniffer_scripts` scripts for the :ref:`esb_monitor` sample.
  * The documentation page for :ref:`nrf_profiler_script`.
    The page also describes the script for calculating statistics (:file:`calc_stats.py`).

* Removed:

  * The SUIT support from the :ref:`nrf_desktop_config_channel_script`.
    The :ref:`nrf_desktop_config_channel_script` from older |NCS| versions can still be used to perform the SUIT DFU operation.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Removed the Fast Pair TinyCrypt cryptographic backend (``CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT``), because the TinyCrypt library support was removed from Zephyr.
  You can use either the Fast Pair Oberon cryptographic backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON`) or the Fast Pair PSA cryptographic backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

* Updated:

  * The ``CONFIG_MEMFAULT_DEVICE_INFO_CUSTOM`` Kconfig option has been renamed to :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_INFO_CUSTOM`.
  * The ``CONFIG_MEMFAULT_DEVICE_INFO_BUILTIN`` Kconfig option has been renamed to :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_INFO_BUILTIN`.
  * The :kconfig:option:`CONFIG_MEMFAULT_NCS_FW_VERSION` Kconfig option will now have a default value set from a :file:`VERSION` file, if present in the application root directory.
    Previously, this option had no default value.

  * Simplified the options for ``CONFIG_MEMFAULT_NCS_DEVICE_ID_*``, which sets the Memfault Device Serial. The default is now :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID`, which uses the :ref:`lib_hw_id` library to provide a unique device ID.
    See the `Migration guide for nRF Connect SDK v3.2.0`_ for more information.

* Added a metric tracking the unused stack space of the Bluetooth Long workqueue thread, when the :kconfig:option:`CONFIG_MEMFAULT_NCS_BT_METRICS` Kconfig option is enabled.
  The new metric is named ``ncs_bt_lw_wq_unused_stack``.

* Removed a metric for the tracking Bluetooth TX thread unused stack ``ncs_bt_tx_unused_stack``.
  The thread in question was removed in Zephyr v3.7.0.

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

|no_changes_yet_note|

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``81315483fcbdf1f1524c2b34a1fd4de6c77cd0f4``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added support for S2RAM resume on nRF54H20 devices.
  MCUboot acts as the S2RAM resume mediator and redirects execution to the application's native resume routine.

* Updated KMU mapping to ``BL_PUBKEY`` when MCUboot is used as the immutable bootloader for nRF54L Series devices.
  You can restore the previous KMU mapping (``UROT_PUBKEY``) with the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_KMU_UROT_MAPPING` Kconfig option.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``0fe59bf1e4b96122c3467295b09a034e399c5ee6``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 0fe59bf1e4 ^fdeb735017

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^0fe59bf1e4

The current |NCS| main branch is based on revision ``0fe59bf1e4`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * The :ref:`debug_log_bt_stack` section to the :ref:`gs_debugging` page.
  * The :ref:`app_power_opt_opp` page to the :ref:`app_optimize` section.

* Updated:

  * The :ref:`emds_readme_application_integration` section in the :ref:`emds_readme` library documentation to clarify the EMDS storage context usage.
  * The Emergency data storage section in the :ref:`bluetooth_mesh_light_lc` sample documentation to clarify the EMDS storage context implementation and usage.
  * The :ref:`ble_mesh_dfu_distributor` sample documentation to clarify the external flash support.
  * The :ref:`ble_mesh_dfu_target` sample documentation to clarify the external flash support.
  * The :ref:`app_power_opt_nRF91` page by moving it under the :ref:`ug_lte` section.
