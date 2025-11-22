.. _ncs_release_notes_3.1.0:

|NCS| v3.1.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

Added the following features as supported:

* nRF54L Series:

  * Find My Network (FMN) and Find My Device Network (FMDN) support for nRF54L05 and nRF54L10 updated from :ref:`experimental <software_maturity>`, including dual network samples.

    .. include:: /includes/fast_pair_fmdn_rename.txt

  * Bootloader and Device Firmware Update (DFU):

    * MCUboot as the first-stage immutable bootloader using hardware cryptographic acceleration on nRF54L10 and nRF54L05.
    * MCUboot sample optimized for NVM footprint and implementing recommended settings when working with the nRF54L15 DK.
      The :ref:`mcuboot_minimal_configuration` sample utilizes hardware cryptography for ED25519 signature verification and KMU for secure key storage, achieving an NVM footprint of ~16.5 KB.
    * MCUboot support for hardware downgrade protection.
    * MCUboot support for SHA512 signature verification.

* Matter:

  * Integration of Matter 1.4.2.

* PMIC:

  * nPM1304 and nPM1304 EK:

    * Added support for nPM1304, which is a PMIC with battery charging, precision fuel gauging and advanced system management features for small-size batteries.
      It integrates in a small form factor two highly efficient buck regulators, two dual-purpose LDO/load switches, an input regulator with USB support, 5 GPIO, 3 LED drivers, an ADC, and other features.

* nRF Desktop:

  * Possibility to add custom HID reports.

Added the following features as experimental:

* nRF54L Series:

  * Bootloader and Device Firmware Update (DFU):

    * Added MCUboot support for AES-256 image encryption.

* nRF54H20:

  * New Secure Domain FW: :ref:`ug_nrf54h20_ironside`.
    IronSide SE will simplify development and usage of nRF54H20 as well as reduce the memory footprint.
  * MCUboot support.

Improved:

* nRF54L Series:

  * Code size optimization for the Bluetooth® LE SoftDevice Controller subsystem for :ref:`peripheral-only <nrfxlib:softdevice_controller>` applications not using advanced features.
    This reduces the usage of NVM and RAM depending on the Bluetooth LE functionality being used.
    As a reference, on the :ref:`peripheral_lbs`, it reduced NVM and RAM by up to 15.5 KB and 1.7 KB, respectively.

* Matter:

  * Matter-over-Thread applications now use a new OpenThread architecture, bypassing the Zephyr Networking layer.
    This change significantly reduces memory usage in Matter applications.
    On the nRF54L15 DK, the new architecture saves approximately 15 KB of RAM and 40 KB of NVM.
  * Introduced the `Matter Cluster Editor app`_ as part of nRF Connect for Desktop.
    This tool allows users to create and edit manufacturer-specific clusters and extend the Matter data model.

* Thread:

  * Added a new architecture for OpenThread in Zephyr, enabling bypass of the Zephyr Networking layer.
    This enhances performance and reduces memory consumption across Thread-based applications.

Removed:

* nRF54H20:

  * SUIT Secure Domain FW and boot solution.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.1.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.1.0`_.

Integration test results
************************

The integration test results for this tag can be found in the following external artifactory:

* `Twister test report for nRF Connect SDK v3.1.0`_
* `Hardware test report for nRF Connect SDK v3.1.0`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.1.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9151 SoC`_
* `Modem firmware compatibility matrix for the nRF9160 SoC`_

Use the latest version of the `Programmer app`_ of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v3.1.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.1.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.1.0`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v3.1.0`_ for the changes required or recommended when migrating your application from |NCS| v3.0.0 to |NCS| v3.1.0.

.. _ncs_release_notes_310_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated the required `SEGGER J-Link`_ version to v8.42.
* Removed the separate requirement for installation of the `nRF Util's device command <Device command overview_>`_ from the :ref:`install_ncs` page under :ref:`installing_vsc`.
  The tool and the command are now included in the |NCS| toolchain bundle.

  .. note::

     If you plan to work with command line, you still need to download nRF Util and install the ``sdk-manager`` command in order to get the toolchain bundle.

Board support
=============

* Added:

  * Bias-pull-up for Thingy:91 X nRF9151 UART RX pins.
  * Alternative partition tables for Thingy:91 X.

Bootloaders and DFU
===================

* Added:

  * MCUboot support as first-stage immutable bootloader using hardware cryptographic acceleration on nRF54L10 and nRF54L05.
  * MCUboot support for hardware downgrade protection on the nRF54L Series devices.
  * MCUboot support for SHA512 signature verification on the nRF54L Series devices.
  * Linking Time Optimization (LTO) support for MCUboot on the nRF54L Series devices.
  * Experimental MCUboot support for AES-256 image encryption for the nRF54L Series devices.
  * Experimental MCUboot support as first-stage immutable bootloader for the nRF54H20:

    * Direct XIP mode with merged images for application and radio cores.
    * Swap mode with separated images for application and radio cores.

  * MCUboot sample optimized for NVM footprint and implementing recommended settings when working with the nRF54L15 DK.
    The :ref:`mcuboot_minimal_configuration` sample utilizes hardware cryptography for ED25519 signature verification and KMU for secure key storage, achieving an NVM footprint of ~16.5 KB.
  * Support to provision KMU on nRF54L115, nRF54L10, and nRF54L05 devices.

* Updated the following security features:

  * HW downgrade prevention.
  * RAM memory cleanup controlled by the ``CONFIG_MCUBOOT_CLEANUP_RAM`` Kconfig option.
  * ECIES-X25519 encryption improvements.

* Fixed several stability and functionality improvements.
* Removed SUIT support.

Developing with nRF54L Series
=============================

* Added:

  * The :ref:`ug_nrf54l_dfu_config` documentation page, describing how to configure DFU and secure boot settings using MCUboot and NSIB.
  * The :ref:`ug_nrf54l_ecies_x25519` page on enabling the AES encryption with ECIES-X25519, which is used for secure encryption key exchange.

* Updated by increasing the default value of the :kconfig:option:`CONFIG_MPSL_HFCLK_LATENCY` Kconfig option to support slower crystals.
  See the Kconfig description for a detailed description on how to select the correct value for a given application.

Developing with nRF54H Series
=============================

* Added:

  * The :ref:`ug_nrf54h20_pm_optimization` page that describes the power management optimizations for the nRF54H20 SoC.
  * The :ref:`ug_nrf54h20_ppr` page that describes how to work with the PPR core of the nRF54H20 SoC.
  * The :ref:`ug_nrf54h20_mcuboot_dfu` and :ref:`ug_nrf54h_ecies_x25519` pages that describe how to configure DFU and MCUboot on the nRF54H20 SoC.
  * The :ref:`ug_nrf54h20_ironside` page for the nRF54H20 SoC.
  * The :ref:`migration_3.1_54h_suit_ironside` page that describes how to migrate from SUIT to IronSide SE for the nRF54H20 SoC.
  * The :ref:`migration_bicr_nrf54h` page that describes how to migrate the nRF54H20 SoC BICR configuration from DTS to JSON.

