.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.4.99
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
See `known issues for nRF Connect SDK v2.4.0`_ for the list of issues valid for the latest release.

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

Working with nRF91 Series
=========================

* Added support for :ref:`nrf91_modem_trace_uart_snippet`.
  Snippet is used for nRF91 modem tracing with the UART backend for the following applications and samples:

  * :ref:`asset_tracker_v2`
  * :ref:`serial_lte_modem`
  * All samples that use nRF9160 DK except for nRF9160: SLM Shell, nRF9160: Modem trace external flash backend, and nRF9160: Modem trace backend samples

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

See `Bluetooth mesh samples`_ for the list of changes in the Bluetooth mesh samples.

Matter
------

* Added a page about :ref:`ug_matter_device_optimizing_memory`.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

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

* Added:

  * Integration of Wi-Fi connectivity with Connection Manager connectivity API.
  * The :kconfig:option:`CONFIG_NRF_WIFI_IF_AUTO_START` Kconfig option to enable an application to set/unset AUTO_START on an interface.
    This can be done by using the ``NET_IF_NO_AUTO_START`` flag.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Updated:

  * Default value of the Kconfig option :kconfig:option:`CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS` is changed to 300 seconds.
  * Enabled link time optimization to reduce the flash size of the application.
    You can disable this using the Kconfig option :kconfig:option:`CONFIG_ASSET_TRACKER_V2_LTO`.

* Fixed an issue with movement timeout handling in passive mode.

nRF9160: Serial LTE modem
-------------------------

* Removed:

  * DFU AT commands ``#XDFUGET``, ``#XDFUSIZE`` and ``#XDFURUN`` because they were not usable without a custom application in the target (nRF52 series) device.
  * Support for bootloader FOTA update because it is not needed for Serial LTE modem.

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the machine learning models (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) used by the application so that they are now hosted by Nordic Semiconductor.

nRF Desktop
-----------

* Added Kconfig options to enable handling of the power management events for the following nRF Desktop modules:

  * :ref:`nrf_desktop_board` - The :ref:`CONFIG_DESKTOP_BOARD_PM_EVENTS <config_desktop_app_options>` Kconfig option.
  * :ref:`nrf_desktop_motion` - The :ref:`CONFIG_DESKTOP_MOTION_PM_EVENTS <config_desktop_app_options>` Kconfig option.
  * :ref:`nrf_desktop_ble_latency` - The :ref:`CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS <config_desktop_app_options>` Kconfig option.

  All listed Kconfig options are enabled by default and depend on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.

