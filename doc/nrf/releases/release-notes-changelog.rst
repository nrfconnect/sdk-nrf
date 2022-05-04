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

* Fixed an issue with |NCS| Toolchain delimiter handling on MacOS, which could in special situations result in the build system not being able to properly locate the correct program needed.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth mesh
--------------

* Added :c:struct:`bt_mesh_sensor_srv` context to relevant callbacks and APIs to help resolve the associated sensor model instance.
  For details, see `Bluetooth mesh samples`_ and `Bluetooth libraries and services`_.

Matter
------

* The CHIP Tool controller is now the recommended controller for Matter.
  The documentation about :ref:`ug_matter_configuring_controller` has been updated accordingly.
  For more information about the CHIP Tool controller, read the :doc:`matter:chip_tool_guide` page in the Matter documentation.

Thread
------

* Added support for the Link Metrics and CSL Thread v1.2 features for the nRF53 Series devices.
* Removed support for the :ref:`thread_architectures_designs_cp_ncp` architecture and the related tools.
* Memory requirements page shows requirements based on the configuration used for certification instead of minimal configuration which has been removed.

Zigbee
------

* Added:

  * Zigbee timer implementation, based on Zephyr's system time.
    This timer implementation is enabled by default in the :ref:`Zigbee NCP sample <zigbee_ncp_sample>` and when using ZBOSS development libraries.
  * :kconfig:option:`CONFIG_ZIGBEE_NVRAM_PAGE_COUNT` Kconfig option to set the number of ZBOSS NVRAM virtual pages.
  * RAM-based ZBOSS NVRAM implementation.
    This implementation must not be used in production.
    It is meant to be used only for debugging purposes.
  * Documentation page about :ref:`ug_zigbee_commissioning`.
  * Experimental support for Zigbee Green Power Combo Basic functionality.
  * Zigbee device definition for each Zigbee sample and application.

* Updated:

  * :ref:` Zigbee shell library <lib_zigbee_shell>`.
    For details, see `Libraries for Zigbee`_.
  * Return code in the ZCL callbacks from :c:macro:`RET_ERROR` to :c:macro:`RET_NOT_IMPLEMENTED` if the device does not handle the ZCL command.
  * Enabled the ZCL8 compatibility mode :c:macro:`ZB_ZCL_AUTO_MODE` inside the :c:func:`zigbee_default_signal_handler`.
  * Production version of :ref:`nrfxlib:zboss` from v3.11.1.0 to v3.11.2.0 and platform v5.1.2 (``v3.11.2.0+v5.1.2``).
  * Development version of :ref:`nrfxlib:zboss` from v3.11.1.177 to v3.12.1.0 and platform v5.2.0 (``v3.12.1.0+v5.2.0``).
  * :ref:`ZBOSS Network Co-processor Host <ug_zigbee_tools_ncp_host>` package to the new version v2.2.0.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Support for :ref:`Bosch Software Environmental Cluster (BSEC) library <bosch_software_environmental_cluster_library>` driver.
  * Support for Indoor Air Quality (IAQ) readings retrieved from the BME680 sensor on Thingy:91. For more information, see the :ref:`asset_tracker_v2_sensor_module`.
  * Support for QEMU x86 emulation.
  * Support for the :ref:`lib_nrf_cloud_pgps` flash memory partition under certain conditions.
  * Support for :ref:`QoS` library to handle multiple in-flight messages for MQTT based cloud backends such as AWS IoT, Azure IoT Hub, and nRF Cloud.