* Updated:

  * The :ref:`ug_nrf54h20_architecture_pm` page to add a mapping of application states to software power states on the nRF54H20 SoC.
  * The :ref:`nRF54H20 <ug_nrf54h>` documentation with the |NCS| v3.1.0 changes.

* Removed SUIT support and the related documentation.

Developing with Thingy:91 X
===========================

* Added the :ref:`thingy91x_recover_to_factory_firmware` documentation page.

Developing with Front-End Modules
=================================

* Added:

  * Support for the nRF21540 Front-End Module in GPIO/SPI mode for nRF54L Series devices.
  * Support for the Simple GPIO Front-End Module for nRF54L Series devices.

Developing with coprocessors
============================

* Added the :ref:`ug_hpf_softperipherals_comparison` documentation page, describing potential use cases and differences between the two solutions.

Security
========

* Added the new section about :ref:`ug_crypto_index`.
  The new section includes pages about :ref:`ug_crypto_architecture` (new page), :ref:`crypto_drivers` and :ref:`psa_crypto_support` (both moved from the :ref:`nrf_security` library documentation).

* Updated:

  * Mbed TLS to version 3.6.4.
  * The :ref:`software_maturity_security_features` section on the :ref:`software_maturity` with updated hardware support status for Trusted Firmware-M and PSA Crypto.
  * The :ref:`ug_tfm_logging` page with more details about how to configure logging on the same UART instance as the application for nRF5340 and nRF91 Series devices.
  * The :ref:`crypto_drivers` page with more details about the driver selection process.
  * The :ref:`ug_crypto_supported_features` page with updated values for the supported cryptographic operations and algorithms.
    Now, the page only lists features and algorithms that are supported by at least one driver.
  * The Oberon PSA Crypto to version 1.5.1 that introduces support for the following new features with the Oberon PSA driver:

    * Experimental support for post-quantum cryptography schemes ML-KEM (FIPS 203), ML-DSA (FIPS 204), and NIST SP 800-208.
    * Experimental support for XChaCha20-Poly1305 AEAD algorithm.
    * Experimental support for SHAKE128 with 256-bit output length.

   The :ref:`ug_crypto_supported_features` page has been updated accordingly.

* Removed the ``CONFIG_PSA_WANT_ALG_WPA3_SAE_PT`` Kconfig option and replaced it with :kconfig:option:`CONFIG_PSA_WANT_ALG_WPA3_SAE_H2E`.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

* Added the :kconfig:option:`CONFIG_BT_CTLR_CHANNEL_SOUNDING_TEST` Kconfig option.
  This option reduces the NVM usage of Channel Sounding when disabled by removing the ``LE CS Test`` and ``LE CS Test End`` HCI commands.

Bluetooth Mesh
--------------

* Updated:

  * The default value of the :kconfig:option:`CONFIG_BT_MESH_DFU_FWID_MAXLEN` Kconfig option to be compliant with the Bluetooth LE Mesh specification.
  * The default value of the :kconfig:option:`CONFIG_BT_MESH_ADV_STACK_SIZE` Kconfig option for the nRF54L-based board targets.
  * The default value of the :kconfig:option:`CONFIG_MBEDTLS_HEAP_SIZE` Kconfig option if the DFU Distributor role is enabled (through the :kconfig:option:`CONFIG_BT_MESH_DFD_SRV` Kconfig option) to accommodate the minimum number of keys needed for the role.
  * The :ref:`bt_mesh_le_pair_resp_readme` vendor model, used in the :ref:`ble_mesh_dfu_distributor` sample, to clear Bluetooth bonding information once a node-reset message is received.

* Fixed:

  * A bug in automated metadata generation using the :kconfig:option:`SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA` Kconfig option to correctly extract vendor model metadata.
  * A buffer leakage where allocated advertisements will not be cleared after the device was suspended.
  * A bug where the Bluetooth LE scanner will be stopped when an LPN is disabled.
    This is only for the case when the :kconfig:option:`CONFIG_BT_MESH_LPN_ESTABLISHMENT` Kconfig option is not enabled (as this option utilizes a reduced scan cycle).
  * A bug in the :ref:`zephyr:bluetooth_mesh_blob_srv` causing the LPN queue to overflow.
    Now, if friendship is established, the maximum possible chunk size and requested number of chunks are adapted according to the current LPN queue size.


Enhanced ShockBurst (ESB)
-------------------------

* Updated by improving protocol disable handling in the :c:func:`esb_disable` and :c:func:`esb_stop_rx` functions.

Matter
------

* Added:

  * :ref:`FastTrack Recertification <ug_matter_device_recertification_process>` and :ref:`Portfolio Certification <ug_matter_device_portfolio_certification>` programs.
  * The :ref:`ug_matter_networking_selection` section on the :ref:`ug_matter_device_advanced_kconfigs` page.
    The section describes how to select the networking layer for Matter applications.

* Updated:

  * The ``west zap-generate`` command to remove previously generated ZAP files before generating new files.
    To skip removing the files, use the ``--keep-previous`` argument.
  * The :ref:`ug_matter_creating_custom_cluster` user guide by adding information about implementing custom commands.
  * Matter-over-Thread apps can now use the OpenThread API directly, instead of using intermediate Zephyr L2 layer.
    This change significantly reduces memory usage in Matter applications.
    On the :zephyr:board:`nrf54l15dk`, it saves approximately 15 kB of RAM and 40 kB of flash.
    To learn more about the new architecture option, see the :ref:`ug_matter_networking_selection` user guide.

