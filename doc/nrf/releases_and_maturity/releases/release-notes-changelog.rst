.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.8.99
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
See `known issues for nRF Connect SDK v2.8.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

|no_changes_yet_note|

Board support
=============

* Updated various tests and samples to use Zephyr's :ref:`native_sim<zephyr:native_sim>` instead of Zephyr's :ref:`native_posix<zephyr:native_posix>` for :ref:`running_unit_tests`.
  This mirrors the deprecation of ``native_posix`` in Zephyr.
  Support for ``native_posix`` will be removed in Zephyr with the v4.2 release.
  In the |NCS|, it will be removed once Zephyr v4.2 is upmerged to sdk-nrf.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

* Updated the allowed offset for :ref:`doc_fw_info` in the :ref:`bootloader`.
  It can now be placed at offset ``0x600``, however, it cannot be used for any applications with |NSIB| compiled before this change.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Working with nRF54H Series
==========================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

|no_changes_yet_note|

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

* Deprecated the explicit use of ``-DSHIELD=nrf21540ek_fwd`` for boards with ``nrf5340/cpuapp`` qualifiers when the nRF21540 EK shield is used.
  The build system uses an appropriate overlay file for each core, relying only on the ``-DSHIELD=nrf21540ek`` parameter.

Developing with PMICs
=====================

|no_changes_yet_note|

Security
========

|no_changes_yet_note|

Protocols
=========

|no_changes_yet_note|

Amazon Sidewalk
---------------

|no_changes_yet_note|

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth Mesh
--------------

|no_changes_yet_note|

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Matter
------

* Added implementation of the :cpp:class:`Spake2pVerifier` class for the PSA crypto backend.
  You can use this class to generate the Spake2+ verifier at runtime.
  To use this class, enable the :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC` and :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE` Kconfig options.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.4.0.0`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Enhanced Network Infrastructure with Home Routers and Access Points (HRAP).
    This provides requirements for devices such as home routers, modems or access points to create a necessary infrastructure for Matter products.
  * Enhanced Multi-Admin that aims to simplify the smart home management from the user perspective.
    This term includes several different features and in this release only Fabric Synchronization was fully delivered.
    The Fabric Synchronization enables commissioning of devices from one fabric to another without requiring manual user actions, only user consent.
  * Dynamic SIT LIT switching support that allows the application to switch between these modes, as long as the requirements for these modes are met.
    This can be enabled using the :kconfig:option:`CONFIG_CHIP_ICD_DSLS_SUPPORT` Kconfig option.
  * The Kconfig option :kconfig:option:`CONFIG_CHIP_ICD_SIT_SLOW_POLL_LIMIT` to limit the slow polling interval value for the device while it is in the SIT mode.
    This can be used to limit the slow poll interval for the ICD LIT device while it is temporarily working in the SIT mode.
  * New device types:

    * Water heater
    * Solar power
    * Battery storage
    * Heat pump
    * Mounted on/off control
    * Mounted dimmable load control

* Updated:

  * Thermostat cluster with support for scheduling and preset modes, like vacation, and home or away settings.
  * Electric Vehicle Supply Equipment (EVSE) with support for user-defined charging preferences, like specifying the time when the car will be charged.
  * Occupancy sensing cluster with features like radar, vision, and ambient sensing.
  * Intermittently Connected Devices feature with enhancements for the Long Idle Time (LIT) and Check-In Protocol.
    With this enhancements, the state of this feature is changed from provisional to certifiable.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

* Added Kconfig options for configuring the MLE child update timeout, child supervision interval, and child supervision check timeout.

Zigbee
------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Machine learning
----------------

|no_changes_yet_note|

Asset Tracker v2
----------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

* Updated the handling of USB CDC ACM baud rate requests to make sure the baud rate is set correctly when the host requests a change.
  This fixes an issue when using GNU screen with the Thingy:91 X.

IPC radio firmware
------------------

|no_changes_yet_note|

Matter Bridge
-------------

|no_changes_yet_note|

nRF5340 Audio
-------------

* Updated the documentation for :ref:`nrf53_audio_app_building` with cross-links and additional information, based on user feedback.

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_settings_loader` to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology.
    As a result, all :ref:`zephyr:nrf54h20dk_nrf54h20` configurations were migrated from the NVS settings backend to the ZMS settings backend.
  * The :ref:`zephyr:nrf54h20dk_nrf54h20` release configuration to enable the :ref:`nrf_desktop_watchdog`.
  * The configuration files of the :ref:`nrf_desktop_click_detector` (:file:`click_detector_def.h`) to allow using them also when Bluetooth LE peer control using a dedicated button (:ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>`) is disabled.
  * The DTS description for board targets with a different DTS overlay file for each build type to isolate the common configuration that is now defined in the :file:`app_common.dtsi` file.
    The following board configurations have been updated:

    * :ref:`zephyr:nrf52840dk_nrf52840`
    * :ref:`zephyr:nrf52840dongle_nrf52840`
    * :ref:`zephyr:nrf54l15dk_nrf54l15`
    * :ref:`zephyr:nrf54h20dk_nrf54h20`