* Updated:

  * For nRF Cloud builds, the configuration section in the shadow is now initialized during the cloud connection process.
  * Allow the :ref:`lib_nrf_cloud` library to handle modem FOTA updates if :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` is enabled.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * ``#XDFUGET`` and ``#XDFURUN`` AT commands to support the cloud-to-nRF52 DFU service.
  * Native TLS support to the HTTPS client.
  * ``#XCMNG`` command to support the use of native TLS.
  * ``#XSOCKETSELECT`` AT command to support multiple sockets in the Socket service.
  * ``#XPOLL`` AT command to poll selected or all sockets for incoming data.

* Updated:

  * Enhanced the ``#XHTTPCREQ`` AT command for better HTTP upload and download support.
  * Enhanced the ``#XSLEEP`` AT command to support data indication when idle.
  * Enhanced the MQTT client to support the reception of large PUBLISH payloads.

* Fixed:

  * The secondary MCUboot partition information is no longer passed to the P-GPS library if the P-GPS partition is enabled.
  * The combined use of A-GPS and P-GPS so that ephemeris and almanac data is not requested via A-GPS, saving both power and bandwidth.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Increased the value of :kconfig:option:`CONFIG_CAF_POWER_MANAGER_TIMEOUT` to 30 seconds for Thingy:53.

nRF Desktop
-----------

* Added documentation for selective HID report subscription in :ref:`nrf_desktop_usb_state` using :ref:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION <config_desktop_app_options>` option.

Thingy:53 Zigbee weather station
--------------------------------

* Added new :ref:`zigbee_weather_station_app` application.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added :ref:`peripheral_ams_client` sample.

* Updated:

  * :ref:`direct_test_mode` sample:

    * Updated:

      * The front-end module test parameters are not set to their default value after the DTM reset command anymore.
      * Added the vendor-specific ``FEM_DEFAULT_PARAMS_SET`` command for restoring the default front-end module parameters.

    * Fixed handling of the disable Constant Tone Extension command.

  * :ref:`peripheral_hids_mouse` sample:

    * Increased the main stack size from 1024 to 1536 bytes.
    * Increased the stack size of the nRF RPC threads from 1024 to 1280 bytes.

  * :ref:`peripheral_uart` sample:

    * Fixed handling of RX buffer releasing in this sample and in the UARTE driver.
      Before this fix, the RX buffer might have been released twice by the main thread.

  * :ref:`central_uart` sample:

    * Fixed handling of RX buffer releasing in this sample and in the UARTE driver.
      Before this fix, the RX buffer might have been released twice by the main thread.
    * Added debug logs for the UART events.

Bluetooth mesh samples
----------------------

* Updated:

  * All samples to use the Partition Manager, replacing the use of the Device Tree Source flash partitions.
  * :ref:`bluetooth_mesh_sensor_server` sample:

    * Definitions for sensor callbacks now include the :c:struct:`bt_mesh_sensor_srv` context.

nRF9160 samples
---------------

* Added:

  * :ref:`nrf_cloud_rest_device_message` sample, demonstrating how to send an arbitrary device message with the nRF Cloud REST API.
  * :ref:`modem_callbacks_sample` sample, showcasing initialization and de-initialization callbacks.
  * :ref:`nrf_cloud_mqtt_multi_service` sample, demonstrating a simple but robust integration of location services, FOTA, sensor sampling, and more.
  * Shell functionality to HTTP Update samples.
  * :ref:`nrf_cloud_rest_cell_pos_sample` sample, demonstrating how to use the :ref:`lib_nrf_cloud_rest` library to perform cellular positioning requests.