* Removed:

  * The nRF Connect Matter Manufacturer Cluster Editor tool page.
    The tool is now available in the `nRF Connect for Desktop`_ app as the Matter Cluster Editor app.
    For installation instructions and more information about the tool, see the `Matter Cluster Editor app`_ documentation.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.4.2.0`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Updated:

  * Improved robustness in group session management.
  * Optimized the device commissioning process.

* Fixed incorrect memory releases and unhandled exceptions.

nRF IEEE 802.15.4 radio driver
------------------------------

* Added the Kconfig options to configure default CSMA-CA algorithm parameters (:kconfig:option:`CONFIG_NRF_802154_CSMA_CA_MIN_BE_DEFAULT`, :kconfig:option:`CONFIG_NRF_802154_CSMA_CA_MAX_BE_DEFAULT`, :kconfig:option:`CONFIG_NRF_802154_CSMA_CA_MAX_CSMA_BACKOFFS_DEFAULT`).
* Updated by replacing the Kconfig option ``CONFIG_NRF_802154_CCA_ED_THRESHOLD`` with :kconfig:option:`CONFIG_NRF_802154_CCA_ED_THRESHOLD_DBM` to ensure consistent behavior on different SoC families and to reduce the likelihood of misconfiguration.

Thread
------

* Added the new architecture option to use the OpenThread stack directly to communicate with the IEEE 802.15.4 radio driver.
  See the :ref:`openthread_stack_architecture` user guide for more information.
  The new architecture option reduces the memory footprint of the OpenThread stack by around 4% and the RAM usage by around 12% in the :ref:`ot_cli_sample` sample.

Wi-Fi®
------

* Added:

  * Support for EAP-PEAP and EAP-TTLS authentication methods to enterprise security in the Wi-Fi management API.
  * Support for Wi-Fi Alliance's Wi-Fi Power saving features (Proxy ARP and BSS Max Idle period).
  * Support for storing Wi-Fi credentials in NVM for the Enterprise mode.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

* Fixed to resume Bluetooth connectable advertising after a disconnect.

Matter bridge
-------------

* Added the implementation of the missing identify cluster for the endpoint 1.
  This resolves the :ref:`known issue <known_issues>` KRKNWK-20019.

nRF5340 Audio
-------------

* Added:

  * Experimental support for Audio on the nRF5340 DK, with LED state indications and button controls.
  * Experimental Support for stereo in :ref:`broadcast sink app<nrf53_audio_broadcast_sink_app>`.
    The broadcast sink can now receive audio from two BISes and play it on the left and right channels of the audio output, if the correct configuration options are enabled.
    The I2S output will be stereo, but :zephyr:board:`nrf5340_audio_dk` will still only have one audio output channel, because it has a mono codec (CS47L63).
    RTT shell commands have been added to select if the HW codec plays back I2S channel 0/left, 1/right or 0 and 1 mixed (default).
    See :file:`overlay-broadcast_sink.conf` for more information.

* Updated:

  * The application to use the ``NFC.TAGHEADER0`` value from FICR as the broadcast ID instead of using a random ID.
  * The application to change from Newlib to Picolib to align with |NCS| and Zephyr.
  * The application to use the :ref:`net_buf_interface` API to pass audio data between threads.
    The :ref:`net_buf_interface` will also contain the metadata about the audio stream in the ``user_data`` section of the API.
    This change was done to transition to standard Zephyr APIs, as well as to have a structured way to pass N-channel audio between modules.
  * The optional buildprog tool to use `nRF Util`_ instead of nrfjprog that has been deprecated.
  * The documentation pages with information about the :ref:`SD card playback module <nrf53_audio_app_overview_architecture_sd_card_playback>` and :ref:`how to enable it <nrf53_audio_app_configuration_sd_card_playback>`.
  * The buffer count (:kconfig:option:`CONFIG_BT_ISO_TX_BUF_COUNT` and :kconfig:option:`CONFIG_BT_BUF_ACL_TX_COUNT`) to be in-line with SoftDevice Controller (SDC) defaults.
    This can be changed and optimized for specific use cases.
  * The audio devices are now set up with a location bitfield according to the BT Audio specification, instead of a channel.
    Since a device can have multiple locations set, the location name has been removed from the device name during DFU.
  * For unicast: If ``CONFIG_STREAM_BIDIRECTIONAL=y``, audio is returned from both left and right devices, and only the first connected device is decoded due to CPU limitation as before.
    Now, the returned audio is copied after being decoded, so returned mono audio will play back on both left and right USB channel.

* Removed:

  * The uart_terminal tool to use standardized tools.
    Similar functionality is provided through the `nRF Terminal <nRF Terminal documentation_>`_ in the |nRFVSC|.
  * The functionality to jump between BIS0 and BIS1 in the :ref:`broadcast sink <nrf53_audio_broadcast_sink_app>` application.
    **Button 4** is no longer needed for this purpose due to added support for stereo audio.

nRF Desktop
-----------

* Added:

  * The :ref:`nrf_desktop_hid_eventq`.
    The utility can be used by an application module to temporarily queue HID events related to keypresses (button press or release) to handle them later.
    The utility uses 64-bit timestamps to prevent overflow issues.
  * The :ref:`nrf_desktop_hid_keymap`.
    The utility can be used by an application module to map an application-specific key ID to a HID report ID and HID usage ID pair according to statically defined user configuration.
    The :file:`hid_keymap.h` file was moved from the :file:`configuration/common` directory to the :file:`src/util` directory.
    The file is now the header of the :ref:`nrf_desktop_hid_keymap` and contains APIs exposed by the utility.
  * The :ref:`nrf_desktop_keys_state`.
    The utility can be used by an application module to track the state of active keys.
  * The :ref:`CONFIG_DESKTOP_HIDS_SUBSCRIBER_REPORT_MAX <config_desktop_app_options>` Kconfig option to :ref:`nrf_desktop_hids`.
    The option allows you to limit the number of HID input reports that can be simultaneously processed by the module.
    This limits the number of GATT notifications with HID reports in the Bluetooth stack.
  * The :ref:`nrf_desktop_ble_adv_ctrl` that is responsible for controlling the :ref:`caf_ble_adv`.
    The module suspends the |ble_adv| when the active USB device is connected (USB state is set to :c:enumerator:`USB_STATE_ACTIVE`).
    The module resumes the |ble_adv| when the USB is disconnected (USB state is set to :c:enumerator:`USB_STATE_DISCONNECTED`) and if the |ble_adv| was suspended earlier.
    This improves the USB High-Speed performance.
    To enable the module, set the :ref:`CONFIG_DESKTOP_BLE_ADV_CTRL_ENABLE <config_desktop_app_options>` Kconfig option to ``y``.
    To enable the module to suspend and resume the |ble_adv| when the USB state changes, set the :ref:`CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB <config_desktop_app_options>` Kconfig option to ``y``.
    These options are enabled for targets that support the USB High-Speed.
  * The following HID report providers:

    * :ref:`nrf_desktop_hid_provider_consumer_ctrl`
    * :ref:`nrf_desktop_hid_provider_keyboard`
    * :ref:`nrf_desktop_hid_provider_mouse`
    * :ref:`nrf_desktop_hid_provider_system_ctrl`

    The HID report providers use the new event :c:struct:`hid_report_provider_event` to establish two-way callbacks used for communication with the HID state module.
    The HID report providers are responsible for gathering input data and generating HID reports when requested by the HID state module.
    HID report providers can use newly added utilities: :ref:`nrf_desktop_hid_eventq`, :ref:`nrf_desktop_hid_keymap`, and :ref:`nrf_desktop_keys_state`.

    You can substitute existing HID report providers with your own implementation or add new HID report providers that can handle other report types.
    This change simplifies the HID state module implementation and allows code reuse.

  * The :kconfig:option:`CONFIG_BT_ID_AUTO_SWAP_MATCHING_BONDS` Kconfig option as an imply to the :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` Kconfig option.
    As a result, all nRF Desktop configurations that are set up for the Bluetooth Peripheral role have the :kconfig:option:`CONFIG_BT_ID_AUTO_SWAP_MATCHING_BONDS` Kconfig option enabled by default.
    The option allows the application to automatically swap matching bonds when the local Bluetooth identity changes.
    Due to this change, the nRF Desktop application no longer requires the :kconfig:option:`CONFIG_BT_ID_UNPAIR_MATCHING_BONDS` Kconfig option to be enabled.
    Removing it enables the possibility for bonding with the same HID host on multiple Bluetooth local identities for all nRF Desktop peripherals.

