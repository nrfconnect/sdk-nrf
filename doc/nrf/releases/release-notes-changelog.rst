.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.9.99
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
See `known issues for nRF Connect SDK v1.9.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Fixed:

  * Issue with NCS Toolchain delimiter handling on MacOS, which could in special situations result in the build system not being able to properly locate the correct program needed.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth mesh
--------------

* Added:

  * :c:struct:`bt_mesh_sensor_srv` context to relevant callbacks and APIs to help resolve the associated sensor model instance.
    For details, see `Bluetooth mesh samples`_ and `Bluetooth libraries and services`_.

Thread
------

* Added:

  *  Support for the Link Metrics and CSL Thread v1.2 features for the nRF53 Series devices.

* Removed:

  *  Support for the :ref:`thread_architectures_designs_cp_ncp` architecture and the related tools.

Zigbee
------

* Updated:

  * :ref:` Zigbee shell library <lib_zigbee_shell>`. For details, see `Libraries for Zigbee`_.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

   * Support for :ref:`Bosch Software Environmental Cluster (BSEC) library <bosch_software_environmental_cluster_library>` driver .
   * Support for indoor air quality (IAQ) readings retrieved from the BME680 sensor on Thingy:91. For more information, see the :ref:`asset_tracker_v2_sensor_module`.
   * Support for QEMU x86 emulation.

* Updated:

  * For nRF Cloud builds, the configuration section in the shadow is now initialized during the cloud connection process.
  * Allow the :ref:`lib_nrf_cloud` library to handle modem FOTA updates if :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` is enabled.

nRF9160: Serial LTE Modem
-------------------------

* Updated:

  * Enhanced the #XHTTPCREQ AT command for better HTTP upload and download support.
  * Enhanced the #XSLEEP AT command to support data indication when idle.
  * Enhanced the MQTT client to support the reception of large PUBLISH payloads.
  * Added the #XDFUGET and #XDFURUN AT commands to support the cloud-to-nRF52 DFU service.

nRF Desktop
-----------