* Updated:

  * :ref:`at_monitor_sample` sample:

    * Added ``denied``, ``unknown``, ``roaming``, and ``UICC failure`` CEREG status codes to :c:func:`cereg_mon`.

  * :ref:`modem_shell_application` sample:

    * Added:

      * Remote control support over MQTT using the :guilabel:`Terminal` window in the nRF Cloud portal.
        It enables executing any MoSh command on the device remotely.
      * An option ``--interval`` (in seconds) to neighbor cell measurements in continuous mode  (``link ncellmeas --continuous``).
        When using this option, a new measurement is started in each interval.
      * Separate plain AT command mode that can be started with the command ``at at_cmd_mode start``.
        AT command termination methods can be configured using Kconfig options.
        The default method is CR termination.
        In AT command mode, a maximum of 10 AT commands can be pipelined with ``|`` as the delimiter character between pipelined AT commands.
      * Threading support for the ``ping`` command.
      * Iperf3 usage over Zephyr native TCP/IP stack and nRF9160 LTE default context.
      * Support for the GNSS features introduced in modem firmware v1.3.2.
        This includes several new fields in the PVT notification and a command to query the expiry times of assistance data.
      * Support for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.

  * :ref:`nrf_cloud_rest_fota` sample:

    * Updated:

      * Enabled building of bootloader FOTA update files.
      * Corrected handling of the bootloader FOTA updates.
      * Enabled the :ref:`lib_at_host` library to make it easier to update certificates.

  * :ref:`lwm2m_client` sample:

    * Added:

      * Minimal Portfolio object support that is required for LwM2M conformance testing.
      * Support for using :ref:`location_assistance` with Coiote LwM2M server.
      * Guidelines on :ref:`setting up the sample for production <lwm2m_client_provisioning>` using AVSystem’s Coiote Device Management server.
      * :ref:`state_diagram` to the sample documentation.

    * Updated:

      * Reworked the retry logic so that the sample can fall back to bootstrap mode and need not always restart the LTE connection.
      * Replaced the deprecated GPS-driver with the new GNSS interface.

  * :ref:`download_sample` sample:

    * Updated the default HTTPS URL and certificate due to the old link being broken.

  * :ref:`gnss_sample` sample:

    * Added support for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
    * Enabled the :ref:`lib_at_host` library to make it easier to update certificates.

  * :ref:`location_sample` sample:

    * Enabled the :ref:`lib_at_host` library to make it easier to update certificates.

Thread samples
--------------

* Updated:

  * Thread 1.2 version made the default configuration option.
  * Thread Beacon payload has been removed after changes in latest Thread Specification.
  * The relevant sample documentation pages with information about support for :ref:`Trusted Firmware-M <ug_tfm>`.
  * All sample documentation with a Configuration section, and organized relevant information under that section.
  * Mnimal configuration for CLI sample has been removed.

Matter samples
--------------

* Added release configuration for all samples.

* Updated:

  * All ZAP configurations due to changes in Matter upstream.
  * :ref:`matter_template_sample`:

    * Added OTA DFU support.

  * :ref:`matter_light_switch_sample` sample:

    * Added:

      * A binding cluster to the sample.
      * Groupcast communication.

    * Updated the Pairing process to Binding process in the sample.

  * :ref:`matter_lock_sample` sample:

    * Added support for the Door Lock cluster, which replaced the previous temporary solution based on the On/Off cluster.

NFC samples
-----------

* Updated:

  * :ref:`nfc_tag_reader` sample:

    * Skip NDEF content printing when message parsing returns an error.

nRF5340 samples
---------------

* Added:

  * :ref:`nrf5340_remote_shell` sample.
  * :ref:`nrf5340_multicore` sample.

* Updated:

  * Changed the transport layer for inter-core communication on the nRF5340 device from the RPMsg to the IPC service library.
    The IPC service library can work with different transport backends and uses the RPMsg backend with static VRINGs by default.
    This transport layer change affects all samples that use Bluetooth HCI driver over RPMsg, 802.15.4 spinel backend over IPC or nRF RPC libraries.

Gazell samples
--------------

* Updated the documentation by separating the :ref:`gazell_samples` into their own pages for Host and Device.
  There are now four different sample pages, where each Host sample must be used along with its corresponding Device sample.

Zigbee samples
--------------

* Added support for the factory reset functionality from :ref:`lib_zigbee_application_utilities` in the following samples:

  * :ref:`zigbee_light_bulb_sample`
  * :ref:`zigbee_light_switch_sample`
  * :ref:`zigbee_network_coordinator_sample`
  * :ref:`zigbee_shell_sample`
  * :ref:`zigbee_template_sample`

* Added identify handler in the :ref:`Zigbee Light Switch sample <zigbee_light_switch_sample>`.

Other Samples
-------------

* Added:

  * :ref:`ipc_service_sample` sample that demonstrates throughput of the IPC service with available backends.
  * :ref:`event_manager_proxy_sample` sample, which demonstrates the usage of :ref:`event_manager_proxy`.
  * :ref:`caf_sensor_manager_sample` for single-core and multi-core SoCs.

* Updated:

  * :ref:`radio_test`:

    * Added new configuration that builds the sample with support for remote IPC Service shell on nRF5340 application core through USB.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Removed the deprecated GPS driver.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* Added :ref:`ams_client_readme` library.