* Updated:

  * The application configurations for dongles on memory-limited SoCs (such as nRF52820) to reuse the system workqueue for GATT Discovery Manager (:kconfig:option:`CONFIG_BT_GATT_DM_WORKQ_SYS`).
    This helps to reduce RAM usage.
  * Link Time Optimization (:kconfig:option:`CONFIG_LTO`) to be enabled in MCUboot configurations of the nRF52840 DK (``mcuboot_smp``, ``mcuboot_qspi``).
    LTO no longer causes boot failures and it reduces the memory footprint.
  * The :ref:`nrf_desktop_hids` to use shared callbacks for multiple HID reports:

    * Use the :c:func:`bt_hids_inp_rep_send_userdata` function to send HID input reports while in report mode.
    * Use an extended callback with the notification event to handle subscriptions for HID input reports in report mode (:c:struct:`bt_hids_inp_rep`).
    * Use generic callbacks to handle HID feature and output reports.

    This approach simplifies the process of adding support for new HID reports.
  * :ref:`nrf_desktop_hid_state`:

    * The module uses the :ref:`nrf_desktop_hid_eventq` to temporarily queue HID events related to keypresses before a connection to the HID host is established.
    * The module uses the :ref:`nrf_desktop_hid_keymap` to map an application-specific key ID from :c:struct:`button_event` to a HID report ID and HID usage ID pair.
    * The module uses the :ref:`nrf_desktop_keys_state` to track the state of active keys.
    * The module offloads some of its responsibilities to HID report providers modules: :ref:`nrf_desktop_hid_provider_consumer_ctrl`, :ref:`nrf_desktop_hid_provider_keyboard`, :ref:`nrf_desktop_hid_provider_mouse`, and :ref:`nrf_desktop_hid_provider_system_ctrl`.

    The features were implemented directly in the HID state module before.
    This change simplifies the HID state module implementation and allows code reuse.
  * The HID input and output report maps (``input_reports`` and ``output_reports`` arrays defined in the :file:`configuration/common/hid_report_desc.h` file) to contain only IDs of enabled HID reports.
  * The default value of the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT` Kconfig option to ``64``.
    This ensures that more complex configurations fit in the limit.
  * The :ref:`nrf_desktop_hid_reportq` to accept HID report IDs that do not belong to HID input reports supported by the application (are not part of the ``input_reports`` array defined in :file:`configuration/common/hid_report_desc.h` file).
    Before the change, providing an unsupported HID report ID caused an assertion failure.
    Function signatures of the :c:func:`hid_reportq_subscribe` and :c:func:`hid_reportq_unsubscribe` functions were slightly changed (both functions return an error in case the provided HID report ID is unsupported).
  * The number of ATT buffers (:kconfig:option:`CONFIG_BT_ATT_TX_COUNT`) in application configuration for nRF Desktop peripherals.
    Extra ATT buffers are no longer needed for keyboards as :ref:`nrf_desktop_hids` limits the maximum number of simultaneously processed HID input reports (:ref:`CONFIG_DESKTOP_HIDS_SUBSCRIBER_REPORT_MAX <config_desktop_app_options>`) to ``2`` by default.
  * The nRF Desktop application to align the defaults of :kconfig:option:`CONFIG_BT_ATT_TX_COUNT` and :kconfig:option:`CONFIG_BT_CONN_TX_MAX` Kconfig options to application needs.
    The options are no longer explicitly set in application configurations.
  * By increasing the default first HID report delay (:ref:`CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY <config_desktop_app_options>`) for keyboard (:ref:`CONFIG_DESKTOP_PERIPHERAL_TYPE_KEYBOARD <config_desktop_app_options>`) in :ref:`nrf_desktop_hids` from ``500 ms`` to ``1000 ms``.
    This change ensures that queued keypresses are not lost when reconnecting with the nRF Desktop dongle.
  * By improving HID subscription handling in the HID transports (:ref:`nrf_desktop_hids` and :ref:`nrf_desktop_usb_state`).
    Both HID transports now unsubscribe from HID input reports related to the previously used HID protocol mode before subscribing to HID input reports related to the new HID protocol mode.
    This change ensures that subscriptions to both HID boot and HID report protocol mode are not enabled at the same time.
  * The :ref:`nrf_desktop_fn_keys` to subscribe for :c:struct:`button_event` as the first subscriber (:c:macro:`APP_EVENT_SUBSCRIBE_FIRST`) by default.
    You can disable the :ref:`CONFIG_DESKTOP_FN_KEYS_BUTTON_EVENT_SUBSCRIBE_FIRST <config_desktop_app_options>` Kconfig option to use early subscription (:c:macro:`APP_EVENT_SUBSCRIBE_EARLY`).
  * The :ref:`nrf_desktop_passkey` and :ref:`nrf_desktop_buttons_sim` to subscribe for :c:struct:`button_event` as an early subscriber (:c:macro:`APP_EVENT_SUBSCRIBE_EARLY`).
    This allows the modules to process the event before other application modules.
  * The memory layout in every configuration variant of the ``nrf54l15dk/nrf54l10/cpuapp`` board target to fix the out-of-bound partition allocations.
    Previously, it was assumed that the memory size for this board target was 10 KB larger than the actual one.
    The NVM size in the nRF54L10 SoC is equal to 1012 KB.

    This change in the nRF54L10 partition map is a breaking change and cannot be performed using DFU.
    As a result, the DFU procedure will fail if you attempt to upgrade the sample firmware based on one of the |NCS| v3.0 releases.
  * The behavior of the :ref:`nrf_desktop_usb_state_pm` on USB cable disconnection.
    While disconnecting the USB cable, the :c:enumerator:`USB_STATE_SUSPENDED` USB state might be reported before the :c:enumerator:`USB_STATE_DISCONNECTED` USB state.
    For application to behave consistently regardless of whether the :c:enumerator:`USB_STATE_SUSPENDED` USB state was reported, the module submits a :c:struct:`force_power_down_event` to force a quick power down.
    The module also restricts the power down level to the :c:enumerator:`POWER_MANAGER_LEVEL_SUSPENDED`.
    Then, after the delay configurable by :ref:`CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS <config_desktop_app_options>` Kconfig option, the module removes the power down level restriction.
    This allows you to take actions, such as restarting Bluetooth LE advertising, after disconnecting the USB cable without going through reboot.
  * The configurations for nRF54L-based board targets that store the MCUboot verification key in the KMU peripheral to automatically generate the :file:`keyfile.json` file in the build directory (the :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` sysbuild Kconfig option) based on the input file provided by the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` sysbuild Kconfig option.
    This KMU provisioning step can now be performed automatically by the west runner, provided that a :file:`keyfile.json` file is present in the build directory.
    The provisioning is only performed if the ``west flash`` command is executed with the ``--erase``  or ``--recover`` flag.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added:

  * Power-optimized configuration for the :zephyr:board:`nrf54h20dk` board target.
  * A single-core release configuration for the :zephyr:board:`nrf54h20dk` board target.

Serial LTE modem
----------------

* Added:

  * The ``AT#XAPOLL`` command to asynchronously poll sockets for data.
  * The send flags for ``#XSEND``, ``#XSENDTO``, ``#XTCPSEND``, and ``#XUDPSEND`` commands.
  * The send flag value ``512`` for waiting for acknowledgment of the sent data.

* Updated:

  * The ``AT#XPPP`` command to support the CID parameter to specify the PDN connection used for PPP.
  * The ``#XPPP`` notification to include the CID of the PDN connection used for PPP.
  * The initialization of the application to ignore a failure in nRF Cloud module initialization.
    This occurs sometimes especially during development.
  * The initialization of the application to send "INIT ERROR" over UART and show clear error log to indicate that the application is not operational in case of failing initialization.
  * The PPP downlink data to trigger the indicate pin when SLM is idle.
  * The ``AT#XTCPCLI`` and the ``AT#XUDPCLI`` commands to support CID of the PDN connection.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added experimental ``llvm`` toolchain support for the nRF54L Series board targets to the following samples:

  * :ref:`peripheral_lbs`
  * :ref:`central_uart`
  * :ref:`power_profiling`

* Removed SUIT support from ``mcumgr_bt_ota_dfu``.

* :ref:`bluetooth_isochronous_time_synchronization` sample:

  * Fixed an issue where the sample would assert with the :kconfig:option:`CONFIG_ASSERT` Kconfig option enabled.
    This was due to calling the :c:func:`bt_iso_chan_send` function from a timer ISR handler and sending SDUs to the controller with invalid timestamps.

* :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples:

  * Added a workaround to an issue with unexpected disconnections that resulted from improper handling of the Bluetooth Link Layer procedures by the connected Bluetooth Central device.
    This resolves the :ref:`known issue <known_issues>` NCSDK-33632.

* :ref:`nrf_auraconfig` sample:

  * Updated the buffer count (:kconfig:option:`CONFIG_BT_ISO_TX_BUF_COUNT`) to be in-line with SoftDevice Controller (SDC) defaults.
    This can be changed and optimized for specific use cases.

* :ref:`direct_test_mode` sample:

  * Fixed a bug in the workaround for errata 216 on nRF54H20 devices.
    The device asserted when a packet was received during reception tests and too few packets were transmitted during transmission tests.

* :ref:`direction_finding_peripheral` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54l15dk/nrf54l05/cpuapp`` board targets.
  * Updated by disabling Direction Finding TX AoD (antenna switching) by default in the sample.

* :ref:`direction_finding_connectionless_tx` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54l15dk/nrf54l05/cpuapp`` board targets.

Bluetooth Mesh samples
----------------------

* :ref:`bt_mesh_chat` sample:

  * Updated the board configuration files for nRF54L15, nRF54L10, and nRF54L05 by increasing the value of the :kconfig:option:`CONFIG_SHELL_STACK_SIZE` Kconfig option.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Added:

    * The integration of the :ref:`bt_fast_pair_adv_manager_readme` helper module (:kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER`) that replaces the application module for managing Fast Pair advertising.
      The sample uses the new module with the locator tag extension (:kconfig:option:`CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG`) that automates common advertising scenarios for this use case.
      As a result, the triggers for the FMDN provisioning and clock synchronization are now handled by the :ref:`bt_fast_pair_adv_manager_readme` module and are no longer part of the application code.
    * Possibility to build and run the sample without the motion detector support (with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR` Kconfig option disabled).

  * Updated:

    * The button action for controlling the Fast Pair advertising to limit its applicability.
      Now, this action allows only to enter and exit the pairing mode when the device is not provisioned.
      It is disabled immediately once the FMND provisioning is started.
    * The advertising to no longer rotate the Resolvable Private Address (RPA) in the DFU mode.
    * The :ref:`fast_pair_locator_tag_testing_fw_update_notifications` section to improve the test procedure.
      The application now provides an additional log message to indicate that the firmware version is being read.
    * The memory layout for the ``nrf54l15dk/nrf54l10/cpuapp`` board target to fix the out-of-bound partition allocations.
      Previously, it was assumed that the memory size for this board target was 10 KB larger than the actual one.
      The NVM size in the nRF54L10 SoC is equal to 1012 KB.

      This change in the nRF54L10 partition map is a breaking change and cannot be performed using DFU.
      As a result, the DFU procedure will fail if you attempt to upgrade the sample firmware based on one of the |NCS| v3.0 releases.
    * The configurations for nRF54L-based board targets that store the MCUboot verification key in the KMU peripheral to automatically generate the :file:`keyfile.json` file in the build directory (the :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` Kconfig option) based on the input file provided by the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` Kconfig option.
      This KMU provisioning step can now be performed automatically by the west runner, provided that a :file:`keyfile.json` file is present in the build directory.
      The provisioning is only performed if the ``west flash`` command is executed with the ``--erase``  or ``--recover`` flag.
    * Link Time Optimization (:kconfig:option:`CONFIG_LTO`) to be enabled in MCUboot configurations of the nRF5340 DK and Thingy:53.
      LTO no longer causes boot failures and it reduces the memory footprint.

Cellular samples
----------------

* Added support for the Thingy:91 X to the following samples:

  * :ref:`nrf_cloud_rest_device_message`
  * :ref:`nrf_cloud_rest_cell_location`
  * :ref:`nrf_cloud_rest_fota`

* Deprecated the LTE Sensor Gateway sample.
  It is no longer maintained.

* :ref:`modem_shell_application` sample:

  * Added:

    * ``ATE0`` and ``ATE1`` commands in AT command mode to handle echo off/on.
    * Support for RX only mode to the ``link funmode`` command.
    * Support for ``AT%CMNG`` multi-line commands.

* nRF Cloud multi-service sample:

  * Added support for native simulator platform and updated the documentation accordingly.

* :ref:`nrf_provisioning_sample` sample:

  * Updated:

    * The sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.
    * The sample by enabling the :ref:`lib_at_shell` library to allow the nRF Cloud Utils to interface with the device.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Updated the sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.
  * Removed Provisioning service and JITP.

* :ref:`nrf_cloud_rest_cell_location` sample:

  * Updated the sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.
  * Removed JITP.

* :ref:`nrf_cloud_rest_fota` sample:

  * Updated the sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.
  * Fixed SMP FOTA for the nRF9160 DK.
  * Removed JITP.

Cryptography samples
--------------------

* :ref:`crypto_aes_gcm` sample:

  * Added a note stating that CRACEN only supports a 96-bit IV for AES GCM.

DFU samples
-----------

* Added a new sample category :ref:`dfu_samples` and a new :ref:`dfu_target_sample` sample that demonstrates the use of the DFU target functionality in the |NCS|.

Matter samples
--------------

* Added:

  * Support for the NFC onboarding for the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target.

* Updated:

  * The Bluetooth Low Energy variant of the Soft Device Controller (SDC) to use the Peripheral-only role in all Matter samples.
  * The API of the ``ncs_configure_data_model()`` CMake method.
    It does not use ``ZAP_FILE`` argument anymore, but creates path to ZAP file based on :ref:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH <CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH>` Kconfig option.
  * By renaming the ``CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH`` Kconfig option to :ref:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH <CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH>` and changed its purpose to configure the absolute path under which the ZAP file is located.
  * By enabling Matter persistent subscriptions by default for all Matter samples.
  * By changing the default values of the following ICD parameters:

    * :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL` from ``1000`` to ``2500`` ms for SIT devices.
    * :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD` from ``300`` to ``0`` ms for SIT devices.
    * :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL` from ``200`` to ``500`` ms.

  * The memory layout for the ``nrf54l15dk/nrf54l10/cpuapp`` board target, as the previous one was invalid and allowed to access memory area out of bounds.
    The maximum size of the non-volatile area was changed from 1022 kB to 1012 kB, the application partition size was decreased by 8 kB, and the reserved partition was removed.
  * Disabled usage of Zephyr L2 networking layer in favor of using the OpenThread API directly in the Matter over Thread applications.

