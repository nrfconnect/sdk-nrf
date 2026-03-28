.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.2.99
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
See `known issues for nRF Connect SDK v3.2.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated the required `SEGGER J-Link`_ version to v9.24a.

Board support
=============

|no_changes_yet_note|

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

* Moved the :ref:`mcuboot_with_decompression` sample to :file:`samples/dfu/compressed_update` and
  updated the sample name from "nRF Compression: MCUboot compressed update".

* :ref:`nrf_smp_svr_sample` sample:

  * Updated by moving the sample to the :file:`samples/dfu/smp_svr` folder.

* :ref:`firmware_loader_entrance` sample:

  * Added the :ref:`sample documentation page <firmware_loader_entrance>`.

* :ref:`mcuboot_with_encryption` sample:

  * Added support for the nRF54H20 SoC.

* Renamed the existing lifecycle state (LCS) type ``lcs`` to :c:enum:`bl_storage_lcs`.

* Added a new lifecycle state (LCS) API designed for nRF54L Series devices.

* Added support for firmware loader updates using the :ref:`installer` application.

* Replaced the ``SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE`` Kconfig option with :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KEY_FILE`.
  This change allows generating a default key file for MCUboot when using either the KMU or ITS for key storage.

  Affected samples:

  * :ref:`nrf_desktop`
  * :ref:`fast_pair_locator_tag`
  * :ref:`mcuboot_with_encryption`
  * :ref:`single_slot_sample`
  * :ref:`nrf_smp_svr_sample`
  * :ref:`mcuboot_minimal_configuration`

* Replaced ``ExternalNcsVariantProject_Add(..)`` CMake function with the ``ExternalZephyrVariantProject_Add(..)`` function.
  The new function is defined and maintained in the Zephyr repository, providing a very similar API and functionality.
  The old function is deprecated and kept for backward compatibility, but it will be removed in the future.

* Moved the UUID configuration for the MCUboot images from the Kconfig options to the Devicetree.
  The UUIDs are now defined in the Devicetree overlay files for each image.
  For more information, see the :ref:`mcuboot_config`.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

* Added:

  * Experimental support for nRF54LS05B.
  * Experimental support for nRF54LM20B.

Developing with nRF54H Series
=============================

* Added:

  * A document describing the merged slot update strategy for nRF54H20 devices, allowing simultaneous updates of both application cores (APP and RAD) in a single update operation.
    For more information, see :ref:`ug_nrf54h20_partitioning_merged`.
  * A document describing the manifest-based update strategy for nRF54H20 devices.
    For more information, see :ref:`ug_nrf54h20_mcuboot_manifest`.
  * The :ref:`ug_nrf54h20_mcuboot_requests` page that describes bootloader requests for the nRF54H20 SoC.
    It explains how you can pass information from the application to the bootloader.

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

|no_changes_yet_note|

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

Security
========

* Added:

  * Support for the WPA3-SAE and WPA3-SAE-PT in the :ref:`CRACEN driver <crypto_drivers_cracen>`.
  * Support for the HMAC KDF algorithm in the CRACEN driver.
    The algorithm implementation is conformant to the NIST SP 800-108 Rev. 1 recommendation.
  * Support for the secp384r1 key storage in the :ref:`Key Management Unit (KMU) <ug_kmu_guides_supported_key_types>`.
  * Support for AES-GCM AEAD using CRACEN for the :ref:`nrf54lm20dk <app_boards>` board.
  * Support for ChaCha20-Poly1305 AEAD using CRACEN for the :ref:`nrf54lm20dk <app_boards>` board.
  * Support for AES Key Wrap (AES-KW) and AES Key Wrap with Padding (AES-KWP) algorithms in the :ref:`nrf_oberon <crypto_drivers_oberon>` and :ref:`CRACEN <crypto_drivers_cracen>` drivers.
    The :ref:`Supported cryptographic operations in the nRF Connect SDK <ug_crypto_supported_features_key_wrap_algorithms>` page has been updated accordingly.
  * Doxygen API group hierarchy files for the CRACEN driver: :file:`cracen_api_structure.h` (top-level), :file:`cracen_sw/include/cracen_sw_api_structure.h` (software implementations), and :file:`common/include/cracen_common_api_structure.h` (common utilities).
    The :ref:`API documentation section for the cryptographic drivers <crypto_drivers_api_documentation>` has been updated accordingly.
  * Support for storing AES Key Wrap (AES-KW) keys in the KMU.

