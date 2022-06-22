.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.0.99
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
See `known issues for nRF Connect SDK v2.0.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Added information about :ref:`app_build_output_files` on the :ref:`app_build_system` page.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth mesh
--------------

|no_changes_yet_note|

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

Matter
------

* Removed the low-power configuration build type from all Matter samples.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``25e241ebcbf11b1f63dbe25546b1f10219866ad0``.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

Thread
------

* Added information about Synchronized Sleepy End Device (SSED) and SED vs SSED activity in the :ref:`thread_ot_device_types` documentation.
* Multiprotocol support was removed from :file:`overlay-cert.config` and moved to :file:`overlay-multiprotocol.conf`.

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

* Updated:

  * Enabled the PAN ID conflict resolution in applications that uses :ref:`lib_zigbee_application_utilities` library.
    For details, see `Libraries for Zigbee`_.
  * Changed the default entropy source of Zigbee samples and unit tests to Cryptocell for SoCs that have Cryptocell.

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

ESB
---

* Fixed the ``update_radio_crc()`` function in order to correctly configure the CRC's registers (8 bits, 16 bits, or none).

RF Front-End Modules
====================

* Fixed a build error that occurred when building an application for nRF53 SoCs with Simple GPIO Front-End Module support enabled.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

  * Removed:

    * ``CONFIG_APP_REQUEST_GNSS_ON_INITIAL_SAMPLING`` option.
    * ``CONFIG_APP_REQUEST_NEIGHBOR_CELLS_DATA`` option.

  * Updated:

    * The default value of the GNSS timeout in the application's :ref:`Real-time configurations <real_time_configs>` is now 30 seconds.

  * Fixed:

    * An issue that reports GNSS altitude, accuracy, and speed incorrectly when using LwM2M engine.
    * An issue that caused modem FOTA jobs to be reported as not validated to nRF Cloud.

nRF9160: Serial LTE modem
-------------------------

  * Added:

    * URC for GNSS sleep and wakeup events.
    * Selected flags support in #XRECV and #XRECVFROM commands.
    * Multi-PDN support in the Socket service.
    * The GNSS service now signifies location info to nRF Cloud.

  * Updated:

    * The AT response and the URC sent when the application enters and exits data mode.
    * ``WAKEUP_PIN`` and ``INTERFACE_PIN`` are now defined as *Active Low*. Both are *High* when the SLM application starts.

  * Removed:

    * The software toggle of ``INDICATE_PIN`` in case of reset.

nRF5340 Audio
-------------

* Documentation in the :ref:`nrf53_audio_app_building_script` section now mentions how to recover the device if programming using script fails.

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

nRF Desktop
-----------

* nRF Desktop peripherals no longer automatically send security request immediately after Bluetooth LE connection is established.
  The feature can be turned on using :kconfig:option:`CONFIG_CAF_BLE_STATE_SECURITY_REQ`.
* nRF Desktop dongles start peripheral discovery immediately after Bluetooth LE connection is established.
  The dongles no longer wait until the connection is secured.

|no_changes_yet_note|

Thingy:53 Zigbee weather station
--------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* :ref:`ble_nrf_dm` sample:

  * Split the configuration of the :ref:`mod_dm` module from the :ref:`nrf_dm`.
    This allows the use of the Nordic Distance Measurement library without the module.

* :ref:`direct_test_mode` sample:

  * Added a workaround for nRF5340 revision 1 Errata 117.

* :ref:`peripheral_hr_coded` sample:

  * Added configuration for the nRF5340 target.
  * Fixed advertising start on the nRF5340 target with the Zephyr LL controller.
    Previously, it was not possible to start advertising, because the :kconfig:option:`CONFIG_BT_EXT_ADV` option was disabled for the Zephyr LL controller.

* :ref:`bluetooth_central_hr_coded` sample:

  * Added configuration for the nRF5340 target.
  * Fixed scanning start on the nRF5340 target with the Zephyr LL controller.
    Previously, it was not possible to start scanning, because the :kconfig:option:`CONFIG_BT_EXT_ADV` option was disabled for the Zephyr LL controller.

* :ref:`peripheral_fast_pair` sample:

  * Added possibility of toggling between show and hide UI indication in the Fast Pair not discoverable advertising.

Bluetooth mesh samples
----------------------

|no_changes_yet_note|

nRF9160 samples
---------------

* Added :ref:`modem_trace_backend_sample` sample, demonstrating how to add a custom modem trace backend. The custom backend prints the amount of trace data received in bytes, trace data throughput, and CPU load.
* :ref:`lwm2m_client` sample:

  * Fixed:

    * Default configuration conforms to the LwM2M specification v1.0 instead of v1.1.
      For enabling v1.1 there is already an overlay file.
    * Bootstrap is not TLV only anymore.
      With v1.1, preferred content format is sent in the bootstrap request.
      SenML CBOR takes precedence over SenML JSON and OMA TLV, when enabled.

  * Added:

    * CoAP max message size is set to 1280 by default.
    * Number of SenML CBOR records is set to a higher value to cope with data exchange after registration with Coiote server.

* :ref:`modem_shell_application` sample:

  * Added:

    * nRF9160 DK overlays for enabling BT support.
      When running this configuration, you can perform BT scanning and advertising using the ``bt`` command.
    * Support for injecting GNSS reference altitude for low accuracy mode.
      For a position fix using only three satellites, GNSS module must have a reference altitude that can now be injected using the ``gnss agps ref_altitude`` command.
    * New command ``startup_cmd``, which can be used to store up to three MoSh commands to be run on start/bootup.
      By default, commands are run after the default PDN context is activated, but can be set to run N seconds after bootup.

  * Updated:

    * Changed timeout parameters from seconds to milliseconds in ``location`` and ``rest`` commands.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Changed:

    * This sample now uses the :ref:`lib_modem_antenna` library to configure the GNSS antenna instead of configuring it directly.
    * Minor logging and function structure improvements
    * :ref:`lib_nrf_cloud` library is no longer de-initialized and re-initialized on disconnect and reconnect.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added:
    * Support for full modem FOTA updates.

Thread samples
--------------

* Enables logging of errors and hard faults in CLI sample by default.

Matter samples
--------------

* Removed the low-power configuration build type from all Matter samples.

* :ref:`matter_light_switch_sample`:

  * Set :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` to be enabled by default.