* :ref:`matter_light_switch_sample`:

  * Updated the testing steps to use the proper commands for groupcast binding.
    This resolves the :ref:`known issue <known_issues>` KRKNWK-19277.

Networking samples
------------------

* Added support for the nRF7002 EK with the ``nrf5340dk/nrf5340/cpuapp/ns`` board target in the following samples:

  * :ref:`download_sample`
  * :ref:`net_coap_client_sample`
  * :ref:`https_client`
  * :ref:`mqtt_sample`
  * :ref:`udp_sample`
  * :ref:`http_server`

* :ref:`download_sample` sample:

  * Added the :ref:`CONFIG_SAMPLE_PROVISION_CERT <CONFIG_SAMPLE_PROVISION_CERT>` Kconfig option to provision the root CA certificate to the modem.
    The certificate is provisioned only if the :ref:`CONFIG_SAMPLE_SECURE_SOCKET <CONFIG_SAMPLE_SECURE_SOCKET>` Kconfig option is set to ``y``.
  * Fixed an issue where the network interface was not re-initialized after a fault.

NFC samples
-----------

* Added experimental ``llvm`` toolchain support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target to the following samples:

  * :ref:`writable_ndef_msg`
  * :ref:`nfc_shell`

* :ref:`record_text` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added experimental ``llvm`` toolchain support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target.