* Updated:

  * The :ref:`ug_kmu_provisioning_production_metadata` section to reflect bit-field size changes in the :c:struct:`kmu_metadata` structure.
  * The :ref:`API documentation section for the cryptographic drivers <crypto_drivers_api_documentation>` with links to the added API documentation for the CRACEN driver.
  * The Oberon PSA Crypto to version 1.5.4 that introduces support for the following new features with the Oberon PSA driver:

    * Support for PSA Certified Crypto API v1.4, PSA Crypto API v1.4 PQC Extension and PSA Crypto Driver Interface v1.0 alpha 1.
    * Experimental support for Extendable-Output Function (XOF) algorithms SHAKE128, SHAKE256, ASCON XOF128 and ASCON CXOF128.
    * Experimental support for hash ML-DSA and deterministic ML-DSA asymmetric signature algorithms.
    * Experimental support for ASCON HASH256 hash algorithm.
    * Experimental support for ASCON AEAD128 AEAD algorithm.
    * Updated implementations of WPA3-SAE, ML-DSA and ML-KEM to support the PSA Crypto API v1.4.

    The :ref:`ug_crypto_supported_features` page has been updated accordingly.

  * The :ref:`ug_crypto_supported_features` page with information about support for the Curve448 (X448) elliptic curve under :ref:`ug_crypto_supported_features_signature_algorithms` and :ref:`ug_crypto_supported_features_ecc_curve_types`.
  * The :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048` Kconfig option is now enabled by default whenever :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC` or :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY` is enabled and no other RSA key size is enabled.
  * The :ref:`app_approtect` page with more detailed information about the AP-Protect implementation types and how to configure them for each device.

  * The :ref:`API documentation section for the cryptographic drivers <crypto_drivers_api_documentation>` with links to the added API documentation for the CRACEN driver.
  * The documentation with the information about the side-channel countermeasures in the CRACEN peripheral:

    * The :ref:`ug_crypto_supported_features` page with the :ref:`ug_crypto_supported_features_countermeasures` section with information about support for the countermeasures in the CRACEN driver.
    * The :ref:`ug_kmu_guides_cracen_overview` page with the section about :ref:`ug_kmu_cracen_countermeasures`.

* Removed:

  * The ``CONFIG_PSA_WANT_KEY_TYPE_WPA3_SAE_PT`` Kconfig option and replaced it with :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_WPA3_SAE`.
  * The ``CONFIG_PSA_WANT_ALG_WPA3_SAE`` Kconfig option and replaced it by options :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_FIXED` and :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_GDH`.
  * The ``CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOT_BUFFER_SIZE`` Kconfig option.
    The PSA Crypto API is able to infer the key slot buffer size based on the keys enabled in the build, so there is no need to define it manually.
  * The ``CONFIG_MBEDTLS_MAC_SHA256_ENABLED`` Kconfig option.

Trusted Firmware-M (TF-M)
-------------------------

* Added support for the PSA Key Wrapping functions from the PSA Certified Crypto API v1.4.0.

* Updated:

  * To version 2.2.2.
  * The default TF-M profile is now :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` for all configurations except for Thingy:91 and Thingy:91 X, on which it is still :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL`.
  * The :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` Kconfig option now supports enabling PSA crypto hash functions.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

* Updated the Direct Test Mode HCI commands by making them optional through the :kconfig:option:`CONFIG_BT_CTLR_DTM_HCI` Kconfig option.

Bluetooth Mesh
--------------

|no_changes_yet_note|

DECT NR+
--------

* Added DECT NR+ full stack support, including the following features:

  * Connection Manager integration for enabling easy connect for the applications.
  * Network management API for controlling DECT NR+ operations.
  * L2 API implementation enabling IPv6 connectivity, including HAL definition.
  * Enabling Internet connectivity through sink and BR (Border Router) support together with, for example, Serial Modem on a gateway nRF91 LTE device.
  * nRF91x1 DECT modem driver implementing HAL API and interfacing with DECT NR+ modem firmware where the DECT NR+ MAC layer is running.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Experimental support for concurrent operation of ESB with other radio protocols using the :ref:`mpsl_timeslot` feature.
  * The :kconfig:option:`CONFIG_ESB_CLOCK_INIT` Kconfig option.
    When this option is enabled, the :c:func:`esb_init` function automatically starts the required high-frequency clock and applies platform-specific errata workarounds.
    Applications that manage clocks independently can leave this option disabled (default).

* Fixed invalid radio configuration for legacy ESB protocol.

Gazell
------

|no_changes_yet_note|

Matter
------

* Added support for nRF54LM20B SoC in Matter samples and applications.
* Updated:

  * The :ref:`matter_test_event_triggers_default_test_event_triggers` section with the new Closure Control cluster test event triggers.
  * Decreased Matter OTA image transfer time by around 15%.
  * By enabling PSA Crypto support by default, even when Wi-Fi is enabled.

* Deprecated the secure persistent storage backend enabled with the :option:`CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND` Kconfig option.

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

* Added:

  * A warning when using precompiled OpenThread libraries with modified Kconfig options related to the OpenThread stack.
  * The otperf tool that is compatible with zperf and iperf for measuring throughput of thread transmissions.
    For more details, see :ref:`ug_otperf_tool`.
  * Support for nRF54LM20B SoC in Thread samples.

Wi-Fi®
------

* Added deprecated Wired Equivalent Privacy (WEP) support for backward compatibility with legacy networks.
  The support is enabled through the :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_WEP` Kconfig option and is not recommended for new deployments.

* Updated by enabling PSA Crypto support by default for personal security modes.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

* Added support for nRF54LM20B SoC.
* Updated partitions mapping for the nRF7002 DK in the application.
  See the :ref:`migration guide <migration_3.3_required>` for more information.
* Removed the ``onoff_plug`` snippet from the application.
  To build the application with the ``onoff_plug`` functionality, use the :file:`overlay-onoff_plug.conf` configuration overlay file.

nRF5340 Audio
-------------

* Added:

  * Dynamic configuration of the number of channels for the encoder based on the configured audio locations.
    The number of channels is set during runtime using the :c:func:`audio_system_encoder_num_ch_set` function.
    This allows configuring mono or stereo encoding depending on the configured audio locations, potentially saving CPU and memory resources.

  * High CPU load callback using the Zephyr CPU load subsystem.
    The callback uses a :c:func:`printk` function, as the logging subsystem is scheduled out if higher priority threads take all CPU time.
    This makes debugging high CPU load situations easier in the application.
    The threshold for high CPU load is set in :file:`peripherals.c` using :c:macro:`CPU_LOAD_HIGH_THRESHOLD_PERCENT`.

  * :kconfig:option:`CONFIG_SPEED_OPTIMIZATIONS` to enable compiler speed optimizations for the application.

  * Support for multiple independent coordinated sets in :ref:`unicast client app<nrf53_audio_unicast_client_app>`.
    When all the devices in a coordinated set are disconnected, the SIRK is cleared, allowing a new unicast group to be formed with a new SIRK without the need to restart the application.