* Updated:

  * :ref:`gatt_dm_readme` library:

    * Added option to discover several service instances using the UUID.
    * Fixed discovery of empty services.

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

    * Fixed:

      * Serialization of the write callback applied to the GATT attribute.
      * Serialization of the :c:func:`bt_gatt_service_unregister` function call.

Modem libraries
---------------

* Added:

  * :ref:`lib_modem_antenna` library:

    * A new library for setting the antenna configuration on an nRF9160 DK or a Thingy:91.

* Updated:

  * :ref:`sms_readme` library:

    * Added handling for SMS client unregistration notification from the modem.
      When the notification is received, the library re-registers the SMS client automatically.

  * :ref:`lib_location` library:

    * Added:

      * Support for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
      * Improved integration of A-GPS and P-GPS when both are enabled.
      * A missing call to the :c:func:`nrf_cloud_pgps_notify_prediction` function, when using the REST interface with P-GPS.
      * Support for P-GPS data retrieval from an external source, implemented separately by the application.
        Enabled by setting the :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL` option.
        The library triggers a :c:enum:`LOCATION_EVT_GNSS_PREDICTION_REQUEST` event when assistance is needed.
      * Obstructed satellite visibility detection feature for GNSS.
        When this feature is enabled, the library tries to detect occurrences where getting a GNSS fix is unlikely or would consume a lot of energy.
        When such an occurrence is detected, GNSS is stopped without waiting for a fix or a timeout.
      * In addition to the current default fallback mode for acquiring a location, it can also be acquired using the :c:enumerator:`LOCATION_REQ_MODE_ALL` mode that runs all methods in the list sequentially.
        Each run method receives a location event, either a success or a failure.

    * Updated:

      * The :c:member:`request` member of the :c:struct:`location_event_data` structure renamed to :c:member:`agps_request`.
      * Current system time is attached to the ``location_datetime`` parameter of the location request response with Wi-Fi and cellular methods.
        The timestamp comes from the moment of scanning or neighbor measurements.
      * Removed dependency on the :ref:`lib_modem_jwt` library.
        The :ref:`lib_location` library now selects :kconfig:option:`CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT` when using :kconfig:option:`CONFIG_NRF_CLOUD_REST`.

  * :ref:`nrf_modem_lib_readme` library:

    * Added:

      * :c:macro:`NRF_MODEM_LIB_ON_INIT` macro for compile-time registration of callbacks on modem initialization.
      * :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macro for compile-time registration of callbacks on modem de-initialization.
      * :kconfig:option:`CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID` to enable logging for both FW version and UUID at the end of the library initialization step.
      * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING` to process modem traces in a thread (experimental).

    * Deprecated :c:func:`nrf_modem_lib_shutdown_wait` function, in favor of :c:macro:`NRF_MODEM_LIB_ON_INIT`.

  * :ref:`lte_lc_readme` library:

    * Added:

      * :c:macro:`LTE_LC_ON_CFUN` macro for compile-time registration of callbacks on modem functional mode changes using :c:func:`lte_lc_func_mode_set`.
      * Support for simple shell commands.

* Removed the deprecated A-GPS library.

Libraries for networking
------------------------

