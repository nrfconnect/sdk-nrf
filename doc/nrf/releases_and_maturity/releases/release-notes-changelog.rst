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

|no_changes_yet_note|

Board support
=============

|no_changes_yet_note|

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

* Updated:

  * Moved the MCUboot SMP Server sample to :file:`samples/dfu/smp_svr`.
    Added documentation for the sample.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

|no_changes_yet_note|

Developing with nRF54H Series
=============================

* Added:

  * A document describing the merged slot update strategy for nRF54H20 devices, allowing simultaneous updates of both application cores (APP and RAD) in a single update operation.
    For more information, see :ref:`ug_nrf54h20_partitioning_merged`.
  * A document describing the manifest-based update strategy for nRF54H20 devices.
    For more information, see :ref:`ug_nrf54h20_mcuboot_manifest`.
  * The :ref:`ug_nrf54h20_mcuboot_requests` page describes bootloader requests for the nRF54H20 SoC.
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
  * Support for the secp384r1 key storage in the :ref:`Key Management Unit (KMU) <ug_nrf54l_crypto_kmu_supported_key_types>`.
  * Support for AES-GCM AEAD using CRACEN for the :ref:`nrf54lm20dk <app_boards>` board.
  * Support for ChaCha20-Poly1305 AEAD using CRACEN for the :ref:`nrf54lm20dk <app_boards>` board.
  * Support for AES Key Wrap (AES-KW) and AES Key Wrap with Padding (AES-KWP) algorithms in the :ref:`nrf_oberon <crypto_drivers_oberon>` and :ref:`CRACEN <crypto_drivers_cracen>` drivers.
    The :ref:`Supported cryptographic operations in the nRF Connect SDK <ug_crypto_supported_features_key_wrap_algorithms>` page has been updated accordingly.

* Updated:

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