* Updated:

  * The application to switch to the new USB stack introduced in Zephyr 3.4.0.
    This change requires no action from you.
    macOS now works out of the box, fixing OCT-2154.

  * Programming script.
    Devices are now halted before programming.
    Furthermore, the devices are kept halted until they are all programmed, and then started together
    with the headsets starting first.
    This eases sniffing of advertisement packets.

  * The audio data path to now accumulate 10 ms frames instead of processing 1 ms blocks individually, reducing message queue operations by 90% and improving overall system performance.
    This optimization affects both USB audio input processing and the I2S audio datapath, resulting in more efficient encoder thread operation.

  * The application to improved handling of I2S RX buffer overruns.
    When an overrun occurs, the most recent block in the current frame is removed to make space for new incoming data.

  * The application to optimize USB-to-encoder audio processing pipeline to reduce CPU usage.
    LC3-encoding of sinusoidal input demands more of the CPU than real-world audio input.

  * The application to improved error handling with ``unlikely()`` macros for better branch prediction in performance-critical paths.

  * The application to separate the audio clock configuration into a dedicated module.
    This allows for better organization and potential reuse of the audio clock configuration code between different SoCs that might not have the high-frequency audio clock (HFCLKAUDIO) feature.
    The new module provides an initialization function for setting up the audio clock and a function for configuring the audio clock frequency.

* Removed:

  * The Bluetooth controller watchdog from the application.
    The watchdog was not providing value and the removal allows for easier porting to other platforms that do not have a multi-core architecture.

  * The DFU options from the application.
    The nRF5340 Audio applications have for some time used standard |NCS| tools to perform a DFU.
    Hence, the applications use the same process as other Bluetooth projects.
    See :ref:`app_bootloaders` for information on how to set up DFU for their requirements.

  * The note about missing support in |nRFVSC|.
    The section about programming using standard methods now lists the steps for |nRFVSC| and the command line from the :ref:`nrf53_audio_app_building` document.
    This is because, with the latest release of |nRFVSC|, you can build and program the nRF5340 Audio application using the |nRFVSC| GUI.

nRF Desktop
-----------

* Added:

  * A workaround for the USB next stack race issue where the application could try to submit HID reports while the USB is being disabled after USB cable has been unplugged, which results in an error.
    The workaround is applied when the :option:`CONFIG_DESKTOP_USB_STACK_NEXT_DISABLE_ON_VBUS_REMOVAL` Kconfig option is enabled.
  * Application-specific Kconfig options that simplify using SEGGER J-Link RTT (:option:`CONFIG_DESKTOP_LOG_RTT`) or UART (:option:`CONFIG_DESKTOP_LOG_UART`) as logging backend used by the application (:option:`CONFIG_DESKTOP_LOG`).
    The :kconfig:option:`CONFIG_LOG_BACKEND_UART` and :kconfig:option:`CONFIG_LOG_BACKEND_RTT` Kconfig options are no longer enabled by default if nRF Desktop logging (:option:`CONFIG_DESKTOP_LOG`) is enabled.
    These options are controlled through the newly introduced nRF Desktop application-specific Kconfig options.
    The application still uses SEGGER J-Link RTT as the default logging backend.
  * Support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.
    The board target can act as either a mouse or a keyboard.
  * Support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target.
    The board target supports the same set of configurations as the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

* Updated:

  * The :option:`CONFIG_DESKTOP_BT` Kconfig option to no longer select the deprecated :kconfig:option:`CONFIG_BT_SIGNING` Kconfig option.
    The application relies on Bluetooth LE security mode 1 and security level of at least 2 to ensure data confidentiality through encryption.
  * The memory map for RAM load configurations of nRF54LM20 SoC to increase KMU RAM section size to allow for secp384r1 key.
  * The default log levels used by the legacy USB stack (:option:`CONFIG_DESKTOP_USB_STACK_LEGACY`) to enable error logs (:kconfig:option:`CONFIG_USB_DEVICE_LOG_LEVEL_ERR`, :kconfig:option:`CONFIG_USB_DRIVER_LOG_LEVEL_ERR`).
    Previously, the legacy USB stack logs were turned off.
    This change ensures visibility of runtime issues.
  * Application configurations that emit debug logs over UART to use the :option:`CONFIG_DESKTOP_LOG_UART` Kconfig option instead of explicitly configuring the logger.
    This is done to simplify the configurations.
  * The memory map for the nRF54H20 SoC to include a secure storage partition and adjust the size of existing partitions.
  * The dependencies of the :ref:`nrf_desktop_constlat` to allow using it for nRF54L Series SoC.
    The :option:`CONFIG_DESKTOP_CONSTLAT_ENABLE` Kconfig option no longer depends on :kconfig:option:`CONFIG_SOC_SERIES_NRF54L` being disabled.