* :ref:`matter_lock_sample`:

  * Set :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` to be enabled by default.

* :ref:`matter_window_covering_sample`:

  * Set :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` and :kconfig:option:`CONFIG_CHIP_THREAD_SSED` to be enabled by default.
  * Added information about the :ref:`matter_window_covering_sample_ssed` in the sample documentation.

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

* :ref:`zigbee_light_switch_sample` sample:

  * Fixed an issue where a buffer would not be freed after a failure occurred when sending a Match Descriptor request.

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` service:

  * Added a SHA-256 hash check to ensure the Fast Pair provisioning data integrity.
  * Added unit test for the storage module.
  * Extended API to allow setting the flag for the hide UI indication in the Fast Pair not discoverable advertising data.

* :ref:`bt_enocean_readme` library
  * Added callback :c:member:`decommissioned` to :c:struct:`bt_enocean_callbacks` when EnOcean switch is decommissioned.

* :ref:`bt_mesh` library:

  * Added:

    * :ref:`bt_mesh_silvair_enocean_srv_readme` added use of decommissioned callback when EnOcean switch is decommissioned.

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* Updated:

  * :ref:`at_monitor_readme` library:

    * The :c:func:`at_monitor_pause` and :c:func:`at_monitor_resume` macros are now functions, and take a pointer to the AT monitor entry.

  * :ref:`modem_key_mgmt` library:

    * Fixed:

      * An issue that would cause the library to assert on an unhandled CME error when the AT command failed to be sent.

  * :ref:`at_cmd_parser_readme` library:

    * Fixed:

      * An issue that would cause AT command responses like ``+CNCEC_EMM`` with underscore to be filtered out.

  * :ref:`pdn_readme` library:

    * Added:

      * Support for setting multiple event callbacks for the default PDP context.
      * The :c:func:`pdn_default_ctx_cb_dereg` function to deregister a callback for the default PDP context.
      * The :c:func:`pdn_esm_strerror` function to retrieve a textual description of an ESM error reason.
        The function is compiled when :kconfig:option:`CONFIG_PDN_ESM_STRERROR` Kconfig option is enabled.

    * Updated:

      * Automatically subscribe to ``+CNEC=16`` and ``+CGEREP=1`` if the :ref:`lte_lc_readme` library is used to change the modem's functional mode.

    * Removed:

      * The :kconfig:option:`CONFIG_PDN_CONTEXTS_MAX` Kconfig option.
        The maximum number of PDP contexts is now dynamic.

  * :ref:`nrf_modem_lib_readme` library:

    * Updated:

      * Ability to add :ref:`custom trace backends <adding_custom_modem_trace_backends>`.

  * :ref:`lib_location` library:

    * Changed timeout parameters' type from uint16_t to int32_t, unit from seconds to milliseconds, and value to disable them from 0 to SYS_FOREVER_MS.
      This change is done to align with Zephyr's style for timeouts.

Libraries for networking
------------------------

* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Fixed:

      * An issue that caused the application to receive multiple disconnect events.

    * Added:

      * :c:func:`nrf_cloud_fota_pending_job_validate` function that enables an application to validate a pending FOTA job before initializing the :ref:`lib_nrf_cloud` library.
      * Handling for new nRF Cloud REST error code 40499.
        Moved the error log from the :c:func:`nrf_cloud_parse_rest_error` function into the calling function.
      * Support for full modem FOTA updates.

  * :ref:`lib_multicell_location` library:

    * Added timeout parameter.
    * Made a structure for input parameters for multicell_location_get() to make updates easier in the future.

  * :ref:`lib_rest_client` library:

    * Updated timeout handling. Now using http_client library timeout also.
    * Removed CONFIG_REST_CLIENT_SCKT_SEND_TIMEOUT and CONFIG_REST_CLIENT_SCKT_RECV_TIMEOUT.

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added:

  * :ref:`nrf_rpc_ipc_readme` library.

* :ref:`lib_flash_patch` library:

  * Allow the :kconfig:option:`CONFIG_DISABLE_FLASH_PATCH` Kconfig option to be used on the nRF52833 SoC.

* :ref:`doc_fw_info` module:

  * Fixed a bug where MCUboot would experience a fault when using the :ref:`doc_fw_info_ext_api` feature.

* :ref:`nrf_rpc_ipc_readme`:

  * This library can use different transport implementation for each nRF RPC group.
  * Memory for remote procedure calls is now allocated on a heap instead of the calling thread stack.

* :ref:`emds_readme`

  * Updated :c:func:`emds_entry_add` to no longer use heap, but instead require a pointer to the dynamic entry structure :c:struct `emds_dynamic_entry`.
    The dynamic entry structure should be allocated in advance.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_state`:

  * Running on Bluetooth Peripheral no longer automatically send security request immediately after Bluetooth LE connection is established.
    The :kconfig:option:`CONFIG_CAF_BLE_STATE_SECURITY_REQ` can be used to enable this feature.
    The option can be used for both Bluetooth Peripheral and Bluetooth Central.