* Removed imply for partial erase feature of the nRF SoC flash driver (:kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE`) for the USB next stack (:ref:`CONFIG_DESKTOP_USB_STACK_NEXT <config_desktop_app_options>`).
  The partial erase feature was used as a workaround for device errors that might be reported by the Windows USB host in Device Manager if a USB cable is connected while erasing a secondary image slot in the background.
  The workaround is no longer needed after the nRF UDC driver was improved.

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

|no_changes_yet_note|

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Amazon Sidewalk samples
-----------------------

|no_changes_yet_note|

Bluetooth samples
-----------------

* Added:

  * The :ref:`channel_sounding_ras_reflector` sample demonstrating how to implement a Channel Sounding Reflector that exposes the Ranging Responder GATT Service.
  * The :ref:`channel_sounding_ras_initiator` sample demonstrating basic distance estimation with Channel Sounding by setting up a Channel Sounding Initiator that acts as a Ranging Requestor GATT Client.
  * Support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in the following sample:

    * :ref:`peripheral_hids_mouse`

* Updated:

  * Configurations of the following Bluetooth samples to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology:

      * :ref:`bluetooth_central_hids`
      * :ref:`peripheral_hids_keyboard`
      * :ref:`peripheral_hids_mouse`

    As a result, all :ref:`zephyr:nrf54h20dk_nrf54h20` configurations of the affected samples were migrated from the NVS settings backend to the ZMS settings backend.
  * Testing steps in the :ref:`peripheral_hids_mouse` to provide the build configuration that is compatible with the `Bluetooth Low Energy app`_ testing tool.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated the MCUboot bootloader configuration for the :ref:`zephyr:nrf54l15dk_nrf54l15` board target to enable the :kconfig:option:`CONFIG_FPROTECT` Kconfig option that is used to protect the bootloader partition against memory corruption.

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Cellular samples
----------------

* Updated the :kconfig:option:`CONFIG_NRF_CLOUD_CHECK_CREDENTIALS` Kconfig option to be optional and enabled by default for the following samples:

  * :ref:`nrf_cloud_rest_cell_location`
  * :ref:`nrf_cloud_rest_device_message`
  * :ref:`nrf_cloud_rest_fota`

* :ref:`location_sample` sample:

  * Updated:

    * The Thingy:91 X build to support Wi-Fi by default without overlays.

Cryptography samples
--------------------

|no_changes_yet_note|

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

* :ref:`dect_shell_application` sample:

  * Added:

    * The ``dect mac`` command.
      A brief MAC-level sample on top of DECT PHY interface with new commands to create a periodic cluster beacon, scan for it, associate or disassociate a PT/client, and send data to a FT/beacon random access window.
      This is not a full MAC implementation and not fully compliant with DECT NR+ MAC specification (`ETSI TS 103 636-4`_).
    * The ``startup_cmd`` command.
      This command is used to store shell commands to be run sequentially after bootup.
    * Band 4 support for nRF9151 with modem firmware v1.0.2.

  * Updated:

    * The ``dect rssi_scan`` command with busy/possible/free subslot count-based RSSI scan.
    * The ``dect rx`` command to provide the possibility to iterate all channels and to enable RX filter.

Edge Impulse samples
--------------------

* :ref:`ei_data_forwarder_sample` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`ei_wrapper_sample` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

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

* Updated all Matter samples that support low-power mode to enable the :ref:`lib_ram_pwrdn` feature.
  It is enabled by default for the release configuration of the :ref:`matter_lock_sample`, :ref:`matter_light_switch_sample`, :ref:`matter_smoke_co_alarm_sample`, and :ref:`matter_window_covering_sample` samples.

* :ref:`matter_template_sample` sample:

  * Updated the internal configuration for the :ref:`zephyr:nrf54l15dk_nrf54l15` target to use the DFU image compression and provide more memory space for the application.

* :ref:`matter_smoke_co_alarm_sample` sample:

  * Added support for ICD dynamic SIT LIT switching (DSLS).

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

* Updated GPIO pins on nRF54L15 DK used for communication between the client and server over UART.
  One of the previously selected pins was also used to drive an LED, which may have disrupted the UART communication.

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

* Removed support for the ``nrf5340dk/nrf5340/cpuapp/ns`` build target for all samples.

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Updated:

    * Configuration for the :ref:`zephyr:nrf54h20dk_nrf54h20` board to support multi-domain logging using the ARM Coresight STM.
    * The logging format in the standard logging mode to align it with the format used in the multi-domain logging mode.
    * Support for alternative configurations to use the :ref:`file suffix feature from Zephyr <app_build_file_suffixes>`.
      The following file suffixes are supported as alternative configurations:

        * ``flash_and_run``
        * ``heap_memory``
        * ``static_memory``
        * ``multiple_threads``

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Wi-Fi drivers
-------------

* Added the :ref:`nrf70_wifi_tx_power_calculation` section to the :ref:`nrf70_wifi` page.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated the :ref:`req_appln_limitations` page to clarify carrier-specific requirements.
    Added overlay files and documentation to :ref:`serial_lte_modem` application and :ref:`lwm2m_carrier` sample to guide in the correct usage of LwM2M carrier library for SoftBank and LG U+.

Bluetooth libraries and services
--------------------------------

* :ref:`hogp_readme` library:

  * Updated the :c:func:`bt_hogp_rep_read` function to forward the GATT read error code through the registered user callback.
    This ensures that API user is aware of the error.

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

|no_changes_yet_note|

Modem libraries
---------------

|no_changes_yet_note|

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

|no_changes_yet_note|

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

Security libraries
------------------

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

* Added semantic version support to :ref:`nrf_desktop_config_channel_script` Python script for devices that use the SUIT DFU.

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

|no_changes_yet_note|

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

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``beb733919d8d64a778a11bd5e7d5cbe5ae27b8ee``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline beb733919d ^ea02b93eea

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^beb733919d

The current |NCS| main branch is based on revision ``beb733919d`` of Zephyr.

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

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Fixed an issue on the :ref:`install_ncs` page where an incorrect directory path was provided for Linux and macOS at the end of the :ref:`cloning_the_repositories_win` section.