* Updated:

  * :ref:`lib_nrf_cloud_rest` library:

    * Added JSON Web Token (JWT) autogeneration feature.
      If enabled, the nRF Cloud REST library automatically generates a JWT if none is provided by the user when making REST requests.

    * Updated:

      * Centralized error handling.
      * Error return values.
      * For cellular positioning responses, the type is now set based on the ``fulfilledWith`` response from the nRF Cloud.
      * nRF Cloud error codes are now parsed and set in the :c:struct:`nrf_cloud_rest_context` structure.

  * :ref:`lib_download_client` library:

    * Fixed an issue where downloads of COAP URIs would fail when they contained multiple path elements.
    * Added the :c:member:`set_native_tls` parameter in the configuration structure to configure native TLS support at runtime.

  * :ref:`lib_fota_download` library:

    * Added :c:func:`fota_download_s0_active_get` function that gets the active B1 slot.
    * Added :kconfig:option:`CONFIG_FOTA_DOWNLOAD_NATIVE_TLS` to configure the socket to be native for TLS instead of offloading TLS operations to the modem.

  * :ref:`lib_nrf_cloud` library:

    * Added:

      * :c:func:`nrf_cloud_bootloader_fota_slot_set` function that sets the active bootloader slot flag during bootloader FOTA updates.
      * :c:func:`nrf_cloud_pending_fota_job_process` function that processes the state of pending FOTA jobs.
      * :c:func:`nrf_cloud_handle_error_message` function that handles error message responses (MQTT) from nRF Cloud.

    * Updated:

      * Shadow data behavior during the connection process.
        The data is now sent to the application even if no ``"config"`` section is present.
      * The application can now send shadow updates earlier in the connection process.
      * nRF Cloud error message responses to location service MQTT requests are now handled.
      * The value of the :kconfig:option:`NRF_CLOUD_HOST_NAME` option is now "mqtt.nrfcloud.com".

    * Fixed the validation of bootloader FOTA updates.

  * :ref:`lib_aws_iot` library:

    * Updated:

      * Renamed ``aws_iot_topic_type`` to ``aws_iot_shadow_topic_type`` and removed ``AWS_IOT_SHADOW_TOPIC_UNKNOWN``.

  * :ref:`lib_lwm2m_client_utils` library:

    * Added support for using location assistance when using Coiote LwM2M server.
    * Updated the library to store credentials and server settings permanently on bootstrap.

  * :ref:`lib_azure_iot_hub` library:

    * Added :kconfig:option:`CONFIG_AZURE_IOT_HUB_NATIVE_TLS` option to configure the socket to be native for TLS instead of offloading TLS operations to the modem.

 * :ref:`lib_nrf_cloud_pgps` library:

    * Added:

      * The passing of the current prediction, if one is available, along with the ``PGPS_EVT_READY`` event.
      * The following three ways to define the storage location in the flash memory:

        * A dedicated P-GPS partition, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
        * The use of the MCUboot secondary partition as storage, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_MCUBOOT_SECONDARY` option.
        * An application-specific storage, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_CUSTOM` option.

 * :ref:`lib_nrf_cloud_agps` library:

    * Fixed premature assistance suppression when the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED` option is enabled.
      Added a 10 minute margin of error to ensure A-GPS assistance is downloaded every two hours even if the modem requests assistance a little early.

Libraries for NFC
-----------------

* Updated:

  * :ref:`nfc_ndef_parser_readme`:

    * Updated:

      * :c:func:`nfc_ndef_msg_parse` with a fix to the declaration, a new assertion to avoid a potential usage fault, and added a note in the function documentation.
      * ``NFC_NDEF_PARSER_REQUIRED_MEMO_SIZE_CALC`` macro has been renamed to :c:macro:`NFC_NDEF_PARSER_REQUIRED_MEM`.

Other libraries
---------------

* Added:

  * :ref:`event_manager_proxy` library.
  * :ref:`QoS` library.


* Updated:

  * :ref:`app_event_manager`:

    * Added:

      * Event type flags to represent if event type should be logged, traced and has dynamic data.
        To update your application, pass a flag variable as a parameter in :c:macro:`APP_EVENT_TYPE_DEFINE` instead of ``init_log``.
        Use :c:macro:`APP_EVENT_FLAGS_CREATE` to set multiple flags:

        .. code-block:: c

           APP_EVENT_TYPE_DEFINE(my_event,
             log_my_event,
             &my_event_info,
             APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_1, APP_EVENT_TYPE_FLAGS_2));

      * :c:func:`app_event_manager_event_size` function with corresponding :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE` option.

      * Universal hooks for Application Event Manager initialization, event submission, preprocessing, and postprocessing.
        This includes implementation of macros that register hooks, grouped as follows:

        * :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST`,  :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_LAST`
        * :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_LAST`
        * :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST`

    * Updated:

      * Renamed Event Manager to Application Event Manager.

  * :ref:`app_event_manager_profiler_tracer`:

    * Updated:

      * The library is no longer directly referenced from the Application Event Manager.
        Instead, it uses the Application Event Manager hooks to connect with the manager.


  * :ref:`esb_readme`:

    * Fixed a compilation error for nRF52833.

  * Partition Manager:

    * Added the :file:`ncs/nrf/subsys/partition_manager/pm.yml.pgps` file.

