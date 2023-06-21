.. _ncs_release_notes_160:

|NCS| v1.6.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the TF-M, MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices are now supported for production.
Wireless protocol stacks and libraries may indicate support for development or support for production for different series of devices based on verification and certification status.
See the release notes and documentation for those protocols and libraries for further information.

Highlights
**********

* Added production support for nRF5340 for Bluetooth LE in single protocol configuration and in multiprotocol operation with Thread for Bluetooth Peripheral role.
* Added production support for multiple advertising sets to SoftDevice Controller.
* Added features available in Trusted Firmware-M (TF-M), which is supported for development with nRF5340 and nRF9160.
* Integrated Apple Find My, supported for production.
* Added development support for the Pelion Device Management library for LTE-M/NB-IoT and Thread.
* Added the :ref:`nrf_machine_learning_app` application for Thingy:52, which further integrates Edge Impulse Machine Learning with the |NCS|.
* Added support for all Thread 1.2 mandatory features for routers and end devices supported for development for the nRF52 family.
* Integrated more complete support for Matter (Project CHIP) which is supported for development for nRF52840 and nRF5340.
* Integrated Apple HomeKit ADK v5.3 and added development support for nRF52832 and nRF52833 for Bluetooth LE HomeKit accessories.
* The application layer part of ZBOSS Zigbee stack is available in source code now.
* New sample :ref:`modem_shell_application` can be used to test various connectivity features.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.6.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Known issues
************

See `known issues for nRF Connect SDK v1.6.0`_ for the list of issues valid for this release.

Important known issues
======================

These known issues are potential regressions or high impact issues:

* :ref:`SoftDevice Controller DRGN-15852 <drgn_15852>`: Affects scanning on the nRF53 Series.
* :ref:`HomeKit KRKNWK-10611 <krknwk_10611>`: DFU will fail when using external flash memory for update image storage.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added:

  * :ref:`lib_modem_jwt` library:

    * The library provides an API to obtain a JSON Web Token (JWT) from the modem.
    * This functionality requires modem firmware v1.3.0 or higher.

  * :ref:`lib_modem_attest_token` library:

    * The library provides an API to get an attestation token from the modem.
    * Functionality requires modem firmware v1.3.0 or higher.

  * :ref:`mod_memfault` module and integration on |NCS| for nRF9160-based devices:

    * Integration of Memfault SDK into |NCS|.

  * :ref:`memfault_sample` sample:

    * Implements :ref:`mod_memfault` module and shows how to use the Memfault SDK functionality in an application.

  * :ref:`modem_shell_application` sample:

    * Enables testing of various connectivity features such as link handling, TCP/IP connections, data throughput (curl and iPerf3), SMS, GNSS, FOTA, and PPP.

  * :ref:`lib_nrf_cloud_pgps` library:

    * The library adds P-GPS (Predicted GPS) support to the :ref:`lib_nrf_cloud` library.

  * :ref:`sms_sample` sample:

    * Demonstrates how you can send and receive SMS messages with your nRF9160-based device using the :ref:`sms_readme` library.

  * :ref:`pdn_sample` sample:

    * Demonstrates how to create and configure a Packet Data Protocol (PDP) context, activate a Packet Data Network connection, and receive events on its state and connectivity using the :ref:`pdn_readme` library.

  * :ref:`pdn_readme` library:

    * Manages Packet Data Protocol (PDP) contexts and PDN connections.

  * :ref:`lib_lwm2m_client_utils` library:

    * Created from common parts of :ref:`lwm2m_client` sample.
    * This module can be reused to add common objects to LwM2M applications.

* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Added cellular positioning support to the :ref:`lib_nrf_cloud_cell_pos` library.
    * Added Kconfig option :kconfig:option:`CONFIG_NRF_CLOUD_CELL_POS` to obtain cell-based location from nRF Cloud instead of using the modem's GPS.
    * Added function :c:func:`nrf_cloud_modem_fota_completed`, which is to be called by the application after it re-initializes the modem (instead of rebooting) after a modem FOTA update.
    * Updated to include the FOTA type value in the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event.
    * Updated configuration options for setting the source of the MQTT client ID (nRF Cloud device ID).
    * Updated nRF Cloud FOTA to use type-validated FOTA download.

  * nRF9160: Asset Tracker application:

    * Added optional P-GPS support.
    * Added application-specific option ``CONFIG_PGPS_STORE_LOCATION``.
    * Added :file:`overlay-pgps.conf` to enable P-GPS support.
    * Added :file:`overlay-agps-pgps.conf` to enable A-GPS and P-GPS support.
    * Updated to handle new Kconfig options:

      * :kconfig:option:`CONFIG_NRF_CLOUD_CELL_POS`

  * :ref:`asset_tracker_v2` application:

    * Added support for Azure IoT Hub.
    * Added support for nRF Cloud.

  * :ref:`modem_info_readme` library:

    * Updated to prevent reinitialization of param list in :c:func:`modem_info_init`.

  * :ref:`lib_fota_download` library:

    * Added an API to retrieve the image type that is being downloaded.
    * Added an API to cancel current downloading.
    * Added an API to validate FOTA image type before starting installation.

  * :ref:`lib_ftp_client` library:

    * Support subset of RFC959 FTP commands only.
    * Added support of STOU and APPE (besides STOR) for "put".
    * Added detection of socket errors, report with proprietary reply message.
    * Increased FTP payload size from NET_IPV4_MTU(576) to MSS as defined on modem side (708).
    * Added polling "226 Transfer complete" after data channel TX/RX, with a configurable timeout of 60 seconds.
    * Ignored the reply code of "UTF8 ON" command as some FTP servers return abnormal replies.

  * :ref:`at_params_readme` library:

    * Added function :c:func:`at_params_int64_get` that allows for getting AT param list entries containing signed 64-bit integers.

  * :ref:`lte_lc_readme` library:

    * Added support for %XT3412 AT command notifications, which allows the application to get prewarnings before Tracking Area Updates.
    * Added support for neighbor cell measurements.
    * Added support for %XMODEMSLEEP AT command notifications, which allows the application to get notifications related to modem sleep.
    * Added support for %CONEVAL AT command that can be used to evaluate the LTE radio signal state in a cell prior to data transmission.

  * :ref:`serial_lte_modem` application:

    * Fixed TCP/UDP port range issue (0~65535).
    * Added AT#XSLEEP=2 to power off UART interface.
    * Added support for the ``verbose``, ``uput``, ``mput`` commands and data mode to the FTP service.
    * Added URC (unsolicited response code) to the FOTA service.
    * Enabled all SLM services by default.
    * Updated the HTTP client service code to handle chunked HTTP responses.
    * Added data mode to the MQTT Publish service to support JSON-type payload.
    * Added SMS support, to send/receive SMS in plain text.

  * :ref:`at_cmd_parser_readme` library:

    * Added support for parsing parameters of type unsigned int or unsigned short.

  * Secure Partition Manager (SPM) library:

    * Added support for the nRF9160 pulse-density modulation (PDM) and inter-IC sound (I2S) peripherals in non-secure applications.
    * Fixed an issue where SPM and the application could have incompatible FPU configurations, resulting in a HardFault.
      Now, the application is free to use FPU regardless of SPM configuration.

  * GPS driver:

    * Renamed ``gps_agps_request()`` to ``gps_agps_request_send()``.

  * :ref:`agps_sample` sample:

    * Added optional P-GPS support.
    * Added :file:`overlay-pgps.conf` to enable P-GPS support.
    * Added :file:`overlay-agps-pgps.conf` to enable A-GPS and P-GPS support.

  * nRF9160: AWS FOTA sample:

    * Removed nRF Cloud support code, because ``fota_v1`` is no longer supported in nRF Cloud.
    * Removed provisioning using :ref:`modem_key_mgmt` and :file:`certificates.h`, because this is not the recommended way of provisioning private certificates.
    * Renamed the following Kconfig options:

      * ``CONFIG_CLOUD_CERT_SEC_TAG`` renamed to :kconfig:option:`CONFIG_CERT_SEC_TAG`.
      * ``CONFIG_USE_CLOUD_CLIENT_ID`` renamed to :kconfig:option:`CONFIG_USE_CUSTOM_CLIENT_ID`.
      * ``CONFIG_CLOUD_CLIENT_ID`` renamed to ``CONFIG_CLIENT_ID``.
      * ``CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX`` renamed to ``CONFIG_CLIENT_ID_PREFIX``.

  * :ref:`lib_aws_fota` library:

    * Updated internal state handling and fault tolerance.

  * :ref:`sms_readme` library:

    * Updated to enable sending of SMS and decoding of received SMS payload.

  * :ref:`liblwm2m_carrier_readme` library:

    * Updated to v0.20.1.
      See :ref:`liblwm2m_carrier_changelog` for details.