* Updated:

  * Set the max compiled-in log level to ``warning`` for the USB HID class (:kconfig:option:`CONFIG_USB_HID_LOG_LEVEL_CHOICE`) and reduce the log message levels used in the :ref:`nrf_desktop_usb_state_pm` source code.
    This is done to avoid flooding logs during USB state changes.
  * If the USB state is set to :c:enum:`USB_STATE_POWERED`, the :ref:`nrf_desktop_usb_state_pm` restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED` instead of requiring :c:enum:`POWER_MANAGER_LEVEL_ALIVE`.
    This is done to prevent the device from powering down and waking up multiple times when an USB cable is connected.
  * Disabled ``CONFIG_BOOT_SERIAL_IMG_GRP_HASH`` in MCUboot bootloader release configurations of boards that use nRF52820 SoC.
    This is done to reduce the memory consumption.

Thingy:53: Matter weather station
---------------------------------

* Added support for the nRF7002 Wi-Fi expansion board.

Samples
=======

Bluetooth samples
-----------------

* :ref:`direct_test_mode` sample:

  * Added:

    * Support for the nRF52840 DK.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Fixed an issue with the sample not fitting into RAM size on the nrf52dk_nrf52832 board.

Cryptography samples
--------------------

* Added the :ref:`crypto_ecjpake` sample demonstrating usage of EC J-PAKE.

nRF9160 samples
---------------

* Added the :ref:`battery` sample to show how to use the :ref:`modem_battery_readme` library.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Added documentation for using the :ref:`lib_nrf_cloud_alert` and :ref:`lib_nrf_cloud_log` libraries.
  * Changed the :file:`overlay_nrfcloud_logging.conf` file to enable JSON logs by default.
  * The :c:struct:`nrf_cloud_obj` structure and associated functions are now used to encode and decode nRF Cloud data.

* :ref:`http_application_update_sample` sample:

   * Updated credentials for the HTTPS connection.

* :ref:`http_full_modem_update_sample` sample:

   * Updated credentials for the HTTPS connection.

* :ref:`http_modem_delta_update_sample` sample:

   * Updated credentials for the HTTPS connection.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added:

    * The ``disable_response`` parameter to the :c:struct:`nrf_cloud_rest_location_request` structure.
      If set to true, no location data is returned to the device when the :c:func:`nrf_cloud_rest_location_get` function is called.
    * A Kconfig option :kconfig:option:`REST_CELL_LOCATION_SAMPLE_VERSION` for the sample version.

  * Updated the sample to print its version when started.

* :ref:`modem_shell_application` sample:

  * The sample now uses the :ref:`lib_nrf_cloud` library function :c:func:`nrf_cloud_obj_pgps_request_create` to create a P-GPS request.

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Matter samples
--------------

|no_changes_yet_note|

NFC samples
-----------

|no_changes_yet_note|

Multicore samples
-----------------

|no_changes_yet_note|

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

* Added :ref:`wifi_wfa_qt_app_sample` that demonstrates how to use the WFA QuickTrack (WFA QT) library needed for Wi-Fi Alliance QuickTrack certification.
* Added :ref:`wifi_shutdown_sample` that demonstrates how to configure the Wi-Fi driver to shut down the Wi-Fi hardware when the Wi-Fi interface is not in use.

Other samples
-------------

* Removed the random hardware unique key sample.
  The sample is redundant since its functionality is presented as part of the :ref:`hw_unique_key_usage` sample.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

* Added:

  * :ref:`nrf_security` library, relocated from the sdk-nrfxlib repository to the :file:`subsys/nrf_security` directory.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_mesh` library:

  * Added:

    * The :kconfig:option:`BT_MESH_LIGHT_CTRL_AMB_LIGHT_LEVEL_TIMEOUT` Kconfig option that configures a timeout before resetting the ambient light level to zero.

  * Updated:

    * The :kconfig:option:`BT_MESH_MODEL_SRV_STORE_TIMEOUT` Kconfig option, that is controlling timeout for storing of model states, is replaced by the :kconfig:option:`BT_MESH_STORE_TIMEOUT` Kconfig option.

  * Fixed an issue where the :ref:'bt_mesh_dtt_srv_readme' model could not be found for models spanning multiple elements.

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

Modem libraries
---------------

* Added the :ref:`modem_battery_readme` library that obtains battery voltage information or notifications from a modem.

* :ref:`nrf_modem_lib_readme`:

  * Updated the :c:func:`nrf_modem_lib_shutdown` function to allow the modem to be in flight mode (``CFUN=4``) when shutting down the modem.

* :ref:`lib_location` library:

  * Neighbor cell search is modified to use GCI search depending on :c:member:`location_cellular_config.cell_count` value.

* :ref:`pdn_readme` library:

  * Updated the library to allow a ``PDP_type``-only configuration in the :c:func:`pdn_ctx_configure` function.

* :ref:`nrf_modem_lib_readme`:

   * Updated the :c:func:`modem_key_mgmt_cmp` function to return ``1`` if the buffer length does not match the certificate length.

Libraries for networking
------------------------

* Multicell location library:

  * This library is now removed and relevant functionality is available through the :ref:`lib_location` library.

* :ref:`lib_nrf_cloud_log` library:

  * Added explanation of text versus dictionary logs.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * :c:struct:`nrf_cloud_obj` structure and functions for encoding and decoding nRF Cloud data.
    * :c:func:`nrf_cloud_obj_pgps_request_create` function that creates a P-GPS request for nRF Cloud.

  * Updated:

    * Moved JSON manipulation from :file:`nrf_cloud_fota.c` to :file:`nrf_cloud_codec_internal.c`.
    * Fixed a build issue that occurred when MQTT and P-GPS are enabled and A-GPS is disabled.

  * Removed:

    * Unused internal codec function ``nrf_cloud_format_single_cell_pos_req_json()``.
    * ``nrf_cloud_location_request_msg_json_encode()`` function and replaced with :c:func:`nrf_cloud_obj_location_request_create`.