* Added:

  * Documentation for selective HID report subscription in :ref:`nrf_desktop_usb_state` using :kconfig:option:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION` option.

Thingy:53 Zigbee weather station
--------------------------------

* Added new :ref:`zigbee_weather_station_app` application.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`peripheral_ams_client` sample.

* Updated:

  * :ref:`direct_test_mode` sample:

    * Fixed handling of the disable Constant Tone Extension command.
    * The front-end module test parameters are not set to their default value after the DTM reset command.
    * Added the vendor-specific ``FEM_DEFAULT_PARAMS_SET`` command for restoring the default front-end module parameters.

  * :ref:`peripheral_hids_mouse` sample:

    * Increased the main stack size from 1024 to 1536 bytes.
    * Increased the stack size of the nRF RPC threads from 1024 to 1280.

nRF9160 samples
---------------

* Added:

  * :ref:`nrf_cloud_rest_device_message` sample, demonstrating how to send an arbitrary device message with the nRF Cloud REST API.

Bluetooth mesh samples
----------------------

* Updated

  * All samples are using the Partition Manager, replacing the use of the Device Tree Source flash partitions.
  * :ref:`bluetooth_mesh_sensor_server` sample:

    * Updated definitions for sensor callbacks to include the :c:struct:`bt_mesh_sensor_srv` context.

Thread samples
--------------

* Updated:

  * The relevant sample documentation pages with information about support for :ref:`Trusted Firmware-M <ug_tfm>`.
  * :ref:`ot_cli_sample` sample:

    * Added :file:`prj_thread_1_2.conf` to support Thread v1.2 build for the nRF52 and nRF53 Series devices.
    * Added child image configuration files for network core builds for Thread v1.2 build.

  * All sample documentation with a Configuration section, and organized relevant information under that section.

Matter samples
--------------

* Added:

  * Release configuration for all samples.
  * A binding cluster to the Light Switch sample.
  * The Groupcast communication to the Light Switch example.
  * OTA DFU support for template application

* Updated:

  * All ZAP configurations due to changes in Matter upstream.
  * Pairing process to Binding process in the Light Switch sample.

nrf5340 Samples
---------------

* Added:

  * :ref:`nrf5340_remote_shell` sample.
  * :ref:`nrf5340_multicore` sample.

nrf9160 Samples
---------------

* Added:

  * Shell functionality to HTTP Update samples.
  * :ref:`modem_callbacks_sample` sample, showcasing initialization and de-initialization callbacks.

* Updated:

  * :ref:`at_monitor_sample` sample:

    * Added ``denied``, ``unknown``, ``roaming``, and ``UICC failure`` CEREG status codes to :c:func:`cereg_mon`.

  * :ref:`modem_shell_application` sample:

    * Added:

      * An option ``--interval`` (in seconds) to neighbor cell measurements in continuous mode  (``link ncellmeas --continuous``).
        When using this option, a new measurement is started in each interval.
      * Separate plain AT command mode that can be started with the command ``at at_cmd_mode start``.
        AT command termination methods can be configured using Kconfig options.
        The default method is CR termination.
        In AT command mode, a maximum of 10 AT commands can be pipelined with ``|`` as the delimiter character between pipelined AT commands.
      * Threading support for the ``ping`` command.
      * Iperf3 usage over Zephyr native TCP/IP stack and nRF9160 LTE default context.

  * :ref:`nrf_cloud_rest_fota` sample:

    * Enabled building of bootloader FOTA update files.
    * Corrected handling of the bootloader FOTA updates.

  * :ref:`lwm2m_client` sample:

    * Reworked the retry logic so that the sample can fall back to bootstrap mode and need not always restart the LTE connection.
      Added :ref:`state_diagram` to the sample documentation.
    * Replaced the deprecated GPS-driver with the new GNSS interface.
    * Added minimal Portfolio object support that is required for LwM2M conformance testing.
    * Added guidelines on :ref:`setting up the sample for production <lwm2m_client_provisioning>` using AVSystem’s Coiote Device Management server.

  * :ref:`download_sample` sample:

    * Updated the default HTTPS URL and certificate due to the old link being broken.

Gazell samples
--------------

* Updated:

  * Separated the :ref:`gazell_samples` into their own pages for Host and Device.

    There are now four different sample pages, where each Host sample must be used along with its corresponding Device sample.

Zigbee samples
--------------

* Added support for the factory reset functionality from :ref:`lib_zigbee_application_utilities` in the following:

  * :ref:`zigbee_light_bulb_sample`
  * :ref:`zigbee_light_switch_sample`
  * :ref:`zigbee_network_coordinator_sample`
  * :ref:`zigbee_shell_sample`
  * :ref:`zigbee_template_sample`

Other Samples
-------------

* Added the :ref:`ipc_service_sample` sample, showing throughtput of IPC service with available backends.

* Updated:

  * :ref:`radio_test`:

    * Added new configuration that builds the sample with support for remote IPC Service shell on nRF5340 application core through USB.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* Added:

  * :ref:`ams_client_readme` library.

* :ref:`gatt_dm_readme` library:

  * Fixed discovery of empty services.
  * Added option to discover several service instances using the UUID.

* :ref:`bt_mesh` library:

  * Added:

    * :c:struct:`bt_mesh_sensor_srv` context to the following API and callbacks:

      * :c:func:`sensor_column_encode` API.
      * :c:member:`get` and :c:member:`set` callbacks in :c:struct:`bt_mesh_sensor_setting`.
      * :c:member:`get` callback in :c:struct:`bt_mesh_sensor_series`.
      * :c:member:`get` callback in :c:struct:`bt_mesh_sensor`.

    * Shell commands for client models.

* :ref:`ble_rpc` library:

  * Added host callback handlers for the ``write`` and ``match`` operations of the CCC descriptor.
  * Fixed the serialization of the write callback applied to the GATT attribute.
  * Fixed the serialization of the :c:func:`bt_gatt_service_unregister` function call.

Modem libraries
---------------

* Updated:

  * :ref:`sms_readme` library:

    * Added handling for SMS client unregistration notification from the modem.
      When the notification is received, the library re-registers the SMS client automatically.

  * :ref:`lib_location` library:

    * Added support for P-GPS data retrieval from an external source, implemented separately by the application.
      Enabled by setting the :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL` option.
      The library triggers a :c:enum:`LOCATION_EVT_GNSS_PREDICTION_REQUEST` event when assistance is needed.
    * The :c:member:`request` member of the :c:struct:`location_event_data` structure was renamed to :c:member:`agps_request`.
    * Current system time is attached to the ``location_datetime`` parameter of the location request response with Wi-Fi and cellular methods.
      The timestamp comes from the moment of scanning or neighbor measurements.
    * Removed dependency on the :ref:`lib_modem_jwt` library.
      The :ref:`lib_location` library now selects :kconfig:option:`CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT` when using :kconfig:option:`CONFIG_NRF_CLOUD_REST`.
    * Added a new feature, obstructed satellite visibility detection for GNSS.
      When this feature is enabled, the library tries to detect occurrences where getting a GNSS fix is unlikely or would consume a lot of energy.
      When such an occurrence is detected, GNSS is stopped without waiting for a fix or a timeout.

  * :ref:`nrf_modem_lib_readme` library:

    * Added :c:macro:`NRF_MODEM_LIB_ON_INIT` macro for compile-time registration of callbacks on modem initialization.
    * Added :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macro for compile-time registration of callbacks on modem de-initialization.
    * Deprecated :c:func:`nrf_modem_lib_shutdown_wait` function, in favor of :c:macro:`NRF_MODEM_LIB_ON_INIT`.
    * Added :kconfig:option:`CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID` to enable logging for both FW version and UUID at the end of the library initialization step.

  * :ref:`lte_lc_readme` library:

    * Added :c:macro:`LTE_LC_ON_CFUN` macro for compile-time registration of callbacks on modem functional mode changes using :c:func:`lte_lc_func_mode_set`.