* :ref:`802154_phy_test` sample:

  * Added print of sent packets and received Acks after ``ltx`` command.

PMIC samples
------------

* :ref:`nPM1300: Fuel gauge <npm13xx_fuel_gauge>` sample:

  * Updated current sign when calling the ``nrf_fuel_gauge`` lib functions.
    In the Zephyr sensor API the convention for the gauge current is ``negative=discharging``, while the :ref:`nrfxlib:nrf_fuel_gauge` lib uses the opposite.

SUIT samples
------------

* Removed all SUIT samples:

  * SUIT: Device firmware “A/B” update on the nRF54H20 SoC
  * SUIT: Flash companion
  * SUIT: Recovery application
  * SUIT: Device firmware update on the nRF54H20 SoC

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_secure_peripheral_partition` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target.

Thread samples
--------------

* Added the new :ref:`architecture option <openthread_stack_architecture>` to use the OpenThread stack directly to communicate with the IEEE 802.15.4 radio driver in the following samples:

  * :ref:`ot_coprocessor_sample`
  * :ref:`coap_server_sample`
  * :ref:`ot_cli_sample`

Wi-Fi samples
-------------

* Added:

  * The :ref:`wifi_provisioning_internal_sample` sample that demonstrates the core Wi-Fi provisioning functionality independently of the transport layer (Bluetooth LE or Wi-Fi SoftAP).
  * Runtime support for Bluetooth LE based Wi-Fi provisioning in enterprise mode.

* Removed system mode and the ``wpa_supplicant`` dependency for monitor mode optimization.

* :ref:`wifi_radiotest_samples`:

  * Updated :ref:`wifi_radio_test` and :ref:`wifi_radio_test_sd` samples to clarify platform support for single-domain and multi-domain radio tests.

* :ref:`wifi_shutdown_sample`:

  * Updated the sample to include both One-shot and Continuous modes of operations.

Other samples
-------------

* Added the :ref:`mcuboot_minimal_configuration` sample that demonstrates the minimal and recommended settings for MCUboot on the nRF54L15 DK.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added the :ref:`mspi_sqspi` that allows for communication with devices that use MSPI bus-based Zephyr drivers.

Flash drivers
-------------

* Removed the SUIT flash IPUC driver.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* Updated the Wi-Fi Provisioning Service library by splitting it into the :ref:`lib_wifi_prov_ble` library and :ref:`lib_wifi_prov_core` library .

* :ref:`bt_fast_pair_readme` library:

  * Added the new :ref:`bt_fast_pair_adv_manager_readme` helper module that can be used to manage the Fast Pair advertising set.
    The module implements a trigger-based system for controlling Fast Pair advertising state that allows client modules to request advertising with their preferred configuration.
    It also defines the use case layer that provides implementation of specific advertising requirements for supported use cases.

  * Updated:

    * The software maturity level from experimental to supported for the nRF54L05 SoC (the ``nrf54l15dk/nrf54l05/cpuapp`` board target) and nRF54L10 SoC (the ``nrf54l15dk/nrf54l10/cpuapp`` board target) in the locator tag use case.
      Fast Pair features, which are required to implement this use case, have also been promoted to the supported maturity level:

      * Initial Pairing
      * Find My Device Network extension

    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_MOTION_DETECTOR` Kconfig option dependency.

  * Removed a workaround for the issue where the FMDN clock value might not be correctly set after the system reboot for nRF54L Series devices.
    The kernel uptime value that is returned by the :c:func:`k_uptime_get` function is now correctly set to ``0`` during the system bootup process for each reset type.
    As a result, the workaround for the FMDN clock value is no longer needed.
    For details, see the ``NCSDK-32268`` known issue in the :ref:`known_issues` page.