Libraries for NFC
-----------------

|no_changes_yet_note|

Nordic Security Module
----------------------

:ref:`nrf_security` library:

  * Removed:

    * Option to build Mbed TLS builtin PSA core (:kconfig:option:`CONFIG_PSA_CORE_BUILTIN`).
    * Option to build Mbed TLS builtin PSA crypto driver (:kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_BUILTIN`) and all its associated algorithms (``CONFIG_MBEDTLS_PSA_BUILTIN_ALG_xxx``).

Other libraries
---------------

* :ref:`lib_identity_key` library:

  * Updated:

    * :c:func:`identity_key_write_random`, :c:func:`identity_key_write_key` and :c:func:`identity_key_write_dummy` functions to return an error code and not panic on error.
    * :c:func:`identity_key_read` function to always return an error code from the library-defined codes.
    * The defined error code names with prefix IDENTITY_KEY_ERR_*.

* :ref:`lib_hw_unique_key` library:

  * Updated:

    * :c:func:`hw_unique_key_write`, :c:func:`hw_unique_key_write_random` and :c:func:`hw_unique_key_load_kdr` functions to return an error code and not panic on error.
    * :c:func:`hw_unique_key_derive_key` function to always return an error code from the library-defined codes.
    * The defined error code names with prefix HW_UNIQUE_KEY_ERR_*.


Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_adv`:

  * Updated the dependencies of the :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST` Kconfig option so that it can be used when the Bluetooth controller is running on the network core.

* :ref:`caf_power_manager`:

  * Reduced verbosity of logs denoting allowed power states from ``info`` to ``debug``.

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

* Removed the relocated :ref:`nrf_security` library.

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

DFU libraries
-------------

|no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`partition_manager`:

  * The size of the span partitions was changed to include the alignment paritions (``EMPTY_x``) appearing between other partitions, but not alignment partitions at the beginning or end of the span partition.
    The size of the span partitions now reflects the memory space taken from the start of the first of its elements to the end of the last, not just the sum of the sizes of the included partitions.

* :ref:`west_sbom`:

  * Changed:

    * To reduce RAM usage, the script now runs the `Scancode-Toolkit`_ detector in a single process.
      This change slows down the licenses detector, because it is no longer executed simultaneously on all files.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``74c4d1c52fd51d07904b27a7aa9b2303e896a4e3``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``a8b28f13c195a00bdf50f5c24092981124664ed9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline a8b28f13c1 ^4bbd91a908

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^a8b28f13c1

The current |NCS| main branch is based on revision ``a8b28f13c1`` of Zephyr.

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

* Added:

  * A page on :ref:`ug_wireless_coexistence` in :ref:`protocols`.
  * Pages on :ref:`thread_device_types` and :ref:`thread_sed_ssed` to the :ref:`ug_thread` documentation.
  * A new section :ref:`ug_pmic`, containing :ref:`ug_npm1300_features` and :ref:`ug_npm1300_gs`.

* Updated:

  * The :ref:`emds_readme` library documentation with :ref:`emds_readme_application_integration` section about the formula used to compute the required storage time at shutdown in a worst case scenario.
  * The structure of the :ref:`nrf_modem_lib_readme` documentation.
  * The structure of the |NCS| documentation at its top level, with the following major changes:

    * The getting started section has been replaced with :ref:`Installation <getting_started>`.
    * The guides previously located in the application development section have been moved to :ref:`config_and_build`, :ref:`test_and_optimize`, :ref:`device_guides`, and :ref:`security_index`.
      Some of these new sections also include guides that were previously in the getting started section.
    * "Working with..." device guides are now located under :ref:`device_guides`.
    * :ref:`release_notes`, :ref:`software_maturity`, :ref:`known_issues`, :ref:`glossary`, and :ref:`dev-model` are now located under :ref:`releases_and_maturity`.