* Removed:

  * The ``CONFIG_PSA_WANT_KEY_TYPE_WPA3_SAE_PT`` Kconfig option and replaced it with :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_WPA3_SAE`.
  * The ``CONFIG_PSA_WANT_ALG_WPA3_SAE`` Kconfig option and replaced it by options :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_FIXED` and :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_GDH`.

Trusted Firmware-M (TF-M)
-------------------------

* Updated to version 2.2.2.
* The default TF-M profile is now :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` for all configurations except for Thingy:91 and Thingy:91 X, on which it is still :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL`.

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

* Fixed invalid radio configuration for legacy ESB protocol.

Gazell
------

|no_changes_yet_note|

Matter
------

* Updated the :ref:`matter_test_event_triggers_default_test_event_triggers` section with the new Closure Control cluster test event triggers.
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

Wi-Fi®
------

|no_changes_yet_note|

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

* Updated:

  * Switched to the new USB stack introduced in Zephyr 3.4.0.
    For an end user, this change requires no action.
    macOS will now work out of the box, fixing OCT-2154.

  * Programming script.
    Devices are now halted before programming.
    Furthermore, the devices are kept halted until they are all programmed, and then started together
    with the headsets starting first.
    This eases sniffing of advertisement packets.

  * With the latest release of |nRFVSC|, you can build and program the nRF5340 Audio application using the |nRFVSC| GUI.
    Updated the :ref:`nrf53_audio_app_building` accordingly: the note about missing support in |nRFVSC| has been removed and the section about programming using standard methods now lists the steps for |nRFVSC| and the command line.

  * Improved handling of I2S RX buffer overruns.
    When an overrun occurs, the most recent block in the current frame is removed to make space for new incoming data.

  * Optimized USB-to-encoder audio processing pipeline to reduce CPU usage.
    Note that LC3-encoding of sinusoidal input demands more of the CPU than real-world audio input.

  * The audio data path now accumulates 10 ms frames instead of processing 1 ms blocks individually, reducing message queue operations by 90% and improving overall system performance.
    This optimization affects both USB audio input processing and the I2S audio datapath, resulting in more efficient encoder thread operation.

  * Improved error handling with ``unlikely()`` macros for better branch prediction in performance-critical paths.

  * Removed the Bluetooth controller watchdog from the application.
    The watchdog was not providing value and the removal allows for easier porting to other platforms that do not have a multi-core architecture.

  * Separated the audio clock configuration into a dedicated module.
    This allows for better organization and potential reuse of the audio clock configuration code between different SoCs that might not have the high-frequency audio clock (HFCLKAUDIO) feature.
    The new module provides an initialization function for setting up the audio clock and a function for configuring the audio clock frequency.

nRF Desktop
-----------

* Added:

  * A workaround for the USB next stack race issue where the application could try to submit HID reports while the USB is being disabled after USB cable has been unplugged, which results in an error.
    The workaround is applied when the :option:`CONFIG_DESKTOP_USB_STACK_NEXT_DISABLE_ON_VBUS_REMOVAL` Kconfig option is enabled.
  * Support for the ``nrf54lv10dk/nrf54lv10a/cpuapp`` board target.
  * Application-specific Kconfig options that simplify using SEGGER J-Link RTT (:option:`CONFIG_DESKTOP_LOG_RTT`) or UART (:option:`CONFIG_DESKTOP_LOG_UART`) as logging backend used by the application (:option:`CONFIG_DESKTOP_LOG`).
    The :kconfig:option:`CONFIG_LOG_BACKEND_UART` and :kconfig:option:`CONFIG_LOG_BACKEND_RTT` Kconfig options are no longer enabled by default if nRF Desktop logging (:option:`CONFIG_DESKTOP_LOG`) is enabled.
    These options are controlled through the newly introduced nRF Desktop application-specific Kconfig options.
    The application still uses SEGGER J-Link RTT as the default logging backend.
  * Support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.

* Updated:

  * The :option:`CONFIG_DESKTOP_BT` Kconfig option to no longer select the deprecated :kconfig:option:`CONFIG_BT_SIGNING` Kconfig option.
    The application relies on Bluetooth LE security mode 1 and security level of at least 2 to ensure data confidentiality through encryption.
  * The memory map for RAM load configurations of nRF54LM20 target to increase KMU RAM section size to allow for secp384r1 key.
  * The default log levels used by the legacy USB stack (:option:`CONFIG_DESKTOP_USB_STACK_LEGACY`) to enable error logs (:kconfig:option:`CONFIG_USB_DEVICE_LOG_LEVEL_ERR`, :kconfig:option:`CONFIG_USB_DRIVER_LOG_LEVEL_ERR`).
    Previously, the legacy USB stack logs were turned off.
    This change ensures visibility of runtime issues.
  * Application configurations that emit debug logs over UART to use the :option:`CONFIG_DESKTOP_LOG_UART` Kconfig option instead of explicitly configuring the logger.
    This is done to simplify the configurations.

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

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* :ref:`peripheral_ams_client` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board targets.

* :ref:`peripheral_ancs_client` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board targets.

* :ref:`peripheral_hids_keyboard` sample:

  * Added support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.

* :ref:`peripheral_hids_mouse` sample:

  * Added support for the ``nrf54ls05dk/nrf54ls05b/cpuapp`` board target.

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

  * Fixed an issue where stale RPL data could persist in EMDS after a node reset.
    The sample now uses the new :c:func:`bt_mesh_dk_prov_node_reset_cb_set` function to clear EMDS data when a node reset occurs, ensuring that stale RPL data is removed.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated the motion detector sensor on Thingy:53 target from gyroscope to accelerometer.

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

Cryptography samples
--------------------

* :ref:`crypto_aes_ccm` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

* Added the :ref:`hello_dect` sample for demonstrating the use of the DECT NR+ stack with connection manager and IPv6 connectivity.
* Added the :ref:`dect_shell_application` sample with the utilities for testing the DECT NR+ networking stack and modem features.

* :ref:`dect_phy_shell_application` sample:

  * Updated:

      * The ``dect rf_tool`` command - Major updates to improve usage for RX and TX testing.
      * Scheduler - Dynamic flow control based on load tier to prevent modem out-of-memory errors.
      * Settings - Continuous Wave (CW) support and possibility to disable Synchronization Training Field (STF) on TX and RX.

Edge Impulse samples
--------------------

* Deprecated :ref:`ei_wrapper_sample` and :ref:`ei_data_forwarder_sample` samples.
  Replaced by samples in `Edge AI Add-on for nRF Connect SDK`_.

Enhanced ShockBurst samples
---------------------------

|no_changes_yet_note|

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

|no_changes_yet_note|

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added the :ref:`uart_driver` documentation.

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

:ref:`bt_mesh_dk_prov` module:

  * Added support for node reset callback.
    Applications can now register a callback using the :c:func:`bt_mesh_dk_prov_node_reset_cb_set` function to perform cleanup operations when a node reset occurs.

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

* :ref:`lib_nrf_cloud` library:

  * Added the :c:func:`nrf_cloud_coap_shadow_network_info_update` function to update the network information section in the device shadow over CoAP.

* :ref:`lib_nrf_cloud_pgps` library:

  * Updated the range for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` and :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` Kconfig options to values supported by nRF Cloud.

  * Fixed an issue where preemptive updates were not always performed when expected.

  * Removed the ``CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD`` Kconfig choice and related options (``CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD_120_MIN`` and ``CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD_240_MIN``).

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added the :ref:`lib_accel_to_angle` library for converting three-dimensional acceleration into pitch and roll angles.

* :ref:`lib_hw_id` library:

  * Updated by renaming the ``CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC`` Kconfig option to :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_BT_DEVICE_ADDRESS`.

* Deprecated :ref:`ei_wrapper` library.
  Replaced by Edge Impulse SDK in `Edge AI Add-on for nRF Connect SDK`_.

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

* Added the :ref:`matter_sample_checker` script to check the consistency of Matter samples in the |NCS|.

* :ref:`west_sbom` script:

  * Added sysbuild support for generating individual SBOM for each application.
  * Updated to support Matter builds by detecting GN external projects and collecting their source files for SBOM generation.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

|no_changes_yet_note|

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

* Added:

  * The :kconfig:option:`CONFIG_MEMFAULT_NCS_POST_INITIAL_HEARTBEAT_ON_NETWORK_CONNECTED` Kconfig option to control whether an initial heartbeat is sent when the device connects to a network
    This shows the device status and initial metrics in the Memfault dashboard soon after boot.
  * Support for recording location metrics when using external cloud location services (:kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL`).

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
* Removed references to JITP in different areas of the documentation.
* Updated the :ref:`emds_readme` library documentation to use static device tree partitions instead of the Partition Manager.