Common Application Framework
----------------------------

* :ref:`caf_ble_state`:

  * Removed the tracking of the active Bluetooth connections.
    CAF no longer assumes that the Bluetooth Peripheral device (:kconfig:option:`CONFIG_BT_PERIPHERAL`) supports only one simultaneous connection (:kconfig:option:`CONFIG_BT_MAX_CONN`).

* :ref:`caf_ble_adv`:

  * Updated the module implementation to handle the newly introduced module suspend request event (:c:struct:`module_suspend_req_event`) and module resume request event (:c:struct:`module_resume_req_event`).
    When entering the suspended state, the module stops Bluetooth LE advertising and disconnects connected peers.
    To enable support for these events, use the :kconfig:option:`CONFIG_CAF_BLE_ADV_MODULE_SUSPEND_EVENTS` Kconfig option, which is enabled by default.
    When the :kconfig:option:`CONFIG_CAF_BLE_ADV_SUSPEND_ON_READY` Kconfig option is enabled, the module is suspended automatically right after initialization.

Debug libraries
---------------

* Added an experimental :ref:`Zephyr Core Dump <zephyr:coredump>` backend that writes a core dump to an internal flash or RRAM partition.
  To enable this backend, set the :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_OTHER` and :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_NRF_FLASH_PARTITION` Kconfig options.

* :ref:`cpu_load` library:

  * Added prefix ``NRF_`` to all Kconfig options (for example, :kconfig:option:`CONFIG_NRF_CPU_LOAD`) to avoid conflicts with Zephyr Kconfig options with the same names.

Security libraries
------------------

* :ref:`nrf_security` library:

  * Updated:

    * The name of the Kconfig option ``CONFIG_PSA_USE_CRACEN_ASYMMETRIC_DRIVER`` to :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER`, which is more descriptive and more consistent with the options of the other drivers.
    * The placement of the page about nRF Security drivers.
      The page was moved to :ref:`ug_crypto_index` and renamed to :ref:`crypto_drivers`.


Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

  * Fixed an issue with modem fault handling in the :ref:`nrf_modem_lib_lte_net_if`, where the event must be deferred from interrupt context before it can be forwarded to the Zephyr's :ref:`net_mgmt_interface` module.

* :ref:`at_parser_readme` library:

  * Added support for parsing DECT NR+ modem firmware names.

  * Updated the following macros and functions to return ``-ENODATA`` when the target subparameter to parse is empty:

    * :c:macro:`at_parser_num_get` macro
    * Functions:

      * :c:func:`at_parser_int16_get`
      * :c:func:`at_parser_uint16_get`
      * :c:func:`at_parser_int32_get`
      * :c:func:`at_parser_uint32_get`
      * :c:func:`at_parser_int64_get`
      * :c:func:`at_parser_uint64_get`
      * :c:func:`at_parser_string_get`

* :ref:`lte_lc_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` and :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig options to enable setting a fallback DNS address.
      The :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` Kconfig option is enabled by default.
      If the application has configured a DNS server address in Zephyr's native networking stack, using the :kconfig:option:`CONFIG_DNS_SERVER1` Kconfig option, the same server is set as the fallback address for DNS queries offloaded to the nRF91 Series modem.
      Otherwise, the :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig option controls the fallback DNS server address that is set to Cloudflare's DNS server 1.1.1.1 by default.
      The device might or might not receive a DNS address by the network during PDN connection.
      Even within the same network, the PDN connection establishment method (PCO vs ePCO) might change when the device operates in NB-IoT or LTE Cat-M1, resulting in missing DNS addresses when one method is used, but not the other.
      Having a fallback DNS address ensures that the device always has a DNS to fallback to.

  * Updated modem events subscription to persist between functional mode changes.

  * Removed:

    * The deprecated functions ``lte_lc_reduced_mobility_get()``, ``lte_lc_reduced_mobility_set()``, and ``lte_lc_factory_reset()``.
    * The deprecated macro ``LTE_LC_ON_CFUN()``.

* Modem SLM library:

  * Added the following Kconfig options:

    * ``CONFIG_MODEM_SLM_UART_RX_BUF_COUNT`` for configuring RX buffer count.
    * ``CONFIG_MODEM_SLM_UART_RX_BUF_SIZE`` for configuring RX buffer size.
    * ``CONFIG_MODEM_SLM_UART_TX_BUF_SIZE`` for configuring TX buffer size.
    * ``CONFIG_MODEM_SLM_AT_CMD_RESP_MAX_SIZE`` for buffering AT command responses.

  * Updated:

      * The software maturity of the library to supported instead of experimental.
      * The UART implementation between the host device, using the Modem SLM library, and the device running the Serial LTE modem application.

  * Removed:

    * The ``CONFIG_MODEM_SLM_DMA_MAXLEN`` Kconfig option.
      Use ``CONFIG_MODEM_SLM_UART_RX_BUF_SIZE`` instead.
    * The ``modem_slm_reset_uart()`` function, as there is no longer a need to reset the UART.

* :ref:`modem_info_readme` library:

  * Added:

    * The :c:func:`modem_info_get_rsrq` function for requesting the RSRQ.
    * The :c:macro:`SNR_IDX_TO_DB` macro for converting the SNR index to dB.

Multiprotocol Service Layer libraries
-------------------------------------

* Added an implementation of the API required by the MPSL (defined by :file:`mpsl_hwres.h`) for the nRF53 and nRF54L Series devices.

* Updated the implementation of the following interrupt service routine wrappers:

  * :c:func:`mpsl_timer0_isr_wrapper`
  * :c:func:`mpsl_rtc0_isr_wrapper`
  * :c:func:`mpsl_radio_isr_wrapper`

  Now, they do not trigger the kernel scheduler or use any kernel APIs.

  .. note::

     Invoking kernel APIs or triggering the kernel scheduler from Zero Latency Interrupts is considered undefined behavior.
     Users of MPSL timeslots should not assume that thread rescheduling will occur automatically at the end of a timeslot.

* Fixed an issue where calling the :c:func:`mpsl_lib_uninit` function would prevent calibration of the RC oscillator when MPSL was subsequently re-initialized using the :c:func:`mpsl_lib_init` function.

  This could happen, for instance, when using Bluetooth with the :kconfig:option:`CONFIG_BT_UNINIT_MPSL_ON_DISABLE` Kconfig option enabled.
  The low-frequency clock had poor accuracy in this case.

Libraries for networking
------------------------