Common Application Framework (CAF)
----------------------------------

* Added :ref:`caf_sensor_data_aggregator`, which buffers sensor events and sends them as packages to the listener.

Shell libraries
---------------

* Added :ref:`shell_ipc_readme`.


Libraries for Zigbee
--------------------

* Updated:

  * :ref:`lib_zigbee_application_utilities`:

    * Added factory reset functionality in :ref:`lib_zigbee_application_utilities`.

  * :ref:`lib_zigbee_shell`:

    * Added:

      * ``nbr monitor`` shell command for monitoring the list of active Zigbee neighbors.
      * Set of ``zcl groups`` shell commands for managing Zigbee groups.
      * :kconfig:option:`CONFIG_ZIGBEE_SHELL_ZCL_CMD_TIMEOUT` for timing out ZCL cmd commands.

    * Updated:

      * :ref:`lib_zigbee_shell` structure to make it an independent library.
      * File names ``zigbee_cli*`` and changed it to ``zigbee_shell*``.
      * Function names ``zigbee_cli*`` and changed it to ``zigbee_shell*``.
      * ``bdb factory_reset`` command.
        Now the command checks if the ZBOSS stack is started before performing the factory reset.
      * ``zcl cmd`` shell command extended to allow sending groupcasts.
      *  ``zdo`` shell commands extended to allow binding to a group address.
      * Internal context manager structures.

  * ref:`lib_zigbee_osif`:

    * Updated:

      * Crypto library used for performing software AES ecryption.
        Now, the :ref:`nrfxlib:nrf_oberon_readme` is used instead of the Tinycrypt library.

  * Fixed:

    * An issue where printing binding table containing group-binding entries results in corrupted output.
    * An issue where Zigbee shell coordinator would not form a new network after the factory reset operation.


sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Unity
-----

|no_changes_yet_note|

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

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``aa566b02070252043015a90946fea0685103d7dd``.

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

* Fixed:

  * NCSDK-13949 known issue where the TF-M Secure Image would copy FICR to RAM on nRF9160.
  * NCSDK-12306 known issue where a usage fault would be triggered in the debug build on nRF9160.
  * NCSDK-14015 known issue that would cause crash during boot when the :kconfig:option:`CONFIG_RPMSG_SERVICE` Kconfig option is enabled on the nRF5340 SoC.

cJSON
=====

*  Fixed an issue with floats in the cJSON module when using NEWLIB_LIBC without the :kconfig:option:`CONFIG_NEWLIB_LIBC_FLOAT_PRINTF` Kconfig option.

Documentation
=============

* Added:

  * Documentation for :ref:`debugging of nRF5340 <debugging>` in :ref:`working with nRF5340 DK<ug_nrf5340>` user guide.
  * Section describing how to enable Amazon Frustration-Free Setup (FFS) support in :ref:`ug_matter_configuring_device_identification`.
  * Notes to the :ref:`bluetooth_central_dfu_smp` sample document specifying the intended use of the sample.
  * DevAcademy links to the :ref:`index` and :ref:`getting_started` pages.
  * Additional user guidance to the :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages and the corresponding Developing with pages.

* Updated:

  * :ref:`ug_matter_architecture` with updated protocol architecture diagram.
  * :ref:`app_memory` with sections for Matter and Zigbee.
  * :ref:`thread_ug_feature_updating_libs` to clarify the use, and added |VSC| instructions.
  * :ref:`ug_thread_communication` by moving it to a separate page instead of it being under :ref:`ug_thread_architectures`.
  * Added a note to several nRF Cloud samples using the `nRF Cloud REST API`_ indicating the need for valid and registered request signing credentials.

* Removed:

  * Documentation on the Getting Started Assistant, as this tool is no longer in use.
    Linux users can install the |NCS| by using the `Installing using Visual Studio Code <Installing on Linux_>`_ instructions or by following the steps on the :ref:`gs_installing` page.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