nRF5
====

Bluetooth LE
------------

* Added:

  * Support for serialization of the :ref:`zephyr:bt_gap` and the :ref:`zephyr:bluetooth_connection_mgmt` API.
  * :ref:`ble_rpc_host` sample that enables support for serialization of the :ref:`zephyr:bt_gap` and the :ref:`zephyr:bluetooth_connection_mgmt`.
  * Samples demonstrating the direction finding feature based on periodic advertising (connectionless), available for the nRF52833 DK:

    * :ref:`direction_finding_connectionless_rx`
    * :ref:`direction_finding_connectionless_tx`

* Updated:

  * :ref:`shell_bt_nus` sample - Fixed an issue where shell transport did not display the initial shell prompt ``uart:~$`` on the remote terminal.

Bluetooth mesh
--------------

* Added:

  * Support for vendor-specific mesh model :ref:`bt_mesh_silvair_enocean_srv_readme`.
  * A new API function ``bt_mesh_rpl_pending_store`` to manually store pending RPL entries in the persistent storage without waiting for the timeout.
  * A ``bt_mesh_scene_entry::recall_complete`` callback that is called for each model that has a scene entry when recalling a scene data is done.

* Updated:

  * Updated the :ref:`bt_mesh_light_xyl_srv_readme` model to publish its state values that were loaded from flash after powering up.
  * Replaced the linked list of scene entries in the model contexts, with a lookup in ROM-allocated scene entries.
  * Updated so the transition pointer can be NULL, if no transition time parameters are provided in APIs.
  * Renamed Kconfig option ``CONFIG_BT_MESH_LIGHT_CTRL_STORE_TIMEOUT`` to :kconfig:option:`CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT`, and default value is set to 0.
  * Updated the :ref:`bt_mesh_light_ctrl_srv_readme` model with a timer, allowing it to resume operation after a certain delay.
  * Updated the proportional-integral (PI) feedback regulator to use instant transition time to relieve the application from overhead.
  * Fixed an issue where an undefined state for some sensor properties is a valid state, and should be handled without giving errors.
  * Fixed an issue with storing and recalling the Light OnOff state in :ref:`bt_mesh_light_ctrl_srv_readme`.
  * Fixed an issue where :ref:`bt_mesh_lightness_srv_readme` publishes twice if extended by two models.
  * Updated :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` to store their states upon a change.
  * Fixed an issue where :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` did not erase model data on reset.
  * Fixed an issue where :ref:`bt_mesh_scene_srv_readme` called scene recall at startup.
  * Fixed an issue by publishing a new value during scene recall in :ref:`bt_mesh_onoff_srv_readme` and :ref:`bt_mesh_lvl_srv_readme`.
  * Fixed issues where extended models stored or recalled instead of the extending model.
  * Updated the extending models by adding the extension API by default through Kconfig.
  * Forced the extension of :ref:`bt_mesh_lightness_srv_readme` to be initialized before :ref:`bt_mesh_light_ctrl_srv_readme`.
  * Fixed an issue where :ref:`bt_mesh_light_ctrl_srv_readme` should disable control when the lightness is set by receiving a message.
  * Added persistent storage to the :ref:`bt_mesh_scheduler_readme` to restore previously configured entries on power-up.
  * Fixed an issue where CTL temperature bindings should use rounding operation for division in the binding formula.
  * Samples are using :kconfig:option:`CONFIG_NVS` instead of :kconfig:option:`CONFIG_FCB` as the default storage backend.
  * Fixed an issue in :ref:`bt_mesh_light_ctrl_srv_readme` by always setting the transition time to a Fade Standby Manual state time when a Light Off event is received.
  * Fixed an issue by reporting maximum remaining time for all components for CTL state transition time when GET is processed.
  * Fixed an issue where a deleted :ref:`bt_mesh_scene_srv_readme` did not delete all its pages from the file system.
  * Fixed an issue where Sensor Threshold was trimmed and an invalid value was calculated.
  * Updated :ref:`bt_mesh_scheduler_srv_readme` to no longer extend :ref:`bt_mesh_scene_srv_readme`, but they must be present on the same element.
  * Moved the configuration settings for acknowledged messages into Kconfig to make them public.
  * Fixed an issue where an Occupancy On event did not transition to a Fade On state even if Occupancy Mode is disabled.
  * Added a flag to :ref:`bt_mesh_onoff_srv_readme` to skip Default Transition Time on Generic OnOff Set (Unack) messages.
  * Fixed an issue by correcting the bindings between the Generic OnOff state and the Light OnOff state.
  * Fixed an issue by clearing the internal sum in the proportional-integral (PI) feedback regulator when entering the OFF state of the state machine.
  * Fixed an issue where :ref:`bt_mesh_lightness_srv_readme` could accidentally disable :ref:`bt_mesh_light_ctrl_srv_readme` before it was started.
  * Fixed an issue by publishing the Light Lightness Status message on startup even if the OnPowerUp state is OFF.
  * Fixed issues by publishing the Light OnOff Status when disabling and restoring the Light LC state.
  * Fixed an issue where temperature and range should be within a valid default range for the :ref:`bt_mesh_light_temp_srv_readme`.
  * Removed sensor type ``BT_MESH_SENSOR_DELTA_DISABLED`` as it is removed from specification.
  * Replaced ``struct bt_mesh_model_ack_ctx`` with ``struct bt_mesh_msg_ack_ctx`` from :ref:`zephyr:bluetooth_mesh_msg`.