* Removed the application-specific Kconfig option (``CONFIG_DESKTOP_RTT``) that enabled RTT for nRF Desktop logging (:option:`CONFIG_DESKTOP_LOG`) or nRF Desktop shell (:option:`CONFIG_DESKTOP_SHELL`).
  nRF Desktop shell automatically enables RTT by default (:kconfig:option:`CONFIG_USE_SEGGER_RTT`).
  You can use the newly introduced application-specific Kconfig option :option:`CONFIG_DESKTOP_LOG_RTT` for nRF Desktop RTT logging configuration.
  By default, this option makes the RTT log backend block the message until it is transferred to host (:kconfig:option:`CONFIG_LOG_BACKEND_RTT_MODE_BLOCK`) instead of dropping messages that do not fit in up-buffer (:kconfig:option:`CONFIG_LOG_BACKEND_RTT_MODE_DROP`).
  This is done to prevent dropping the newest logs.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Deprecated the :ref:`nrf_machine_learning_app` application.
  Replaced by the Gesture Recognition application in `Edge AI Add-on for nRF Connect SDK`_.

Thingy:53: Matter weather station
---------------------------------

Installer (MCUboot Firmware Loader installer)
-----------------------------------------------

* Added support for the Installer application which is a helper application for the MCUboot Firmware Loader update.

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* :ref:`central_uart` sample:

  * Added support for the ``nrf54lm20dongle/nrf54lm20b/cpuapp`` board target.

* Added support for the ``nrf54lm20dk/nrf54lm20b`` board target variant in the following samples:

  * :ref:`central_and_peripheral_hrs`
  * :ref:`central_bas`
  * :ref:`bluetooth_central_hr_coded`
  * :ref:`central_uart`
  * :ref:`multiple_adv_sets`
  * :ref:`peripheral_ams_client`
  * :ref:`peripheral_ancs_client`
  * :ref:`peripheral_bms`
  * :ref:`peripheral_cgms`
  * :ref:`peripheral_cts_client`
  * :ref:`peripheral_gatt_dm`
  * :ref:`peripheral_hr_coded`
  * :ref:`peripheral_lbs`
  * :ref:`peripheral_mds`
  * :ref:`peripheral_nfc_pairing`
  * :ref:`power_profiling`
  * :ref:`peripheral_rscs`
  * :ref:`peripheral_status`
  * :ref:`peripheral_uart`
  * :ref:`shell_bt_nus`
  * :ref:`ble_throughput`

* :ref:`bluetooth_central_hids` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target.

* :ref:`peripheral_ams_client` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board targets.

* :ref:`peripheral_ancs_client` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board targets.

* :ref:`peripheral_hids_keyboard` sample:

  * Added support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.
  * Added support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target.
  * Renamed sample-specific Kconfig options to use the ``SAMPLE_`` prefix to follow the Kconfig naming convention for samples.
  * Updated the sample's configuration to limit the number of supported HID reports (:kconfig:option:`CONFIG_BT_HIDS_FEATURE_REP_MAX`, :kconfig:option:`CONFIG_BT_HIDS_INPUT_REP_MAX`, and :kconfig:option:`CONFIG_BT_HIDS_OUTPUT_REP_MAX`) and the maximum number of GATT attribute descriptors (:kconfig:option:`CONFIG_BT_HIDS_ATTR_MAX`) in the :ref:`hids_readme`.
    This reduces the memory consumption of the sample.

* :ref:`peripheral_hids_mouse` sample:

  * Added:

    * Support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.
    * Support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target.
    * A sample-specific :option:`CONFIG_SAMPLE_BT_HIDS_SECURITY_MITM` Kconfig option.
      You can use this option to control support for passkey display MITM protection.
      The passkey display feature can be used only if the configuration emits printk messages (:kconfig:option:`CONFIG_PRINTK` Kconfig option must be enabled).
    * Configuration without debug features (``release`` configuration).
      You can use this configuration to verify the memory footprint of the sample as it is closer to the configuration used by a final product.

  * Updated:

    * Sample's configuration to limit the number of supported HID reports (:kconfig:option:`CONFIG_BT_HIDS_FEATURE_REP_MAX`, :kconfig:option:`CONFIG_BT_HIDS_INPUT_REP_MAX`, and :kconfig:option:`CONFIG_BT_HIDS_OUTPUT_REP_MAX`) and maximum number of GATT attribute descriptors (:kconfig:option:`CONFIG_BT_HIDS_ATTR_MAX`) in the :ref:`hids_readme`.
      This reduces the memory consumption of the sample.
    * The Bluetooth TX processor thread stack size (:kconfig:option:`CONFIG_BT_TX_PROCESSOR_STACK_SIZE` Kconfig option) to prevent stack overflow when receiving directed advertising right after boot.
      On the ``nrf54l15dk/nrf54l15/cpuapp`` board target, the maximum measured stack usage is 936 bytes.
    * The directed advertising from high-duty to low-duty cycle to prevent disconnecting peers that are already connected.
      Scheduling high-duty directed advertising might cause problems with reacting to Bluetooth LE connection events from connected peers, which can result in disconnections.
      The low-duty cycle directed advertising is used only when there is at least one connected peer.
      An application-specified timeout of two seconds is used for each low-duty cycle directed advertising attempt.

  * Renamed sample-specific Kconfig options to use the ``SAMPLE_`` prefix and dropped the ``_ENABLED`` suffix to follow the Kconfig naming convention for samples.

  * Fixed a :c:struct:`bt_conn` reference leak in the ``pairing_failed`` callback that occurred when the second queued MITM pairing request failed, causing advertising to fail with ``-ENOMEM``.

* :ref:`peripheral_mds`:

   * Replaced the nRF Memfault mobile applications with the `nRF Connect Device Manager`_.

Bluetooth Mesh samples
----------------------

