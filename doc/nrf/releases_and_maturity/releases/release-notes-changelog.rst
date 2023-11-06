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

|no_changes_yet_note|

Bluetooth mesh
--------------

|no_changes_yet_note|

Matter
------

* Updated:

  * Page about :ref:`ug_matter_device_low_power_configuration` with the information about Intermittently Connected Devices (ICD) configuration.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.2.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

   * Support for the Intermittently Connected Devices (ICD) Management cluster.
   * The Kconfig options :kconfig:option:`CONFIG_CHIP_ICD_IDLE_MODE_DURATION`, :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_DURATION` and :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC` to manage ICD configuration.
   * New device types:

     * Refidgerator
     * Room Air Conditioner
     * Dishwasher
     * Laundry Washer
     * Robotic Vacuum Cleaner
     * Smoke CO Alarm
     * Air Quality Sensor
     * Air Purifier
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

* Updated:

  * ``#XMQTTCON`` AT command to exclude MQTT client ID from the parameter list.

nRF5340 Audio
-------------


|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

nRF Desktop
-----------

|no_changes_yet_note|

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Matter Bridge
-------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

|no_changes_yet_note|

* :ref:`ble_throughput` sample:

  * Enabled encryption in the sample.
    The measured throughput is calculated over the encrypted data, which is how most of the Bluetooth products use this protocol.

Bluetooth mesh samples
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

  * Changed:

    * The sample now explicitly uses :c:func:`conn_mgr_all_if_connect` to start network connectivity, instead of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START` and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT` Kconfig options.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added credential check before connecting to network.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added credential check before connecting to network.

Cryptography samples
--------------------

* Updated:

  * All crypto samples to use ``psa_key_id_t`` instead of ``psa_key_handle_t``.
    These concepts have been merged and ``psa_key_handle_t`` is removed from the PSA API specification.

|no_changes_yet_note|

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

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

* Changed building method to use :ref:`zephyr:snippets` for predefined configuration.

Sensor samples
--------------

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* Added the :ref:`wifi_throughput_sample` sample that demonstrates how to measure the network throughput of a Nordic Wi-Fi enabled platform under the different Wi-Fi stack configuration profiles.

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Wi-Fi drivers
-------------

* OS agnostic code is moved to |NCS| (``sdk-nrfxlib``) repository.

   - Low-level API documentation is now available on the :ref:`Wi-Fi driver API <nrfxlib:nrf_wifi_api>`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

|no_changes_yet_note|

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

* :ref:`lte_lc_readme`:

  * Added the :c:func:`lte_lc_psm_param_set_seconds` function and Kconfig options :kconfig:option:`LTE_PSM_REQ_FORMAT`, :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU_SECONDS`, and :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT_SECONDS` to enable setting of PSM parameters in seconds instead of using bit field strings.

* :ref:`nrf_modem_lib_readme`:

   * Added a mention about enabling TF-M logging while using modem traces in the :ref:`modem_trace_module`.
   * Updated by renaming ``lte_connectivity`` module to ``lte_net_if``.
     All related Kconfig options have been renamed accordingly.
   * Changed the default value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START`, :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT`, and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_DOWN` Kconfig options from enabled to disabled.

  * Removed:

    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_SYS_INIT``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE``.

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

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

|no_changes_yet_note|

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

* :ref:`ug_nrf9160`:

   *  Section :ref:tfm_enable_share_uart in the :ref:uge_nrf9160 page.

* Updated:

  * The :ref:`installation` section: by replacing two separate pages about installing the |NCS| with just one (:ref:`install_ncs`).
  * The :ref:`requirements` page with new sections about :ref:`requirements_clt` and :ref:`toolchain_management_tools`.
  * The :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages  so that instructions in the :ref:`nrf9160_gs_connecting_dk_to_cloud` and :ref:`thingy91_connect_to_cloud` sections, respectively, match the updated nRF Cloud workflow.