Matter (Project CHIP)
---------------------

* Project CHIP has been officially renamed to `Matter`_.
* Added:

  * New user guide about :ref:`ug_matter_configuring`.

* Updated:

  * Renamed occurrences of Project CHIP to Matter.
  * Updated the Matter fork in the |NCS| to the revision mentioned in the Matter (Project CHIP) submodule section below.

Thread
------

* Added support for the following Thread 1.2 features:

  * Domain Unicast Addresses
  * Multicast Listener Registration
  * Backbone Router (Thread Network side only)
  * Link Metrics
  * Coordinated Sampled Listening (CSL)

  Link Metrics and CSL are supported for the nRF52 Series devices.
* NCP sample renamed to :ref:`Co-processor <ot_coprocessor_sample>`, with added support for the :ref:`thread_architectures_designs_cp_rcp` architecture.

Zigbee
------

In this release, Zigbee is supported for development and should not be used for production.
|NCS| v1.5.1 contains the certified Zigbee solution supported for production.

* Added version 0.9.5 of the `ZBOSS NCP Host`_ package that includes a simple gateway application.
* Updated:

  * Reworked the :ref:`NCP sample <zigbee_ncp_sample>` to work with the simple gateway application.
  * Moved the `NCP Host documentation`_ from the `ZBOSS NCP Host`_ package to the same location as the `external ZBOSS development guide and API documentation`_.

nRF Desktop
-----------

* Fixed issues with boot reports on the USB backend.
* Adapted the application to use common modules from the :ref:`lib_caf` library.
* Fixed minor bugs.

Common
======

The following changes are relevant for all device families.

Trusted Firmware-M
------------------

* Added support for hardware-accelerated cryptography in TF-M using the Nordic Security module (nrf_security).
  When enabled (default), any calls to psa_crypto APIs will utilize the CryptoCell hardware on nRF9160 and nRF5340.
* Added support for using hardware unique keys (HUKs) for key derivation (``TFM_CRYPTO_ALG_HUK_DERIVATION``).
  TF-M automatically generates and stores random hardware unique keys (if not present), using the :ref:`lib_hw_unique_key` library.
* TFM_MINIMAL: Added a size-optimized configuration of TF-M which provides a minimal set of features:

  * This configuration requires 32 KB of flash and provides random number generation, SHA-256, and the platform memory read service, which is analogous to the feature set of Secure Partition Manager.
  * The configuration is showcased in the :ref:`tfm_hello_world` sample.

* The TF-M protected storage service is now using non-static labels when deriving encryption keys.
  The derivation labels are generated by combining the client ID of the requesting service and the UID of the resource.