* :ref:`ble_mesh_dfu_distributor` sample:

  * Added a force disconnect of the mesh after provisioning to ensure the apps reconnect through the proxy service.
    This is a workaround for apps that do not properly close the PB-GATT connection after provisioning, especially after DFU.
    Disable :kconfig:option:`CONFIG_BT_MESH_DK_PROV_PB_GATT_DISCONNECT` to restore old behavior.

* :ref:`bluetooth_mesh_light_lc` sample with :file:`overlay-dfu.conf` enabled:

  * Added a force disconnect of the mesh after provisioning to ensure the apps reconnect through the proxy service.
    This is a workaround for apps that do not properly close the PB-GATT connection after provisioning, especially after DFU.
    Disable :kconfig:option:`CONFIG_BT_MESH_DK_PROV_PB_GATT_DISCONNECT` to restore old behavior.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Updated the approach to build the sample with EMDS support.
    There is no ``overlay-emds.conf`` anymore.
    EMDS support is enabled as a suffixed configuration.

  * Fixed an issue where stale RPL data could persist in EMDS after a node reset.
    The sample now uses the new :c:func:`bt_mesh_dk_prov_node_reset_cb_set` function to clear EMDS data when a node reset occurs, ensuring that stale RPL data is removed.

Bluetooth Fast Pair samples
---------------------------

* Added experimental support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets in all Bluetooth Fast Pair samples.

* Updated:

  * The Fast Pair samples to use the new :file:`<bluetooth/fast_pair/...>` include paths.
  * The Fast Pair samples to use devicetree (DTS) for defining NVM partitions instead of the deprecated :ref:`partition_manager`.
    The Partition Manager configuration is removed from the samples except for a few selected board targets that are used to validate the correctness of the deprecated Partition Manager solution.

    The DTS migration has not been completed for the nRF53 Series DFU configuration with MCUboot in the overwrite mode (for example, the ``nrf5340dk/nrf5340/cpuapp`` board target in the :ref:`fast_pair_locator_tag` sample).
    This particular configuration is not yet deprecated, as the DTS alternative is not yet available.

* :ref:`fast_pair_input_device` sample:

  * Removed support for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target.
    This board target is only available with the deprecated Partition Manager solution as a legacy build configuration (``SB_CONFIG_PARTITION_MANAGER=y``).
  * Updated the documentation with a note about Android 16 automatic reconnection behavior for bonded HID devices.
    On Android 16 and later, disconnecting a bonded HID over GATT Profile (HOGP) device might result in an immediate reconnection.

* :ref:`fast_pair_locator_tag` sample:

  * Added:

    * An experimental application ranging module (:ref:`CONFIG_APP_RANGING <CONFIG_APP_RANGING>`) that uses the :ref:`ug_bt_fast_pair_fhn_pf` feature of the :ref:`bt_fast_pair_readme` library with Bluetooth Low Energy Channel Sounding as the ranging technology.
      The module is enabled by default on board targets with Channel Sounding hardware support.
      The experimental status is inherited due to its experimental dependencies.

      As of the beginning of the second quarter of 2026, Precision Finding support using Bluetooth Low Energy Channel Sounding as the ranging technology is not publicly available on Android platforms.
      However, you can experiment with this configuration on Android test devices that provide hardware and software support for Bluetooth Low Energy Channel Sounding (for example, Google Pixel 10) if you satisfy the necessary requirements.
      For details regarding the requirements, see the note in the :ref:`ug_bt_fast_pair_fhn_pf` documentation section.

    * Support for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
      The ``nrf54l15tag/nrf54l15/cpuapp`` board target is a complete prototyping platform in a compact form factor.
      It features the nRF54L15 SoC with additional sensors, RGB LED, button and a battery.
      The sample on this board target can closly represent a final product based on the nRF54L15 SoC.

  * Updated:

    * The sample to use the new name for the Find Hub Network (FHN) that was previously known as the Find My Device Network (FMDN).
      Migrated the sample to use the new Kconfig options and the new FHN API header.
    * The motion detector sensor on Thingy:53 target from gyroscope to accelerometer.
    * The location of the Kconfig fragment files from the :file:`src` subdirectory to the sample root directory.
    * The battery level measurement module by switching to the :ref:`fuel_gauge_api` API, allowing for more accurate battery level readings and better support for different battery types.
      For more information, see the :ref:`migration guide for Fast Pair Locator Tag sample <migration_3.3_fp_locator_tag>`.
    * The behaviour of the RGB LED on the ``thingy53/nrf5340/cpuapp`` target.
      The green LED color no longer indicates running status.
      Instead, the green flashes of the RGB LED are used to indicate the following:

      * The button actions.
      * The ringing state.

Cellular samples
----------------

* :ref:`nrf_cloud_mqtt_fota` and :ref:`nrf_cloud_mqtt_device_message` samples:

  * Added support for JWT authentication by enabling the :kconfig:option:`CONFIG_MODEM_JWT` Kconfig option.
    Enabling this option in the :file:`prj.conf` is necessary for using UUID as the device ID.

* :ref:`location_sample`:

  * Added support for onboarding with `nRF Cloud Utils`_ by using AT commands to set up the modem and device credentials.

* :ref:`modem_shell_application` sample:

  * Added support for JWT authentication by enabling the :kconfig:option:`CONFIG_MODEM_JWT` Kconfig option.
    Enabling this option is necessary for using nRF Cloud Utils as an onboarding method.
  * Removed JITP from the shell commands and references from the sample documentation.

