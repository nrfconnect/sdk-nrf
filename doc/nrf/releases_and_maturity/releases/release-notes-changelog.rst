.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.5.99
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
See `known issues for nRF Connect SDK v2.5.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

Application development
=======================

This section provides detailed lists of changes to overarching SDK systems and components.

Build system
------------

|no_changes_yet_note|

nRF Front-End Modules
---------------------

|no_changes_yet_note|

Working with nRF91 Series
=========================

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

Bluetooth® LE
-------------

* Added host extensions for Nordic-only vendor-specific command APIs.
  Implementation and integration of the host APIs can be found in the :file:`host_extensions.h` header file.

Bluetooth Mesh
--------------

* Updated:

  * :ref:`bt_mesh_dm_srv_readme` and :ref:`bt_mesh_dm_cli_readme` model IDs and opcodes have been updated to avoid conflict with Simple OnOff Server and Client models.

Matter
------

* Updated the page about :ref:`ug_matter_device_low_power_configuration` with the information about Intermittently Connected Devices (ICD) configuration.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.2.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

   * Support for the Intermittently Connected Devices (ICD) Management cluster.
   * The Kconfig options :kconfig:option:`CONFIG_CHIP_ICD_IDLE_MODE_DURATION`, :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_DURATION` and :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC` to manage ICD configuration.
   * New device types:

     * Refridgerator
     * Room air conditioner
     * Dishwasher
     * Laundry washer
     * Robotic vacuum cleaner
     * Smoke CO alarm
     * Air quality sensor
     * Air purifier
     * Fan

   * Product Appearance attribute in the Basic Information cluster that allows describing the product's color and finish.

* Updated:

   * Renamed the ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT`.
   * Renamed the ``CONFIG_CHIP_SED_IDLE_INTERVAL`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`.
   * Renamed the ``CONFIG_CHIP_SED_ACTIVE_INTERVAL`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL`.
   * Renamed the ``CONFIG_CHIP_SED_ACTIVE_THRESHOLD`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD`.

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

HomeKit
-------

HomeKit is now removed, as announced in the :ref:`ncs_release_notes_250`.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Asset Tracker v2
----------------

|no_changes_yet_note|

Serial LTE modem
----------------

* Added:

  * ``#XMQTTCFG`` AT command to configure MQTT client before connecting to the broker.
  * The :ref:`CONFIG_SLM_AUTO_CONNECT <CONFIG_SLM_AUTO_CONNECT>` Kconfig option to support automatic LTE connection at start-up or reset.
  * The :ref:`CONFIG_SLM_CUSTOMER_VERSION <CONFIG_SLM_CUSTOMER_VERSION>` Kconfig option for customers to define their own version string after customization.
  * The optional ``path`` parameter to the ``#XCARRIEREVT`` AT notification.
  * ``#XCARRIERCFG`` AT command to configure the LwM2M carrier library using the LwM2M carrier settings (see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SETTINGS` Kconfig option).

* Updated:

  * ``#XMQTTCON`` AT command to exclude MQTT client ID from the parameter list.
  * ``#XSLMVER`` AT command to report CONFIG_SLM_CUSTOMER_VERSION if it is defined.