* The TF-M protected storage on nRF9160 is now configured to use AES CCM to perform authenticated encryption and decryption.
* You can now run PSA tests (psa-arch-tests) and TF-M regression tests in the |NCS|.
  The tests can be found as samples in :file:`zephyr/samples/tfm_integration`.

Crypto
------

* :ref:`crypto_samples`:

  * Added samples showcasing the usage of the Platform Security Architecture (PSA) Crypto APIs.
    The samples perform various cryptographic operations such as encryption/decryption using symmetric and asymmetric ciphers, key exchange, hashing, and random number generation.
    Both TF-M enabled targets and secure-only targets are supported.

* :ref:`lib_hw_unique_key` library:

  * New library for managing and using hardware unique keys (HUKs), building on the APIs in nrf_cc3xx_platform.
    HUKs are secret keys that are kept hidden from the application code, but which can be used by the application for deriving keys for other purposes, such as encrypting data for storing.

Edge Impulse
------------

* Added the :ref:`nrf_machine_learning_app` application that integrates the Edge Impulse wrapper library with sensor sampling.
* Sample reference to a public pre-trained model.

Pelion
------

* Added the integration of Pelion Device Management library, available as one of the external submodule repositories in the |NCS|.
  For more information, see Using Pelion with the |NCS|.
* Added the nRF Pelion Client application that showcases the usage of Pelion Device Management library.

Common Application Framework (CAF)
----------------------------------

* Migrated some of the application modules of nRF Desktop application to :ref:`lib_caf` for reuse by other applications.

Hardware flash write protection
-------------------------------

* Fixed an issue where :ref:`fprotect_readme` did not properly add protection on devices with the ACL peripheral, if multiple boot stages were using the flash write protection.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Modem library
+++++++++++++

* Updated :ref:`nrf_modem` to version 1.2.1.
  See the :ref:`nrfxlib:nrf_modem_changelog` for detailed information.
* Added a new function-based GNSS API with support for new GNSS features in modem firmware v1.3.0.
  See :ref:`nrfxlib:gnss_interface` for more information.

  * GNSS socket API is now deprecated.

* PDN socket API is deprecated.
  The functionality has been replaced by the :ref:`pdn_readme` library.

Crypto
++++++

* nrf_security:

  * Added functionality to configure and enable crypto hardware acceleration as part of the TF-M build.
  * Added configurations to enable PSA Crypto APIs in non-TF-M builds.
  * psa_eits: added a library to provide ITS APIs for using the Zephyr settings subsystem for non-volatile storage of key material.
    This library is development quality and the storage format is likely to change without backwards compatibility.

* nrf_cc3xx_platform/nrf_cc3xx_mbedcrypto:

  * Added low-level APIs for managing and using hardware unique keys located in the KMU peripheral, or flash + K_DR, when no KMU is available.
  * Added platform APIs for ``hmac_drbg``.
  * Updated the used Mbed TLS version to 2.26.0 to align with upstream TF-M.
  * For full information, see :ref:`crypto_changelog_nrf_cc3xx_platform` and :ref:`crypto_changelog_nrf_cc3xx_mbedcrypto`.

nRF IEEE 802.15.4 radio driver
++++++++++++++++++++++++++++++

* Added production support for nRF5340 in multiprotocol configuration (IEEE 802.15.4 and Bluetooth Peripheral Role).

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``2fce9769b1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* Added support for indicating serial recovery through LED.
* Made the debounce delay of the serial detect pin state configurable.
* Added support for Mbed TLS ECDSA for signatures.
* Added an option to use GPIO PIN to enter to USB DFU class recovery.
* Added an optional check that prevents attempting to boot an image built for a different ROM address than the slot it currently resides in.
  The check is enabled if the image was signed with the ``IMAGE_F_ROM_FIXED`` flag.

nrfx
====

See the `Changelog for nrfx 2.5.0`_ for detailed information.

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``5c5055f5a``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* Fixed an issue with the file system management failing to open files due to missing initializations of ``fs_file_t`` structures.
* Fixed an issue where multiple SMP commands sent one after the other would corrupt CBOR payload.
* Fixed a problem where mcumgr over shell would stall and wait for retransmissions of frames.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``730acbd6ed`` (``v2.6.0-rc1``), plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline v2.6.0-rc1 ^v2.4.99-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^v2.6.0-rc1