* :ref:`nrf_cloud_mqtt_device_message` and :ref:`nrf_cloud_multi_service` samples:

  * Updated shadow handling by removing the shadow type ``Accepted`` and added ``Transform`` request event handling.
    Delta events can now handle error cases using the ``/shadow/update/delta/trim/err`` and ``/shadow/update/delta/full/err`` topics in nRF Cloud.

Cryptography samples
--------------------

* Added the :ref:`crypto_aes_kw` sample.

* :ref:`crypto_aes_ccm` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

Debug samples
-------------

|no_changes_yet_note|

DFU samples
-----------

* :ref:`nrf_compression_mcuboot_compressed_update`:

  * Added support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.

DECT NR+ samples
----------------

* Added the :ref:`hello_dect` sample for demonstrating the use of the DECT NR+ stack with connection manager and IPv6 connectivity.
* Added the :ref:`dect_shell_application` sample with the utilities for testing the DECT NR+ networking stack and modem features.

* :ref:`dect_phy_shell_application` sample:

  * Updated:

    * The ``dect rf_tool`` command - Major updates to improve usage for RX and TX testing.
    * Scheduler - Dynamic flow control based on load tier to prevent modem out-of-memory errors.
    * Settings - Continuous Wave (CW) support and possibility to disable Synchronization Training Field (STF) on TX and RX.

DFU samples
-----------

* Updated the :ref:`single_slot_sample` sample configuration for demonstrating DFU on the :zephyr:board:`nrf54lm20dk` board target using the :ref:`fw_loader_ble_mcumgr` firmware loader.