* Removed Kconfig options ``CONFIG_SLM_CUSTOMIZED`` and ``CONFIG_SLM_SOCKET_RX_MAX``.

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_dfu` to use :ref:`partition_manager` definitions for determining currently booted image slot in build time.
    The other image slot is used to store an application update image.
  * The :ref:`nrf_desktop_dfu_mcumgr` to use MCUmgr SMP command status callbacks (the :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` Kconfig option) instead of MCUmgr image and OS management callbacks.
  * The dependencies of the :kconfig:option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` Kconfig option.
    The option can be enabled even when the Bluetooth controller is not enabled as part of the application that uses :ref:`nrf_desktop_ble_latency`.
  * The :ref:`nrf_desktop_bootloader` and :ref:`nrf_desktop_bootloader_background_dfu` sections in the nRF Desktop documentation to explicitly mention the supported DFU configurations.
  * The documentation describing the :ref:`nrf_desktop_memory_layout` configuration to simplify the process of getting started with the application.
  * Changed the term *flash memory* to *non-volatile memory* for generalization purposes.
  * The :ref:`nrf_desktop_usb_state` to use the :c:func:`usb_hid_set_proto_code` function to set the HID Boot Interface protocol code.
    The ``CONFIG_USB_HID_PROTOCOL_CODE`` Kconfig option is deprecated and a dedicated API needs to be used instead.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Matter Bridge
-------------

* Added:

  * Support for groupcast communication to the On/Off Light device implementation.
  * Support for controlling the OnOff Light simulated data provider by using shell commands.
  * Support for Matter Generic Switch bridged device type.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* :ref:`ble_throughput` sample:

  * Enabled encryption in the sample.
    The measured throughput is calculated over the encrypted data, which is how most of the Bluetooth products use this protocol.

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Cellular samples (renamed from nRF9160 samples)
-----------------------------------------------

* :ref:`modem_shell_application` sample:

  * Added:

    * Printing of the last reset reason when the sample starts.
    * Support for printing the sample version information using the ``version`` command.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed:

    * The sample now waits for a successful connection before printing ``Connected to nRF Cloud!``.
    * Building for the Thingy:91.
    * The PSM Requested Active Time is now reduced from 1 minute to 20 seconds.
      The old value was too long for PSM to activate.

  * Changed:

    * The sample now explicitly uses the :c:func:`conn_mgr_all_if_connect` function to start network connectivity, instead of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START` and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT` Kconfig options.
    * The sample to use the FOTA support functions in the :file:`nrf_cloud_fota_poll.c` and :file:`nrf_cloud_fota_common.c` files.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added credential check before connecting to network.
  * Changed the sample use the functions in the :file:`nrf_cloud_fota_poll.c` and :file:`nrf_cloud_fota_common.c` files.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added credential check before connecting to network.

* :ref:`nrf_provisioning_sample` sample:

  * Added event handling for events from device mode callback.

Cryptography samples
--------------------

* Updated:

  * All crypto samples to use ``psa_key_id_t`` instead of ``psa_key_handle_t``.
    These concepts have been merged and ``psa_key_handle_t`` is removed from the PSA API specification.

Debug samples
-------------

|no_changes_yet_note|

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* :ref:`matter_lock_sample` sample:

  * Fixed an issue that prevented nRF Toolbox for iOS in version 5.0.9 from controlling the sample using :ref:`nus_service_readme`.

Multicore samples
-----------------

|no_changes_yet_note|

Networking samples
------------------

* :ref:`nrf_coap_client_sample` sample:

  * Added support for Wi-Fi and LTE connectivity through the connection manager API.
  * Updated by moving the sample from :file:`cellular/coap_client` folder to :file:`net/coap_client`.
    The documentation is now found in the :ref:`networking_samples` section.

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * The "start_tx_modulated_carrier" command, when used without an additional parameter, does not enable the radio end interrupt.

PMIC samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

* Changed building method to use :ref:`zephyr:snippets` for predefined configuration.

* In the :ref:`thread_ug_feature_sets` provided as part of the |NCS|, the following features have been removed from the FTD and MTD variants:

  * ``DHCP6_CLIENT``
  * ``JOINER``
  * ``SNTP_CLIENT``
  * ``LINK_METRICS_INITIATOR``

  All mentioned features are still available in the master variant.

Sensor samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* Added the :ref:`wifi_throughput_sample` sample that demonstrates how to measure the network throughput of a Nordic Wi-Fi enabled platform under the different Wi-Fi stack configuration profiles.

Other samples
-------------

* :ref:`radio_test` sample:

  * Corrected the way of setting the TX power with FEM.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* OS agnostic code is moved to |NCS| (``sdk-nrfxlib``) repository.

  * Low-level API documentation is now available on the :ref:`Wi-Fi driver API <nrfxlib:nrf_wifi_api>`.

* Added TX injection feature to the nRF70 Series device.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Updated:

    * Improved the :ref:`bt_fast_pair_readme` library documentation to include the description of the missing Kconfig options.

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`lib_location` library:

  * Updated:

    * The use of neighbor cell measurements for cellular positioning.
      Previously, 1-2 searches were performed and now 1-3 will be done depending on the requested number of cells and the number of found cells.
      Also, only GCI cells are counted towards the requested number of cells, and normal neighbors are ignored from this perspective.
    * Cellular positioning not to use GCI search when the device is in RRC connected mode, because the modem cannot search for GCI cells in that mode.

* :ref:`lte_lc_readme` library:

  * Added:

    * The :c:func:`lte_lc_psm_param_set_seconds` function and Kconfig options :kconfig:option:`CONFIG_LTE_PSM_REQ_FORMAT`, :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU_SECONDS`, and :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT_SECONDS` to enable setting of PSM parameters in seconds instead of using bit field strings.

  * Updated:

    * The default network mode to :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` from :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_GPS`.
    * The default LTE mode preference to :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE_LTE_M_PLMN_PRIO` from :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE_AUTO`.
    * The :kconfig:option:`CONFIG_LTE_NETWORK_USE_FALLBACK` Kconfig option is deprecated.
      Use the :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT` or :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` Kconfig option instead.
      In addition, you can control the priority between LTE-M and NB-IoT using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.
    * The :c:func:`lte_lc_init` function is deprecated.
    * The :c:func:`lte_lc_deinit` function is deprecated.
      Use the :c:func:`lte_lc_power_off` function instead.
    * The :c:func:`lte_lc_init_and_connect` function is deprecated.
      Use the :c:func:`lte_lc_connect` function instead.
    * The :c:func:`lte_lc_init_and_connect_async` function is deprecated.
      Use the :c:func:`lte_lc_connect_async` function instead.

  * Removed the deprecated Kconfig option ``CONFIG_LTE_AUTO_INIT_AND_CONNECT``.

* :ref:`nrf_modem_lib_readme`:

  * Added:

    * A mention about enabling TF-M logging while using modem traces in the :ref:`modem_trace_module`.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_DOWN_DEFAULT_LTE_DISCONNECT` option, allowing the user to change the behavior of the driver's :c:func:`net_if_down` implementation at build time.

  * Updated by renaming ``lte_connectivity`` module to ``lte_net_if``.
    All related Kconfig options have been renamed accordingly.
  * Changed the default value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START`, :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT`, and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_DOWN` Kconfig options from enabled to disabled.

  * Removed:

    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_SYS_INIT``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE``.
    * The ``NRF_MODEM_LIB_NET_IF_DOWN`` flag support in the ``lte_net_if`` network interface driver.

* :ref:`lib_modem_slm`:

    * Changed the GPIO used to be configurable using devicetree.

* :ref:`pdn_readme` library:

   * Fixed a potential issue where the library tries to free the PDN context twice, causing the application to crash.
   * Updated the library to add PDP auto configuration to the :c:enumerator:`LTE_LC_FUNC_MODE_POWER_OFF` event.

Libraries for networking
------------------------

* :ref:`lib_aws_iot` library:

  * Added library tests.
  * Updated the library to use the :ref:`lib_mqtt_helper` library.
    This simplifies the handling of the MQTT stack.

* :ref:`lib_nrf_cloud_coap` library:

  * Added:

    * Automatic selection of proprietary PSM mode when building for the SOC_NRF9161_LACA.
    * Support for bulk transfers to the :c:func:`nrf_cloud_coap_json_message_send` function.

* :ref:`lib_nrf_cloud_log` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOG_INCLUDE_LEVEL_0` Kconfig option.
    * Support for nRF Cloud CoAP text mode logging.