The current |NCS| release is based on Zephyr v2.6.0-rc1.
See the :ref:`zephyr:zephyr_2.6` Release Notes for an overview of the most important changes inherited from upstream Zephyr.

Zephyr Workqueue API Migration
------------------------------

|NCS| v1.6.0 includes changes to the Zephyr Workqueue API introduced as part of `Zephyr pull request #29618`_.
This pull request deprecates part of the current Workqueue API, and introduces new APIs to cover the same usage scenarios.
The new API fixes issues discussed in `Zephyr issue #27356`_.

|NCS| code has been migrated for these changes and it is recommended that all applications migrate to the new ``k_work`` API when upgrading to |NCS| v1.6.0.
All of the deprecated APIs have a corresponding new API that can be used as a drop-in replacement, except :c:func:`k_delayed_work_submit_for_queue` and :c:func:`k_delayed_work_submit`.
These functions have both been split into two functions to cover two different usage scenarios:

* :c:func:`k_work_reschedule` (and :c:func:`k_work_reschedule_for_queue`) behaves the same way as the old :c:func:`k_delayed_work_submit` function, and will resubmit the work item, even if it has already been queued.
* :c:func:`k_work_schedule` (and :c:func:`k_work_schedule_for_queue`) will only submit the work item if it has not yet been queued.

Replacing the deprecated APIs with their new counterparts will fix most of the internal issues observed in the old Workqueue implementation.
However, to avoid the most common pitfalls, you should also make sure they follow the Workqueue best practices, documented under the "Workqueue Best Practices" section of :ref:`zephyr:workqueues_v2`.

The following is a full list of the deprecated Workqueue APIs in |NCS| v1.6.0 and their respective replacements:

.. list-table:: ``k_work`` API replacements
   :header-rows: 1

   * - Deprecated API
     - Corresponding new API
   * - :c:func:`k_work_pending`
     - :c:func:`k_work_is_pending`
   * - :c:func:`k_work_q_start`
     - :c:func:`k_work_queue_start`
   * - :c:func:`k_delayed_work`
     - :c:func:`k_work_delayable`
   * - :c:func:`k_delayed_work_init`
     - :c:func:`k_work_init_delayable`
   * - :c:func:`k_delayed_work_submit_to_queue`
     - :c:func:`k_work_schedule_for_queue` or :c:func:`k_work_reschedule_for_queue`
   * - :c:func:`k_delayed_work_submit`
     - :c:func:`k_work_schedule` or :c:func:`k_work_reschedule`
   * - :c:func:`k_delayed_work_pending`
     - :c:func:`k_work_delayable_is_pending`
   * - :c:func:`k_delayed_work_cancel`
     - :c:func:`k_work_cancel_delayable`
   * - :c:func:`k_delayed_work_remaining_get`
     - :c:func:`k_work_delayable_remaining_get`
   * - :c:func:`k_delayed_work_expires_ticks`
     - :c:func:`k_work_delayable_expires_get`
   * - :c:func:`k_delayed_work_remaining_ticks`
     - :c:func:`k_work_delayable_remaining_get`
   * - :c:macro:`K_DELAYED_WORK_DEFINE`
     - :c:macro:`K_WORK_DELAYABLE_DEFINE`

For more information about the new Workqueue API, refer to :ref:`zephyr:workqueues_v2`.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to and including ``aa96ea0365``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Completed the persistent storage feature, which allows Matter devices to successfully communicate with each other even after reboot.
  * Added support for OpenThread's Service Registration Protocol (SRP) to enable the discovery of Matter nodes using the DNS-SD protocol.
  * Added support for Network Commissioning Cluster, used when provisioning a Matter node.
  * Enabled Message Reliability Protocol (MRP) for the User Datagram Protocol (UDP) traffic within a Matter network.
  * Added support for Operational Credentials Cluster, used to equip a Matter node with an operational certificate.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* Added a new documentation set for nrfx - a standalone set of drivers for peripherals.