* Added:

  * The :ref:`fw_loader_usb_mcumgr` sample, which provides a minimal configuration for firmware loading using SMP over the USB CDC ACM virtual serial port.
    The sample serves as a starting point for developing custom firmware loader applications that work with the MCUboot bootloader.
    To enable this firmware loader implementation, use the :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_USB_MCUMGR` Kconfig option.
  * Support for using the :ref:`fw_loader_usb_mcumgr` sample with the :ref:`single_slot_sample`.
    You can test this configuration on the :zephyr:board:`nrf54lm20dk` board.

Edge Impulse samples
--------------------

* Deprecated :ref:`ei_wrapper_sample` and :ref:`ei_data_forwarder_sample` samples.
  Replaced by samples in `Edge AI Add-on for nRF Connect SDK`_.

Enhanced ShockBurst samples
---------------------------

* Added:

  * The :ref:`esb_prx_ble` sample that demonstrates how to use the ESB protocol in PRX mode concurrently with the Bluetooth LE LBS service.
  * The :ref:`esb_ptx_ble` sample that demonstrates how to use the ESB protocol in PTX mode concurrently with the Bluetooth LE LBS service.
  * Support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target in all ESB samples.
  * Support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target in all ESB samples.

* Updated all ESB samples to use the :kconfig:option:`CONFIG_ESB_CLOCK_INIT` Kconfig option for automatic HF clock management, removing the manual clock initialization code from the :file:`main.c` files.

Gazell samples
--------------

|no_changes_yet_note|

|ISE| samples
-------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Updated:

  * The documentation for all Matter samples and applications to make it more consistent and easier to maintain and read.
  * Partitions mapping for the nRF7002 DK in all Matter samples.
    See the :ref:`migration guide <migration_3.3_required>` for more information.

* :ref:`matter_light_switch_sample`:

  * Removed the ``lit_icd`` snippet from the sample and enabled LIT ICD configuration by default.

* :ref:`matter_manufacturer_specific_sample`:

  * Added support for the ``NRF_MATTER_CLUSTER_INIT`` macro.

* :ref:`matter_closure_sample`:

  * Added support for the Closure Control cluster test event triggers.

* :ref:`matter_lock_sample`:

  * Added:

    * Support for the :ref:`matter_lock_sample_wifi_thread_switching` in the nRF54LM20 DK with the nRF7002-EB II shield attached.
    * Lock data storage implementation based on the ARM PSA Protected Storage API, enabled with the :kconfig:option:`CONFIG_LOCK_ACCESS_STORAGE_PROTECTED_STORAGE` Kconfig option.

* :ref:`matter_light_bulb_sample`:

  * Added support for :ref:`matter_light_bulb_aws_iot_integration` in the nRF54LM20 DK with the nRF7002-EB II shield attached.

Networking samples
------------------

|no_changes_yet_note|

NFC samples
-----------

* Updated NFC samples for the nRF54H20 SoC to reduce current consumption in idle mode.
  An empty netcore and additional configurations for power management were added to the sysbuild in the following samples:

  * :ref:`record_launch_app`
  * :ref:`record_text`
  * :ref:`nfc_shell`
  * :ref:`nfc_tnep_tag`
  * :ref:`writable_ndef_msg`

* :ref:`writable_ndef_msg` sample:

  * Fixed a power consumption issue.
    The main loop now blocks on a semaphore instead of using the :c:func:`k_cpu_atomic_idle` function, allowing the idle thread to handle power management after the read or write tag.

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

|no_changes_yet_note|

PMIC samples
------------

* Added support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target to the :ref:`npm13xx_fuel_gauge` and the :ref:`npm13xx_one_button` samples.

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

|no_changes_yet_note|

Thread samples
--------------

* Added support for the nRF54L Series DKs in the following Thread sample documents:

  * :ref:`coap_client_sample`
  * :ref:`coap_server_sample`

* Removed all application-specific snippets from the Thread samples.
  Refer to the :ref:`migration guide <migration_3.3_required>` to see the list of changes.

Wi-Fi samples
-------------

* Added support for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target variant to all Wi-Fi samples.

Other samples
-------------

* Added support for the ``nrf54lm20dk/nrf54lm20b`` board target variant in the following samples:

  * :ref:`coremark_sample`
  * :ref:`nrf_rpc_protocols_serialization_client`
  * :ref:`nrf_rpc_protocols_serialization_server`

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added the :ref:`uart_driver` documentation.

* Updated the ``nrfx_gppi`` API to now have full support on the application core of the nRF54H20 device.
  Using DPPI does not require the devicetree configuration.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Flash drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* Added the :ref:`dtm_twowire_to_hci_readme` library that converts DTM 2-wire UART commands into appropriate 2-wire UART events or HCI commands, and subsequent HCI events into 2-wire UART events.

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * :ref:`ug_bt_fast_pair_fhn_pf` support for the Find Hub Network (FHN) extension, enabling distance measurement for FHN accessories using supported ranging technologies.
      The feature is controlled by the :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_PF` Kconfig option and is experimental.
    * Support for the Fast Pair partition definition using devicetree (DTS) for all supported board targets.
      Defining the Fast Pair partition using DTS is the recommended approach and replaces the deprecated :ref:`partition_manager` approach.

  * Updated:

    * The location of the Fast Pair headers and implementation out of the :file:`services` subdirectory.
      The headers moved from :file:`include/bluetooth/services/fast_pair/` to :file:`include/bluetooth/fast_pair/` and the implementation moved from :file:`subsys/bluetooth/services/fast_pair/` to :file:`subsys/bluetooth/fast_pair/`.
      The deprecated forwarding headers remain at the old paths to provide backward compatibility.
    * The naming of the Find My Device Network (FMDN) extension to Find Hub Network (FHN) to align with the updated Google specification:

      * All public API symbols have been renamed from ``bt_fast_pair_fmdn_*`` to ``bt_fast_pair_fhn_*``.
        The new FHN header is located at :file:`include/bluetooth/fast_pair/fhn/fhn.h`.
        Deprecated FMDN API aliases remain available through the :file:`include/bluetooth/services/fast_pair/fmdn.h` header.
      * All Kconfig options have been renamed from ``CONFIG_BT_FAST_PAIR_FMDN_*`` to ``CONFIG_BT_FAST_PAIR_FHN_*``.
        Deprecated FMDN Kconfig options remain available under the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN` option tree.
      * The FMDN implementation directory has been renamed from :file:`subsys/bluetooth/services/fast_pair/fmdn/` to :file:`subsys/bluetooth/fast_pair/fhn/`.

      See the `migration guide <Migration guide for nRF Connect SDK v3.3.0_>`_ for details.

* :ref:`bt_mesh_dk_prov` module:

  * Added support for node reset callback.
    Applications can now register a callback using the :c:func:`bt_mesh_dk_prov_node_reset_cb_set` function to perform cleanup operations when a node reset occurs.

* :ref:`lib_nrf_bt_scan_readme` library:

  * Fixed a bug where the central would attempt to connect to a non-connectable peripheral if the device had been matched by specified filters.

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Gazell libraries
----------------

|no_changes_yet_note|

Security libraries
------------------

* :ref:`nrf_security` library:

  * Updated the header files at :file:`subsys/nrf_security/src/drivers/cracen/cracenpsa/include/` with Doxygen documentation.

Modem libraries
---------------

* :ref:`lte_lc_readme` library:

  * Added:

    * Support for new PDN events :c:enumerator:`LTE_LC_EVT_PDN_SUSPENDED` and :c:enumerator:`LTE_LC_EVT_PDN_RESUMED`.
    * The :kconfig:option:`CONFIG_LTE_LOCK_BAND_LIST` Kconfig option to set bands for the LTE band lock using a comma-separated list of band numbers.

  * Removed:

    * The default value for the :kconfig:option:`CONFIG_LTE_LOCK_BAND_MASK` Kconfig option.
    * The ``lte_lc_modem_events_enable()`` and ``lte_lc_modem_events_disable()`` functions.
      Instead, use the :kconfig:option:`CONFIG_LTE_LC_MODEM_EVENTS_MODULE` Kconfig option to enable modem events.

* :ref:`lib_at_shell` library:

  * Added:

    * Support for AT command mode (shell bypass) using the :kconfig:option:`CONFIG_AT_SHELL_CMD_MODE` Kconfig option.
    * Tab-completion for known shell subcommands.

* :ref:`lib_location` library:

  * Added:

    * The :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SCANNING_PARAMS_OVERRIDE` Kconfig option and related options :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SCANNING_DWELL_TIME_ACTIVE` and :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SCANNING_DWELL_TIME_PASSIVE` to configure Wi-Fi scan parameters.
    * The :c:enumerator:`LOCATION_EVT_CANCELLED` event that notifies the application when a location request has been cancelled.

  * Fixed a bug where GNSS was never stopped if the :c:func:`location_cloud_location_ext_result_set` function was called during GNSS method execution.

Multiprotocol Service Layer libraries
-------------------------------------

* Fixed:

  * An issue with toggling of the **REQUEST** and **STATUS0** pins when using the nRF700x coexistence interface on the nRF54H20 device.
  * An issue where coexistence pin GPIO polarity settings were ignored when using the nRF700x coexistence interface.

Libraries for networking
------------------------

* The FTP client library has been moved to the Zephyr repository (see :ref:`zephyr:ftp_client_interface`).