* Added the :ref:`wifi_prov_tools` library that generates Protocol Buffers (protobuf) configuration messages for Wi-Fi provisioning, supporting both EAP-TLS (Enterprise) and Personal (WPA2-PSK/WPA3-PSK) modes.
* Updated the Wi-Fi Provisioning Service library by splitting it into the :ref:`lib_wifi_prov_ble` library and :ref:`lib_wifi_prov_core` library .
  The :ref:`lib_wifi_prov_core` library is now a standalone library that can be used by applications to implement the Wi-Fi provisioning functionality independently of the Bluetooth stack.

* :ref:`lib_nrf_cloud` library:

  * Updated:

    * To return negative :file:`errno.h` errors instead of positive ZCBOR errors.
    * The CoAP download authentication to no longer depend on the :ref:`CoAP Client library <zephyr:coap_client_interface>`.

* :ref:`lib_nrf_provisioning` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_COAP_MAX_RETRIES` Kconfig option to configure the maximum number of retries for CoAP requests.
    * The :kconfig:option:`CONFIG_NRF_PROVISIONING_INITIAL_BACKOFF` Kconfig option to configure the initial backoff time for provisioning retries.
    * The :kconfig:option:`CONFIG_NRF_PROVISIONING_STACK_SIZE` Kconfig option to configure the stack size of the provisioning thread.
    * A new query parameter to limit the number of provisioning commands included in a single provisioning request.
      You can set this limit using the :kconfig:option:`CONFIG_NRF_PROVISIONING_CBOR_RECORDS` Kconfig option.

  * Updated:

    * Limited key-value pairs in a single provisioning command to ``10``.
      This is done to reduce the RAM usage of the library.

  * Fixed an issue where the results from the :c:func:`zsock_getaddrinfo` function were not freed when the CoAP protocol was used for connection establishment.

* :ref:`lib_downloader` library:

  * Fixed:

    * A bug in the shell implementation causing endless download retries on errors.
    * A bug in the shell to allow multiple downloads.
    * An issue with error handling where network-down errors were not properly forwarded to the application during reconnection failures.

nRF RPC libraries
-----------------

* The nRF RPC device information library has been replaced :ref:`nrf_rpc_utils`, a more generic library.

Other libraries
---------------

* :ref:`dult_readme` library:

  * Updated the write handler of the accessory non-owner service (ANOS) GATT characteristic to no longer assert on write operations if the DULT was not enabled at least once.

* :ref:`supl_client` library:

  * Updated the SUPL client OS integration library to remove the dependency on the newlib C library.
    To use SUPL with picolibc, v0.8.0 or later of the nRF91 Series SUPL client library is required.

* :ref:`emds_readme` library:

  * Added support of the robust data storage mechanism, as it enables the library to always have a valid snapshot of the stored data.

  * Updated:

    * By redesigning the library to support two partitions for storing data.
    * By redesigning the data format in which the library stores data.
    * By changing the function :c:func:`emds_store_size_get` for calculation of the needed size to store the registered data.
    * By changing the function :c:func:`emds_store_time_get` for estimation of the needed time to write the registered data into persistent memory.

  * Removed experimental status.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

* Added the :file:`ncs_ironside_se_update.py` script in the :file:`scripts/west_commands` folder.
  The script adds the west command ``west ncs-ironside-se-update`` for installing an IronSide SE update.

* :ref:`nrf_desktop_config_channel_script` Python script:

  * Updated:

    * The udev rules for Debian, Ubuntu, and Linux Mint HID host computers (replaced the :file:`99-hid.rules` file with :file:`60-hid.rules`).
      This is done to ensure that the rules are properly applied for an nRF Desktop device connected directly over Bluetooth LE.
      The new udev rules are applied to any HID device that uses the Nordic Semiconductor Vendor ID (regardless of Product ID).
    * The HID device discovery to ensure that a discovery failure of a HID device would not affect other HID devices.
      Without this change, problems with discovery of a HID device could lead to skipping discovery and listing of other HID devices (even if the devices work properly).

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Added the :ref:`ug_bt_fast_pair_adv_manager` page that describes how to integrate the :ref:`bt_fast_pair_adv_manager_readme` module in your application.
* Updated the :ref:`ug_bt_fast_pair` page to mention the availability of the guide for :ref:`ug_bt_fast_pair_adv_manager` that covers the associated helper module.
  Mentioned applicability of the :ref:`bt_fast_pair_adv_manager_readme` module in the :ref:`ug_bt_fast_pair_advertising` and the :ref:`ug_bt_fast_pair_use_case_locator_tag` sections.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``81315483fcbdf1f1524c2b34a1fd4de6c77cd0f4``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Support as a first-stage immutable bootloader using hardware cryptographic acceleration on nRF54L10 and nRF54L05.
  * Support for AES-256 image encryption.
  * Support for hardware downgrade protection.
  * Support for SHA512 signature verification.
  * Linking Time Optimization support.

* Updated the following security features:

  * HW downgrade prevention.
  * RAM memory cleanup controlled by the ``CONFIG_MCUBOOT_CLEANUP_RAM`` Kconfig option.
  * ECIES-X25519 encryption improvements.

* Fixed:

  * Several stability and functionality improvements.
  * An issue related to referencing the ARM Vector table of the application, which causes jumping to wrong address instead of the application reset vector for some builds when Zephyr LTO was enabled.

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

ADC drivers
+++++++++++

* Added the ``NRF_SAADC_GND`` analog input to allow negative values for single-ended settings.

Trusted Firmware-M
==================

* Updated to TF-M version 2.1.2, which is a bugfix release addressing issues in protected storage and in the secure partition manager.
  This release is integrated towards Mbed TLS v3.6.4.

Documentation
=============

* Added:

  * The :ref:`asset_tracker_template_redirect` page, which provides the information about the `Asset Tracker Template Add-on <Asset Tracker Template_>`_.
  * The :ref:`log_rpc` library documentation page.
  * The :ref:`mcuboot_serial_recovery` documentation page, based on the official Zephyr documentation, which discusses the implementation and usage of the serial recovery.
  * The :ref:`data_storage` page, which covers storage alternatives for general data, including NVMC, NVS, file systems, Settings, and PSA Protected Storage, with feature comparisons and configuration examples.
  * The :ref:`key_storage` page, which covers storage alternatives for cryptographic keys, including PSA Crypto API, Hardware Unique Keys (HUK), modem certificate storage, and other security-focused storage mechanisms.

* Removed:

  * The Wi-Fi credentials library page and moved to the upstream :ref:`Zephyr repository <zephyr:lib_wifi_credentials>`.
  * The Getting started with nRF7002 DK and Getting started with other DKs pages from the :ref:`gsg_guides` section.
    These pages were no longer relevant as the `Quick Start app`_ now also supports the nRF7002 DK.