Libraries for networking
------------------------

* Updated:

  * :ref:`lib_nrf_cloud_rest` library:

    * Added JSON Web Token (JWT) autogeneration feature.
      If enabled, the nRF Cloud REST library automatically generates a JWT if none is provided by the user when making REST requests.

    * Updated:

      * Centralized error handling.
      * Changes to error return values.
      * For cellular positioning responses, the type is now set based on the ``fulfilledWith`` response from the nRF Cloud.
      * nRF Cloud error codes are now parsed and set in the :c:struct:`nrf_cloud_rest_context` structure.

  * :ref:`lib_download_client` library:

    * Fixed an issue where downloads of COAP URIs would fail when they contained multiple path elements.

  * :ref:`lib_fota_download` library:

    * Added:

      * :c:func:`fota_download_s0_active_get` function that gets the active B1 slot.

  * :ref:`lib_nrf_cloud` library:

    * Added:

      * :c:func:`nrf_cloud_bootloader_fota_slot_set` function that sets the active bootloader slot flag during bootloader FOTA updates.
      * :c:func:`nrf_cloud_pending_fota_job_process` function that processes the state of pending FOTA jobs.
      * :c:func:`nrf_cloud_handle_error_message` function that handles error message responses (MQTT) from nRF Cloud.

    * Updated:

      * During the connection process, shadow data is sent to the application even if no "config" section is present.
      * The application can now send shadow updates earlier in the connection process.
      * nRF Cloud error message responses to location service MQTT requests are now handled.

    * Fixed:

      * Validation of bootloader FOTA updates.

  * :ref:`lib_aws_iot` library:

    * Renamed ``aws_iot_topic_type`` to ``aws_iot_shadow_topic_type`` and removed ``AWS_IOT_SHADOW_TOPIC_UNKNOWN``.

  * :ref:`lib_lwm2m_client_utils` library:

    * Updated the library to store credentials and server settings permanently on bootstrap.

Other libraries
---------------

* :ref:`app_event_manager`:

  * Renamed Event Manager to Application Event Manager.

  * Added:

    * Event type flags to represent if event type should be logged, traced and has dynamic data.
      To update your application, pass a flag variable as a parameter in :c:macro:`APP_EVENT_TYPE_DEFINE` instead of ``init_log``.
      Use :c:macro:`APP_EVENT_FLAGS_CREATE` to set multiple flags:

      .. code-block:: c

         APP_EVENT_TYPE_DEFINE(my_event,
           log_my_event,
           &my_event_info,
           APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_1, APP_EVENT_TYPE_FLAGS_2));

     * :c:func:`app_event_manager_event_size` function with corresponding :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROVIDE_APP_EVENT_SIZE` option.

    * Universal hooks for Application Event Manager initialization, event submission, preprocessing, and postprocessing.
      This includes implementation of macros that register hooks, grouped as follows:

      * :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_LAST`
      * :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_LAST`
      * :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST`

* :ref:`app_event_manager_profiler_tracer`:

  * Updated the :ref:`app_event_manager_profiler_tracer` library.
    The library is no longer directly referenced from the Application Event Manager.
    Instead, it uses the Application Event Manager hooks to connect with the manager.