* Project CHIP has been renamed to Matter in all occurrences throughout the documentation.
* :ref:`glossary` - Added a new, comprehensive glossary that explains many of the terms used when working with the |NCS|.
* :ref:`doc_build` - Added a new section on dealing with warnings during documentation builds.
* :ref:`gs_updating` - A new section of the Getting Started that explains how to update the west tool and how to use it to update the |NCS| repositories.
* Restructured the bootloader-related guides into a self-contained section covering bootloader architecture, firmware updates, and user guides.

   * Added the following bootloader user guides:

      * :ref:`ug_bootloader_adding`
      * :ref:`ug_bootloader_testing`
      * :ref:`ug_bootloader_external_flash`
      * :ref:`ug_bootloader_config`
      * :ref:`ug_fw_update`

   * :ref:`app_bootloaders` - Bootloader and firmware update guide header page.
   * :ref:`bootloader` - Refactored to include more information about implemented features.
   * :ref:`ug_bootloader` - Updated architecture information for clarifying first- and second-stage bootloader design.
   * "Immutable bootloader" references have been changed to "|NSIB|".

* :ref:`ug_multi_image` - Added more information regarding child image usage, configuration options, and image-specific variables.
* :ref:`partition_manager` - Added section about partition reports.
* :ref:`ug_tfm` - Added references to new crypto samples that utilize TF-M and information about the TF-M minimal build.
* :ref:`ug_thread` - The following sections were added or changed considerably:

   * :ref:`thread_ot_device_types`
   * :ref:`ug_thread_configuring_basic_building`
   * :ref:`ug_thread_tools`

* :ref:`ug_zigbee`:

   * :ref:`zigbee_memory` - Updated the memory values for the latest release.
   * :ref:`ug_zigbee_other_ecosystems` - New page.
   * :ref:`ug_zigbee_tools` - Updated with new content and structure.

* Documentation updates in HomeKit and Find My private repositories.

Applications and samples
------------------------

* nRF9160:

   * nRF9160: Asset Tracker - Added sections on using nRF Cloud A-GPS or P-GPS, and on using nRF Cloud FOTA.
   * :ref:`asset_tracker_v2`:

      * Added a table showing cloud services and the corresponding cloud-side instances.
      * Extended the documentation to include Azure IoT Hub and nRF Cloud support.

   * :ref:`serial_lte_modem` - Added links to AT command reference guides.
   * :ref:`agps_sample` - Added a section on using nRF Cloud A-GPS or P-GPS.
   * nRF9160: AWS FOTA sample - Changes in the sample configuration section.
   * :ref:`fmfu_smp_svr_sample` - Updated the Building and running section.
   * :ref:`gps_with_supl_support_sample` - Changes in the Overview to reflect the introduction of GNSS interface.
   * :ref:`lwm2m_client` - Added information about additional configurations.

* nRF5340:

   * :ref:`nc_bootloader` - Reworked the Overview and Building and running sections.

* Bootloader:

   * :ref:`bootloader` - Sample renamed to |NSIB| and content changed to a large extent.

* Edge Impulse

   * Edge Impulse samples are now in a separate :ref:`edge_impulse_samples` section.

* TF-M

   * :ref:`tfm_hello_world` - Updated the expected sample output and added a reference to the :ref:`lib_tfm_ioctl_api` library, which the sample now uses.

* Thread

   * :ref:`ot_cli_sample` - Updated the procedure for testing Thread 1.2 features.
   * :ref:`coap_client_sample` - Added information on Device Firmware Upgrade extension and its testing procedure.

* Zigbee

   * Added links to ZBOSS API documentation in :ref:`zigbee_light_bulb_sample`, :ref:`zigbee_network_coordinator_sample`, and :ref:`zigbee_light_switch_sample` samples.
   * :ref:`zigbee_ncp_sample` - Updated the nRF5 SDK Bootloader section.

Libraries and drivers
---------------------

* :ref:`bt_mesh` - This overview now points to specifications for general Bluetooth mesh information.
* :ref:`caf_overview` - Added a major description of the library with extensive sections describing all of its aspects.
* :ref:`lte_lc_readme` - Added multiple new sections in this library documentation.
* :ref:`mpsl_assert` - New library documentation.
* :ref:`lib_nrf_cloud` - Added sections on Configuration options for device ID and on Firmware over-the-air (FOTA) updates.

nrfxlib
-------

* :ref:`nrf_802154` - Full documentation of the radio driver is now available as part of nrfxlib and replaces the previous nRF 802.15.4 Service Layer documentation.
* :ref:`nrf_security_readme` - Added section on Building with TF-M.
* Added :ref:`zboss_certification`.