* :ref:`caf_sensor_data_aggregator`:

  * Added unit tests for the library.

* :ref:`caf_sensor_manager`:

  * No longer uses floats to calculate and determine if the sensor trigger is activated.
    This is because the float uses more space.
    Also, data sent to :c:struct:`sensor_event` uses :c:struct:`sensor_value` instead of float.

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

* :ref:`lib_zigbee_application_utilities` library:

  * Added :kconfig:option:`CONFIG_ZIGBEE_PANID_CONFLICT_RESOLUTION` for enabling automatic PAN ID conflict resolution.
    This option is enabled by default.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`bt_fast_pair_provision_script`:

  * Added a SHA-256 hash of the Fast Pair provisioning data to ensure its integrity.

Unity
-----

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``e86f575f68fdac2cab1898e0a893c8c6d8fd0fa1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``53fbf40227de087423620822feedde6c98f3d631``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 53fbf40227 ^45ef0d2

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^53fbf40227

The current |NCS| main branch is based on revision ``53fbf40227`` of Zephyr.

zcbor
=====

The `zcbor`_ module has been updated from version 0.4.0 to 0.5.1.
Release notes for 0.5.0 and 0.5.1 can be found in :file:`ncs/nrf/modules/lib/zcbor/RELEASE_NOTES.md`.
:ref:`lib_fmfu_fdev` code has been regenerated using zcbor 0.5.1.


Trusted Firmware-M
==================

* Fixed:

  * |no_changes_yet_note|

cJSON
=====

* |no_changes_yet_note|

Documentation
=============

* Added:

  * Documentation for the :ref:`lib_flash_map_pm` library.
  * Documentation for the :ref:`lib_adp536x` library.
  * Documentation for the :ref:`lib_flash_patch` library.
  * :ref:`gs_debugging` section on the :ref:`gs_testing`.
    Also added links to this section in different areas of documentation.
  * :ref:`ug_thread_prebuilt_libs` as a separate page instead of being part of :ref:`ug_thread_configuring`.
  * Added software maturity entries for security features: TF-M, PSA crypto, Immutable bootloader, HW unique key.
  * A section about NFC in the :ref:`app_memory` page.

* Updated:

  * :ref:`ug_thread_configuring` page to better indicate what is required and what is optional.
    Also added further clarifications to the page to make everything clearer.
  * :ref:`ug_nrf9160_gs` guide by moving :ref:`nrf9160_gs_updating_fw_modem` section before :ref:`nrf9160_gs_updating_fw_application` because updating modem firmware erases application firmware.
  * :ref:`ug_matter_tools` page with a new section about the ZAP tool.
  * :ref:`build_pgm_nrf9160` section in the :ref:`ug_nrf9160` documentation by adding |VSC| and command line instructions.

* Removed:

  * |no_changes_yet_note|

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