Common Application Framework (CAF)
----------------------------------

* Added:

  * :ref:`caf_sensor_data_aggregator`, which buffers sensor events and sends them as packages to the listener.

Other libraries
---------------

* Updated:

  * :ref:`esb_readme`:

    * Fixed a compilation error for nRF52833.

* Added:

  * :ref:`shell_ipc_readme`.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Libraries for Zigbee
--------------------

* :ref:`lib_zigbee_application_utilities`:

  * Added factory reset functionality in :ref:`lib_zigbee_application_utilities`.

* :ref:`lib_zigbee_shell`:

  * Added ``nbr monitor`` shell command for monitoring the list of active Zigbee neighbors.
  * Added a set of ``zcl groups`` shell commands for managing Zigbee groups.
  * Added :kconfig:option:`CONFIG_ZIGBEE_SHELL_ZCL_CMD_TIMEOUT` for timing out ZCL cmd commands.
  * Changed internal context manager structures.
  * Changed :ref:`lib_zigbee_shell` structure to be an independent library.
  * Changed file names from ``zigbee_cli*`` to ``zigbee_shell*``.
  * Changed function names from ``zigbee_cli*`` to ``zigbee_shell*``.
  * Changed ``bdb factory_reset`` command.
    Now the command checks if the ZBOSS stack is started before performing the factory reset.
  * Extended ``zcl cmd`` shell command to allow sending groupcasts.
  * Extended ``zdo`` shell commands to allow binding to a group addresses.
  * Fixed an issue where printing binding table containing group-binding entries results in corrupted output.
  * Fixed an issue where Zigbee shell coordinator would not form a new network after the factory reset operation.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Unity
-----

|no_changes_yet_note|

Trusted Firmware-M
==================

* Fixed the NCSDK-13949 known issue where the TF-M Secure Image would copy FICR to RAM on nRF9160.
* Fixed the NCSDK-12306 known issue where a usage fault would be triggered in the debug build on nRF9160.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``1eedec3e79``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``45ef0d2``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 45ef0d2 ^zephyr-v2.7.0

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^45ef0d2

The current |NCS| main branch is based on revision ``45ef0d2`` of Zephyr, which is located between v2.7.0 and v3.0.0 of Zephyr.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``80da8ab1c0aa315a6ee2ce6adf448606372b0a2d``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added the Binding cluster and Groupcast communication to the Light Switch sample.

cddl-gen/zcbor
==============

.. note::
    In March 2022, cddl-gen has been renamed to zcbor.

The `zcbor`_ module has been updated from version 0.3.0 to 0.4.0.
Release notes for 0.4.0 can be found in :file:`ncs/nrf/modules/lib/zcbor/RELEASE_NOTES.md`.

The following major changes have been implemented:

* Renamed "cddl-gen" to "zcbor" throughout the repo.
* Regenerated fmfu code from cddl.
* Added Kconfig options to control the zcbor configuration options.
* Updated tests to run with updated zcbor.

Trusted Firmware-M
==================

* Fixed the NCSDK-14015 known issue that would cause crash during boot when the :kconfig:option:`CONFIG_RPMSG_SERVICE` Kconfig option was enabled on the nRF5340 SoC.

cJSON
=====

*  Fixed an issue with floats in the cJSON module when using NEWLIB_LIBC without the :kconfig:option:`NEWLIB_LIBC_FLOAT_PRINTF` Kconfig option.

Documentation
=============

* Added:

  * Documentation for :ref:`degugging of nRF5340 <debugging>` in :ref:`working with nRF5340 DK<ug_nrf5340>` user guide.
  * Section describing how to enable Amazon Frustration-Free Setup (FFS) support in :ref:`ug_matter_configuring_device_identification`.
  * Notes to the :ref:`bluetooth_central_dfu_smp` sample document specifying the intended use of the sample.
  * DevAcademy links to the :ref:`index` and :ref:`getting_started` pages.

* Changed:

  * Updated the outdated protocol architecture diagram on the :ref:`ug_matter_architecture` page.

* Removed:

  * Documentation on the Getting Started Assistant, as this tool is no longer in use.
    Linux users can install the |NCS| by using the `Installing using Visual Studio Code <Installing on Linux_>`_ instructions or by following the steps on the :ref:`gs_installing` page.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
