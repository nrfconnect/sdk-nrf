.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.3.99
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
   Add the sections you need, as only a handful of sections is kept when the changelog is cleaned.
   "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.3.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

MCUboot
=======

|no_changes_yet_note|

Application development
=======================

|no_changes_yet_note|

RF Front-End Modules
--------------------

|no_changes_yet_note|

Build system
------------

|no_changes_yet_note|

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

|no_changes_yet_note|

Bluetooth mesh
--------------

* Updated the default configuration of advertising sets used by the Bluetooth mesh subsystem to improve performance of the Relay, GATT and Friend features.
  This configuration is specified in the :file:`ncs/nrf/subsys/bluetooth/mesh/Kconfig` file.

See `Bluetooth mesh samples`_ for the list of changes in the Bluetooth mesh samples.

Matter
------

* Added:

  * Support for the :ref:`ug_matter_configuring_optional_persistent_subscriptions` feature.
  * The Matter Nordic UART Service (NUS) feature to the :ref:`matter_lock_sample` sample.
    This feature allows using Nordic UART Service to control the device remotely through Bluetooth LE and adding custom text commands to a Matter sample.
    The Matter NUS implementation allows controlling the device regardless of whether the device is connected to a Matter network or not.
    The feature is dedicated for the Matter over Thread solution.
  * Documentation page about :ref:`ug_matter_device_configuring_cd`.