* :ref:`lib_nrf_cloud` library:

  * Added the :c:func:`nrf_cloud_credentials_configured_check` function to check if credentials exist based on the application's configuration.

* :ref:`lib_nrf_provisioning` library:

  * Updated the device mode callback to send an event when the provisioning state changes.

* :ref:`lib_nrf_cloud_fota` library:

  * Added the :file:`nrf_cloud_fota_poll.c` file to consolidate the FOTA polling code from the :ref:`nrf_cloud_multi_service` and :ref:`nrf_cloud_rest_fota` samples.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF Security
------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_state`:

  * Updated the dependencies of the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` Kconfig option.
    The option can be enabled even when the Bluetooth controller is not enabled as part of the application that uses :ref:`caf_ble_state`.

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`nrf_desktop_config_channel_script`:

  * Separated functions that are specific to handling the :file:`dfu_application.zip` file format.
    The ZIP format is used for update images in the nRF Connect SDK.
    The change simplifies integrating new update image file formats.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``11ecbf639d826c084973beed709a63d51d9b684e``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``a768a05e6205e415564226543cee67559d15b736``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline a768a05e62 ^4bbd91a908

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^a768a05e62

The current |NCS| main branch is based on revision ``a768a05e62`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

* The minimal TF-M build profile no longer silences TF-M logs by default.

  .. note::
     This can be a breaking change if the UART instance used by TF-M is already in use, for example by modem trace with a UART backend.

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added

  * A page on :ref:`ug_nrf70_developing_debugging` in the :ref:`ug_nrf70_developing` user guide.
  * A page on :ref:`ug_nrf70_developing_fw_patch_ext_flash` in the :ref:`ug_nrf70_developing` user guide.

  * A page on :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission` in the :ref:`ug_nrf70_developing` user guide.

* Updated:

  * The :ref:`installation` section by replacing two separate pages about installing the |NCS| with just one (:ref:`install_ncs`).
  * The :ref:`requirements` page with new sections about :ref:`requirements_clt` and :ref:`toolchain_management_tools`.
  * The :ref:`configuration_and_build` section:

    * :ref:`app_build_system` gathers conceptual information about the build and configuration system previously listed on several other pages.
    * New reference page :ref:`app_build_output_files` gathers information previously listed on several other pages.

  * The :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages so that instructions in the :ref:`nrf9160_gs_connecting_dk_to_cloud` and :ref:`thingy91_connect_to_cloud` sections, respectively, match the updated nRF Cloud workflow.
  * The :ref:`ug_nrf9160_gs` by replacing the Updating the DK firmware section with a new :ref:`nrf9160_gs_installing_software` section.
    This new section includes steps for using Quick Start, a new application in `nRF Connect for Desktop`_ that streamlines the getting started process with the nRF91 Series DKs.
  * The :ref:`tfm_enable_share_uart` section on :ref:`ug_nrf9160`.
  * Integration steps in the :ref:`ug_bt_fast_pair` guide.
    Reorganized extension-specific content into dedicated subsections.
  * The :ref:`ug_nrf70_developing_powersave_power_save_mode` section in the :ref:`ug_nrf70_developing_powersave` user guide.

* Removed the Welcome to the |NCS| page.
  This page is replaced with existing :ref:`ncs_introduction` page.