* :ref:`lib_nrf_cloud` library:

  * Added:

    * The :c:func:`nrf_cloud_coap_shadow_network_info_update` function.
    * The :c:func:`nrf_cloud_coap_shadow_configured_info_update` function to update the configured sections in the device shadow over CoAP.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SHADOW_INFO_ON_CONNECT` Kconfig option to control whether the device shadow information is sent when the device connects to nRF Cloud.
    * Handling of the ``/shadow/update/delta/trim/err`` topic.
      Errors will be logged to the application if the delta shadow is larger than 1792 bytes.

  * Removed:

    * The ``NRF_CLOUD_MQTT_SHADOW_TRANSFORMS`` Kconfig option.
      Transform request are the default method to request AWS shadows, replacing the shadow type ``Accepted``.
    * The topics ``/shadow/get/accepted/trim``, ``/shadow/get/accepted``, and ``/shadow/get/trim``.
      Requesting shadow updates through them potentially caused the device to disconnect due to a shadow update larger than two KB.
    * The ``/shadow/update/delta/full`` topic.
      It is replaced by ``/shadow/update/delta/trim``.
      Deltas are now trimmed by default to prioritize smaller shadows.

  * Fixed an issue where CoAP FOTA was affected due to the downloader reading ``proxy_uri`` as a C string using ``strlen()`` to determine the CoAP option length.
    It picked up the stale tail, producing a corrupted download URL.

* :ref:`lib_nrf_cloud_pgps` library:

  * Updated the range for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` and :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` Kconfig options to values supported by nRF Cloud.
  * Fixed an issue where preemptive updates were not always performed when expected.
  * Removed the ``CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD`` Kconfig choice and related options (``CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD_120_MIN`` and ``CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD_240_MIN``).

Libraries for NFC
-----------------

* :ref:`lib_nfc_tnep` library:

  * Fixed the ``BUILD_ASSERT`` for the waiting time extension count (``_n_wait``) in the TNEP Tag API.
    It now uses ``NFC_TNEP_TAG_MAX_N_WAIT_TIME`` instead of ``NFC_TNEP_TAG_MAX_WAIT_TIME``, aligning the check with the TNEP 1.0 specification.

  * Removed the unused ``NFC_TNEP_NDEF_NLEN_SIZE`` define from the TNEP base API.

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added the :ref:`lib_accel_to_angle` library for converting three-dimensional acceleration into pitch and roll angles.

* Deprecated the :ref:`ei_wrapper` library.
  Replaced by Edge Impulse SDK in `Edge AI Add-on for nRF Connect SDK`_.

* :ref:`lib_hw_id` library:

  * Updated by renaming the ``CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC`` Kconfig option to :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_BT_DEVICE_ADDRESS`.

* :ref:`emds_readme` library:

  * Removed strict dependency on the Partition Manager.
    The Partition Manager can still be used with the library, but it is deprecated and not recommended for new designs.

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* Added:

  * The :ref:`matter_sample_checker` script to check the consistency of Matter samples in the |NCS|.
  * The :ref:`bt_nus_shell_script` that forwards data between TCP clients and a Bluetooth LE device using the Nordic UART Service.

* :ref:`west_sbom` script:

  * Added sysbuild support for generating individual SBOM for each application.
  * Updated to support Matter builds by detecting GN external projects and collecting their source files for SBOM generation.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Added documentation for the :ref:`ug_bt_fast_pair_fhn_pf` to the :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide, covering the feature overview with supported ranging technologies, prerequisite operations, GATT service interaction with the ranging management workflow, and the Bluetooth Low Energy Channel Sounding helper APIs.

* Updated:

  * The :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide to reflect the Find My Device Network (FMDN) extension rename to Find Hub Network (FHN), aligning with the updated Google specification.
  * The :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide to recommend devicetree (DTS) as the primary partition definition method and to deprecate the :ref:`partition_manager` approach.

    The only exception from these migration recommendations in the context of the Fast Pair sample support is the nRF53 Series DFU configuration with MCUboot in the overwrite mode (for example, the ``nrf5340dk/nrf5340/cpuapp`` board target in the :ref:`fast_pair_locator_tag` sample).
    This particular configuration is not yet deprecated, as the DTS alternative is not yet available.

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

* Added:

  * The :kconfig:option:`CONFIG_MEMFAULT_NCS_POST_INITIAL_HEARTBEAT_ON_NETWORK_CONNECTED` Kconfig option to control whether an initial heartbeat is sent when the device connects to a network
    This shows the device status and initial metrics in the Memfault dashboard soon after boot.
  * Support for recording location metrics when using external cloud location services (:kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL`).
  * Updated Memfault to version 1.37.1.
    See the `Memfault firmware SDK changelog`_ for details.

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

* Updated by enabling a transform request for topic prefix and pairing during connection initialization to nRF Cloud in the MQTT finite state machine (FSM).
* Fixed a hang in the nRF Cloud log backend caused by incorrect error handling.
  When the semaphore cannot be acquired, the function now returns the original size instead of 0, allowing the logging system to proceed correctly.

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``93b5f19f994b9a9376985299c1427a1630f6950e``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 93b5f19f99 ^911b3da139

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^93b5f19f99

The current |NCS| main branch is based on revision ``93b5f19f99`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added a section in :ref:`ug_nrf54h20_pm_optimization` about optimizing power on the nRF54H20 SoC by relocating the radio core firmware to TCM.
* Updated the :ref:`emds_readme` library documentation to use static device tree partitions instead of the Partition Manager.
* Removed references to JITP in different areas of the documentation.