* Updated:

  * The :ref:`ug_matter` protocol page with a table that lists compatibility versions for the |NCS|, the Matter SDK, and the Matter specification.
  * The :ref:`ug_matter_tools` page with installation instructions for the ZAP tool, moved from the :ref:`ug_matter_creating_accessory` page.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``SVE RC2`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Updated the factory data generation script with the feature for generating the onboarding code.
  You can now use the factory data script to generate a manual pairing code and a QR Code that are required to commission a Matter-enabled device over Bluetooth LE.
  Generated onboarding codes should be put on the device's package or on the device itself.
  For details, see the Generating onboarding codes section on the :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.
* Introduced ``SLEEPY_ACTIVE_THRESHOLD`` parameter that makes the Matter sleepy device stay awake for a specified amount of time after network activity.

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Support for bigger payload size.
    ESB supports a payload with a size of 64 bytes or more.
  * The `use_fast_ramp_up` feature that reduces radio ramp-up delay from 130 µs to 40 µs.
  * The :kconfig:option:`CONFIG_ESB_NEVER_DISABLE_TX` Kconfig option as an experimental feature that enables the radio peripheral to remain in TXIDLE state instead of TXDISABLE when transmission is pending.

* Updated:

  * The number of PPI/DPPI channels used from three to six.
  * Events 6 and 7 from the EGU0 instance by assigning them to the ESB module.
  * The type parameter of the :c:func:`esb_set_tx_power` function to ``int8_t``.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added the integration of the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.

* Updated:

  * The application now uses the function :c:func:`nrf_cloud_location_request_msg_json_encode` to create an nRF Cloud location request message.
  * The application now uses defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * AT command ``#XWIFIPOS`` to get Wi-Fi location from nRF Cloud.
  * Support for *WRITE REQUEST* in TFTP client.

* Updated:

  * Use defines from the :ref:`lib_nrf_cloud` library for nRF Cloud related string values.

* Fixed:

  * A bug in receiving large MQTT Publish message.

nRF5340 Audio
-------------

* Moved the LE Audio controller for the network core to the standalone :ref:`lib_bt_ll_acs_nrf53_readme` library.

* Added Kconfig options for setting periodic and extended advertising intervals.
  Search :ref:`Kconfig Reference <kconfig-search>` for ``BLE_ACL_PER_ADV_INT_`` and ``BLE_ACL_EXT_ADV_INT_`` to list all of them.

* Implemented :ref:`zephyr:zbus` for handling events from buttons and LE Audio.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the machine learning models (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) used by the application to ensure compatibility with the new Zephyr version.
* Simplified the over-the-air (OTA) device firmware update (DFU) configuration of nRF53 DK .
  The configuration relies on the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

nRF Desktop
-----------

* Added:

  * The :ref:`nrf_desktop_swift_pair_app`.
    The module is used to enable or disable the Swift Pair Bluetooth advertising payload depending on the selected Bluetooth peer (used local identity).
  * An application-specific string representing device generation (:ref:`CONFIG_DESKTOP_DEVICE_GENERATION <config_desktop_app_options>`).
    The generation allows to distinguish configurations that use the same board and bootloader, but are not interoperable.
    The value can be read through the :ref:`nrf_desktop_config_channel`.
    On the firmware side, fetching the values is handled by the :ref:`nrf_desktop_dfu`.
  * Unpairing old peers right after a successful erase advertising procedure.
    This prevents blocking the bond slots until the subsequent erase advertising procedure is triggered.

* Updated:

  * The :ref:`nrf_desktop_dfu` automatically enables 8-bit write block size emulation (:kconfig:option:`CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS`) to ensure that update images with sizes not aligned to word size can be successfully stored in the internal flash.
    The feature is not enabled if the MCUboot bootloader is used and the secondary slot is placed in an external flash (when :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY` is enabled).
  * The :ref:`nrf_desktop_ble_latency` uses low latency for the active Bluetooth connection in case of the SMP transfer event and regardless of the event submitter module.
    Previously, the module lowered the connection latency only for SMP events submitted by the :ref:`caf_ble_smp`.
  * In the Fast Pair configurations, the bond erase operation is enabled for the dongle peer, which will let you change the bonded Bluetooth Central.
  * The `Swift Pair`_ payload is, by default, included for all of the Bluetooth local identities apart from the dedicated local identity used for connection with an nRF Desktop dongle.
    If a configuration supports both Fast Pair and a dedicated dongle peer (:ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>`), the `Swift Pair`_ payload is, by default, included only for the dongle peer.

Samples
=======

Bluetooth samples
-----------------

* :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples register HID Service before Bluetooth is enabled (before calling the :c:func:`bt_enable` function).
  The :c:func:`bt_gatt_service_register` function can no longer be called after enabling Bluetooth and before loading settings.

* Removed the Bluetooth: External radio coexistence using 3-wire interface sample because of the removal of the 3-wire implementation.

* :ref:`peripheral_hids_mouse` sample:

  * The :kconfig:option:`CONFIG_BT_SMP` Kconfig option is included when ``CONFIG_BT_HIDS_SECURITY_ENABLED`` is selected.
  * Fixed a CMake warning by moving the nRF RPC configuration (the :kconfig:option:`CONFIG_NRF_RPC_THREAD_STACK_SIZE` Kconfig option) to a separate overlay config file.

* :ref:`direct_test_mode` sample:

  * Added:

    * Support for the :ref:`nrfxlib:mpsl_fem` Tx power split feature.
      The DTM command ``0x09`` for setting the transmitter power level takes into account the front-end module gain when this sample is built with support for front-end modules.
      The vendor-specific commands for setting the SoC output power and the front-end module gain are not available when the :kconfig:option:`CONFIG_DTM_POWER_CONTROL_AUTOMATIC` Kconfig option is enabled.
    * Support for +1 dBm, +2 dBm, and +3 dBm output power on the nRF5340 DK.

  * Changed the handling of the hardware erratas.

  * Removed a compilation warning when used with minimal pinout Skyworks FEM.

* :ref:`peripheral_uart` sample:

  * Fixed the unit of the :kconfig:option:`CONFIG_BT_NUS_UART_RX_WAIT_TIME` Kconfig option to comply with the UART API.

Bluetooth mesh samples
----------------------

* Updated the configuration of advertising sets in all samples to match the new default values.
  See `Bluetooth mesh`_ for more information.
* Removed the :file:`hci_rpmsg.conf` file from all samples that support nRF5340 DK or Thingy:53.
  This configuration is moved to the :file:`ncs/nrf/subsys/bluetooth/mesh/hci_rpmsg_child_image_overlay.conf` file.

nRF9160 samples
---------------

* :ref:`modem_shell_application` sample:

  * Updated the sample to use defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.
    Removed the inclusion of the file :file:`nrf_cloud_codec.h`.
  * Added sending of GNSS data to carrier library when the library is enabled.

* :ref:`https_client` sample:

  * Added IPv6 support and wait time for PDN to fully activate (including IPv6, if available) before looking up the address.

* :ref:`slm_shell_sample` sample:

  * Added support for the nRF7002 DK PCA10143.

* :ref:`lwm2m_client` sample:

  * Added:

    * Integration of the connection pre-evaluation functionality using the :ref:`lib_lwm2m_client_utils` library.

  * Updated:

    * The sample now integrates the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.

* :ref:`pdn_sample` sample:

  * Updated the sample to show how to get interface address information using the :c:func:`nrf_getifaddrs` function.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Updated:

    * Increased the MCUboot partition size to the minimum necessary to allow bootloader FOTA.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added:

    * Overlays to use RTT instead of UART for testing purposes.

  * Updated:

    * The Hello World message sent to nRF Cloud now contains a timestamp (message ID).

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`provisioning_image` sample:

  * Thet network core logic is now moved to the new sample :ref:`provisioning_image_net_core` instead of being a Zephyr module..

Thread samples
--------------

|no_changes_yet_note|

Matter samples
--------------

* Updated the default settings partition size for all Matter samples from 16 kB to 32 kB.

  .. caution::
      This change can affect the Device Firmware Update (DFU) from the older firmware versions that were using the 16-kB settings size.
      Read more about this in the :ref:`ug_matter_device_bootloader_partition_layout` section of the Matter documentation.
      You can still perform DFU from the older firmware version to the latest firmware version, but you will have to change the default settings size from 32 kB to the value used in the older version.

* :ref:`matter_lock_sample` sample:

  * Added the Matter Nordic UART Service (NUS) feature, which allows controlling the door lock device remotely through Bluetooth LE using two simple commands: ``Lock`` and ``Unlock``.
    This feature is dedicated for the nRF52840 and the nRF5340 DKs.

NFC samples
-----------

|no_changes_yet_note|

Multicore samples
-----------------

* :ref:`multicore_hello_world` sample:

  * Added :ref:`zephyr:sysbuild` support to the sample.

nRF5340 samples
---------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

* :ref:`ei_wrapper_sample` sample:

  * Updated the machine learning model (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) to ensure compatibility with the new Zephyr version.

* :ref:`radio_test` sample:

  * Added:

    * A workaround for the hardware `Errata 254`_ of the nRF52840 chip.
    * A workaround for the hardware `Errata 255`_ of the nRF52833 chip.
    * A workaround for the hardware `Errata 256`_ of the nRF52820 chip.
    * A workaround for the hardware `Errata 257`_ of the nRF52811 chip.
    * A workaround for the hardware `Errata 117`_ of the nRF5340 chip.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added :ref:`nrf700x_wifi`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* Added the standalone :ref:`lib_bt_ll_acs_nrf53_readme` library, originally a part of the :ref:`nrf53_audio_app` application.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_le_adv_prov_readme` library:

  * Added API to enable or disable the Swift Pair provider (:c:func:`bt_le_adv_prov_swift_pair_enable`).

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * The :c:func:`bt_fast_pair_info_cb_register` function and the :c:struct:`bt_fast_pair_info_cb` structure to register Fast Pair information callbacks.
      The :c:member:`bt_fast_pair_info_cb.account_key_written` callback can be used to notify the application about the Account Key writes.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_USER_RESET_ACTION` Kconfig option to enable a custom user reset action that executes together with the Fast Pair factory reset operation triggered by the :c:func:`bt_fast_pair_factory_reset` function.

  * Updated:

    * Salt size in the Fast Pair not discoverable advertising from 1 byte to 2 bytes, to align with the Fast Pair specification update.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON` Kconfig option is now the default Fast Pair cryptographic backend.


Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The function :c:func:`nrf_modem_lib_fault_strerror` to retrieve a statically allocated textual description of a given modem fault.
      The function can be enabled using the new Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_FAULT_STRERROR`.
    * The :c:func:`nrf_modem_lib_bootloader_init` function to initialize the Modem library in bootloader mode.

  * Updated:

    * The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_PRIO_OVERRIDE` is now deprecated.
    * The :c:func:`nrf_modem_lib_init` function is now initializing the Modem library in normal operating mode only and the ``mode`` parameter is removed from the input parameters.
      Use the :c:func:`nrf_modem_lib_bootloader_init` function to initialize the Modem library in bootloader mode.
    * The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_SYS_INIT` is now deprecated.
      The application initializes the modem library using the :c:func:`nrf_modem_lib_init` function instead.

  * Removed:

    * The deprecated function ``nrf_modem_lib_get_init_ret``.
    * The deprecated function ``nrf_modem_lib_shutdown_wait``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_TRACE_ENABLED``.

* :ref:`pdn_readme` library:

  * Updated the library to use ePCO mode if the Kconfig option :kconfig:option:`CONFIG_PDN_LEGACY_PCO` is not enabled.

* :ref:`lte_lc_readme` library:

  * Updated the library to handle notifications from the modem when eDRX is not used by the current cell.
    The application now receives an :c:enum:`LTE_LC_EVT_EDRX_UPDATE` event with the network mode set to :c:enum:`LTE_LC_LTE_MODE_NONE` in these cases.
    Modem firmware version v1.3.4 or newer is required to receive these events.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud` library:

  * Added:

    * A public header file :file:`nrf_cloud_defs.h` that contains common defines for interacting with nRF Cloud and the :ref:`lib_nrf_cloud` library.
    * A new event :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR` to indicate an error while the transport connection is being established when the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled.
      Earlier this was indicated with a second :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECTING` event with an error status.
    * A public header file :file:`nrf_cloud_codec.h` that contains encoding and decoding functions for nRF Cloud data.
    * Defines to enable parameters to be omitted from a P-GPS request.

  * Removed unused internal codec function ``nrf_cloud_format_single_cell_pos_req_json()``.

  * Updated:

    * The :c:func:`nrf_cloud_device_status_msg_encode` function now includes the service info when encoding the device status.
    * Renamed files :file:`nrf_cloud_codec.h` and :file:`nrf_cloud_codec.c` to :file:`nrf_cloud_codec_internal.h` and :file:`nrf_cloud_codec_internal.c` respectively.
    * Standardized encode and decode function names in the codec.
    * Moved the :c:func:`nrf_cloud_location_request_json_get` function from the :file:`nrf_cloud_location.h` file to :file:`nrf_cloud_codec.h`.
      The function is now renamed to :c:func:`nrf_cloud_location_request_msg_json_encode`.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated:

    * The mask angle parameter can now be omitted from an A-GPS REST request by using the value ``NRF_CLOUD_AGPS_MASK_ANGLE_NONE``.
    * Use defines from the :file:`nrf_cloud_pgps.h` file for omitting parameters from a P-GPS request.
      Removed the following values: ``NRF_CLOUD_REST_PGPS_REQ_NO_COUNT``, ``NRF_CLOUD_REST_PGPS_REQ_NO_INTERVAL``, ``NRF_CLOUD_REST_PGPS_REQ_NO_GPS_DAY``, and ``NRF_CLOUD_REST_PGPS_REQ_NO_GPS_TOD``.

* :ref:`lib_lwm2m_client_utils` library:

  * Added:

    * Support for the connection pre-evaluation feature using the Kconfig option :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LTE_CONNEVAL`.

  * Updated:

    * :file:`lwm2m_client_utils.h` includes new API for FOTA to register application callback to receive state changes and requests for the update process.

  * Removed the old API ``lwm2m_firmware_get_update_state_cb()``.

* :ref:`pdn_readme` library:

  * Added:

    * ``PDN_EVENT_NETWORK_DETACH`` event to indicate a full network detach.

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

* :ref:`dk_buttons_and_leds_readme` library:

  * The library now supports using the GPIO expander for the buttons, switches, and LEDs on the nRF9160 DK.

Common Application Framework (CAF)
----------------------------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

DFU libraries
-------------

|no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`partition_manager`:

  * Fixed an issue that prevents an empty gap after a static partition for a region with the ``START_TO_END`` strategy.

* :ref:`nrf_desktop_config_channel_script`:

  * Added support for the device information (``devinfo``) option fetching.
    The option provides device's Vendor ID, Product ID and generation.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``59624334748129cb93f096408911a227b0dd64c0``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Support for the downgrade prevention feature using hardware security counters (:kconfig:option:`CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`).
  * Generation of a new variant of the :file:`dfu_application.zip` when the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option is enabled.
    Mentioned archive now contains images for both slots, primary and secondary.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``6d9adf2e8af476819fce802d326a02226d98ed0c``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 6d9adf2e8a ^e1e06d05fa

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^6d9adf2e8a

The current |NCS| main branch is based on revision ``6d9adf2e8a`` of Zephyr.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

Updated:

  * The :ref:`software_maturity` page with details about Bluetooth feature support.
  * The :ref:`ug_nrf5340_gs`, :ref:`ug_thingy53_gs`, :ref:`ug_nrf52_gs`, and :ref:`ug_ble_controller` pages with a link to the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.
  * The :ref:`zigbee_weather_station_app` documentation to match the application template.
