.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.1.99
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
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and other components that implement or support protocol functionality.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.1.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Added macOS 26 support (Tier 3) to the table listing :ref:`supported operating systems for proprietary tools <additional_nordic_sw_tools_os_support>`.
* Updated:

  * The required `SEGGER J-Link`_ version to v8.66.
  * Steps on the :ref:`install_ncs` page for installing the |NCS| and toolchain together.
    With this change, the separate steps to install the toolchain and the SDK were merged into a single step.

Board support
=============

* Added support for the nRF7002-EB II Wi-Fi shield for use with the nRF54LM20 DK board target.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

* Added an option to restore progress after a power failure when using DFU multi-image with MCUboot.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

|no_changes_yet_note|

Developing with nRF54H Series
=============================

|no_changes_yet_note|

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Thingy:91 X
===========================

|no_changes_yet_note|

Developing with Thingy:91
=========================

* Updated the title of the page about updating the Thingy:91 firmware using the Cellular Monitor app to :ref:`thingy91_update_firmware`.
* Removed the page about updating the Thingy:91 firmware using the Programmer app.
  Its contents are now available in the app documentation on the `Programming Nordic Thingy prototyping platforms`_ page.
  The :ref:`thingy91_partition_layout` section has been moved to the :ref:`thingy91_update_firmware` page.

Developing with Thingy:53
=========================

|no_changes_yet_note|

Developing with PMICs
=====================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

|no_changes_yet_note|

Developing with custom boards
=============================

|no_changes_yet_note|

Developing with coprocessors
============================

|no_changes_yet_note|

Security
========

* Added:

  * CRACEN and nrf_oberon driver support for nRF54LM20.
    For the list of supported features and limitations, see the :ref:`ug_crypto_supported_features` page.

  * Support for disabling Internal Trusted Storage (ITS) on nRF54L series devices when using
    :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO` with Trusted Firmware-M (TF-M) through the
    :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` Kconfig option.

  * Support for AES in counter mode using CRACEN for the :zephyr:board:`nrf54lm20dk`.

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

* Updated the NLC profile configuration system:

  * Introduced individual profile configuration options for better user control.
  * Deprecated the ``CONFIG_BT_MESH_NLC_PERF_CONF`` and ``CONFIG_BT_MESH_NLC_PERF_DEFAULT`` Kconfig options.
    Existing configurations continue to work but you should migrate to individual profile options.

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

* Added the :ref:`esb_monitor_mode` feature.

Gazell
------

|no_changes_yet_note|

Matter
------

|no_changes_yet_note|

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

* Updated the :ref:`thread_sed_ssed` documentation to clarify the impact of the SSED configuration on the device's power consumption and provide a guide for :ref:`thread_ssed_fine_tuning` of SSED devices.

Wi-Fi®
------

|no_changes_yet_note|

Applications
============

|no_changes_yet_note|

Connectivity bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

* Updated the application to store a portion of the application code related to the nRF70 Series Wi-Fi firmware in the external flash memory by default.
  This change breaks the DFU between the previous |NCS| versions and the |NCS| v3.2.0.
  To fix this, you need to disable storing the Wi-Fi firmware patch in external memory.
  See the :ref:`migration guide <migration_3.2_required>` for more information.

nRF5340 Audio
-------------

* Added:

  * The :ref:`Audio application API documentation <audio_api>` page.
  * The :ref:`config_audio_app_options` page.

* Updated:

  * The power measurements to be disabled by default in ``debug`` builds.
    To enable power measurements, set the :kconfig:option:`CONFIG_NRF5340_AUDIO_POWER_MEASUREMENT` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file.
  * The audio application targeting the :zephyr:board:`nrf5340dk` to use pins **P1.5** to **P1.9** for the I2S interface instead of **P0.13** to **P0.17**.
    This change was made to avoid conflicts with the onboard peripherals on the nRF5340 DK.
  * The API documentation in the header files listed on the :ref:`audio_api` page.

nRF Desktop
-----------

  * Updated:

    * The memory layouts for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to make more space for the application code.
      This change in the partition map of every nRF54LM20 configuration is a breaking change and cannot be performed using DFU.
      As a result, the DFU procedure will fail if you attempt to upgrade the application firmware based on one of the |NCS| v3.1 releases.
    * The application and MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
      The application image signature is verified with the CRACEN hardware peripheral.
    * The MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the KMU-based key storage.
      The public key used by MCUboot for validating the application image is securely stored in the KMU hardware peripheral.
      To simplify the programming procedure, the application is configured to use the automatic KMU provisioning.
      The KMU provisioning is performed by the west runner as a part of the ``west flash`` command when the ``--erase`` or ``--recover`` flag is used.
    * Application configurations to avoid using the deprecated Kconfig options :ref:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE <config_desktop_app_options>`.
      The configurations rely on Kconfig options specific to HID providers instead.
      The HID keypress queue sizes for HID consumer control (:ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>`) and HID system control (:ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>`) reports were decreased to ``10``.
    * Application configurations integrating the USB legacy stack (:ref:`CONFIG_DESKTOP_USB_STACK_LEGACY <config_desktop_app_options>`) to suppress build warnings related to deprecated APIs of the USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
      The configurations enable the :kconfig:option:`CONFIG_DEPRECATION_TEST` Kconfig option to suppress the deprecation warnings.
      The USB legacy stack is still used by default.
    * MCUboot configurations that support serial recovery over USB CDC ACM to enable the :kconfig:option:`CONFIG_DEPRECATION_TEST` Kconfig option to suppress deprecation warnings.
      The implementation of serial recovery over USB CDC ACM still uses the deprecated APIs of the USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
    * Configurations of the ``nrf52840dongle/nrf52840`` board target to align them after the ``bare`` variant of the board was introduced in Zephyr.
      The application did not switch to the ``bare`` board variant to keep backwards compatibility.
    * The :ref:`nrf_desktop_hid_state` to allow for delayed registration of HID report providers.
      Before the change was introduced, subscribing to a HID input report before the respective provider was registered triggered an assertion failure.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the application to change the default libc from the :ref:`zephyr:c_library_newlib` to the :ref:`zephyr:c_library_picolibc` to align with the |NCS| and Zephyr.

* Removed support for the ``thingy53/nrf5340/cpuapp/ns`` build target.

Serial LTE modem
----------------

* Updated to use the new ``SEC_TAG_TLS_INVALID`` definition as a placeholder for security tags.


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

* Added the :ref:`samples_test_app` application to demonstrate how to use the Bluetooth LE Test GATT Server and test Bluetooth LE functionality in peripheral samples.

* Updated the network core image applications for the following samples from the :zephyr:code-sample:`bluetooth_hci_ipc` sample to the :ref:`ipc_radio` application for multicore builds:

  * :ref:`bluetooth_conn_time_synchronization`
  * :ref:`bluetooth_iso_combined_bis_cis`
  * :ref:`bluetooth_isochronous_time_synchronization`
  * :ref:`bt_scanning_while_connecting`
  * :ref:`channel_sounding_ras_initiator`
  * :ref:`channel_sounding_ras_reflector`

  The :ref:`ipc_radio` application is commonly used for multicore builds in other |NCS| samples and projects.
  Hence, this is to align with the common practice.

* Removed support for the ``thingy53/nrf5340/cpuapp/ns`` build target from the following samples:

   * :ref:`peripheral_lbs`
   * :ref:`peripheral_status`
   * :ref:`peripheral_uart`

* Disabled legacy pairing in the following samples:

   * :ref:`central_nfc_pairing`
   * :ref:`power_profiling`

   Support for legacy pairing remains exclusively for :ref:`peripheral_nfc_pairing` sample to retain compatibility with older Andorid devices.

* :ref:`direct_test_mode` sample:

  * Updated by simplifying the 2-wire UART polling.
    This is done by replacing the hardware timer with the ``k_sleep()`` function.

Bluetooth Mesh samples
----------------------

* :ref:`ble_mesh_dfu_distributor` sample:

  * Added support for external flash memory for the ``nrf52840dk/nrf52840`` as the secondary partition for the DFU process.

* :ref:`ble_mesh_dfu_target` sample:

  * Added support for external flash memory for the ``nrf52840dk/nrf52840`` as the secondary partition for the DFU process.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added polling toggle to **Button 1** (**Button 0** on nRF54 DKs) to start/stop the periodic Sensor Get loop, ensuring the functionality is available on all supported devices including single-button hardware.

  * Updated:

    * To demonstrate the Bluetooth :ref:`ug_bt_mesh_nlc` HVAC Integration profile.
    * The following Mesh samples to use individual NLC profile configurations instead of the deprecated options:

      * :ref:`bluetooth_mesh_light_dim`
      * :ref:`bluetooth_mesh_light_lc`
      * :ref:`bluetooth_mesh_sensor_server`
      * :ref:`bluetooth_mesh_sensor_client`

    * Button functions.
      Assignments are shifted down one index to accommodate the new polling toggle.
      The descriptor action has been removed from button actions but is still available through mesh shell commands.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated:

    * The memory layout for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to make more space for the application code.
      This change in the nRF54LM20 partition map is a breaking change and cannot be performed using DFU.
      As a result, the DFU procedure will fail if you attempt to upgrade the sample firmware based on one of the |NCS| v3.1 releases.
    * The application and MCUBoot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
      Note, that the Fast Pair subsystem still uses the Oberon software library.
      The application image signature is verified with the CRACEN hardware peripheral.
    * The MCUBoot configuration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the KMU-based key storage.
      The public key used by MCUboot for validating the application image is securely stored in the KMU hardware peripheral.
      To simplify the programming procedure, the samples are configured to use the automatic KMU provisioning.
      The KMU provisioning is performed by the west runner as a part of the ``west flash`` command when the ``--erase`` or ``--recover`` flag is used.

* :ref:`fast_pair_input_device` sample:

  * Updated the application configuration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
    Note, that the Fast Pair subsystem still uses the Oberon software library.

Cellular samples
----------------

* Added:

  * The :ref:`nrf_cloud_coap_cell_location` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for nRF Cloud's cellular location service.
  * The :ref:`nrf_cloud_coap_fota_sample` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for FOTA updates.
  * The :ref:`nrf_cloud_coap_device_message` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for device messages.

* :ref:`nrf_cloud_rest_cell_location` sample:

  * Added runtime setting of the log level for the nRF Cloud logging feature.

* Updated the following samples to use the new ``SEC_TAG_TLS_INVALID`` definition:

  * :ref:`modem_shell_application`
  * :ref:`http_application_update_sample`
  * :ref:`http_modem_delta_update_sample`
  * :ref:`http_modem_full_update_sample`

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for environment evaluation using the ``link enveval`` command.
    * Support for NTN NB-IoT to the ``link sysmode`` and ``link edrx`` commands.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed an issue where sporadically the application was stuck waiting for the device to connect to the internet.
    This was due to wrong :ref:`Connection Manager <zephyr:conn_mgr_overview>` initialization.

Cryptography samples
--------------------

* Added the :ref:`crypto_kmu_usage_nrf54l` sample.

* :ref:`crypto_aes_ctr` sample:

  * Added support for ``nrf54lm20dk/nrf54lm20a/cpuapp``.

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

|no_changes_yet_note|

DFU samples
-----------

* Added:

  * The :ref:`dfu_multi_image_sample` sample to demonstrate how to use the :ref:`lib_dfu_target` library.
  * The :ref:`ab_sample` sample to demonstrate how to implement the A/B firmware update strategy using :ref:`MCUboot <mcuboot_index_ncs>`.

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

* Added the :ref:`esb_monitor` sample to demonstrate how to use the :ref:`ug_esb` protocol in Monitor mode.

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Added:

  * The :ref:`matter_temperature_sensor_sample` sample that demonstrates how to implement and test a Matter temperature sensor device.
  * The :ref:`matter_contact_sensor_sample` sample that demonstrates how to implement and test a Matter contact sensor device.

* Updated all Matter over Wi-Fi samples and applications to store a portion of the application code related to the nRF70 Series Wi-Fi firmware in the external flash memory by default.
  This change breaks the DFU between the previous |NCS| versions and the |NCS| v3.2.0.
  To fix this, you need to disable storing the Wi-Fi firmware patch in external memory.
  See the :ref:`migration guide <migration_3.2_required>` for more information.

* :ref:`matter_lock_sample` sample:

   * Added a callback for the auto-relock feature.
     This resolves the :ref:`known issue <known_issues>` KRKNWK-20691.

Networking samples
------------------

* Added support for the nRF7002-EB II with the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target in the following samples:

  * :ref:`aws_iot`
  * :ref:`net_coap_client_sample`
  * :ref:`download_sample`
  * :ref:`http_server`
  * :ref:`https_client`
  * :ref:`mqtt_sample`
  * :ref:`udp_sample`

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

|no_changes_yet_note|

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

|no_changes_yet_note|

Wi-Fi samples
-------------

* Removed support for the nRF7002-EB II with the ``nrf54h20dk/nrf54h20/cpuapp`` board target from the following samples:

  * :ref:`wifi_station_sample`
  * :ref:`wifi_scan_sample`
  * :ref:`wifi_shell_sample`
  * :ref:`wifi_radio_test`
  * :ref:`ble_wifi_provision`
  * :ref:`wifi_provisioning_internal_sample`

Other samples
-------------

* :ref:`nrf_profiler_sample` sample:

  * Added a new testing step demonstrating how to calculate event propagation statistics.
    Also added the related test preset for the :file:`calc_stats.py` script (:file:`nrf/scripts/nrf_profiler/stats_nordic_presets/nrf_profiler.json`).

* :ref:`app_event_manager_profiling_tracer_sample` sample:

  * Added a new testing step demonstrating how to calculate event propagation statistics.
    Also added the related test preset for the :file:`calc_stats.py` script (:file:`nrf/scripts/nrf_profiler/stats_nordic_presets/app_event_manager_profiler_tracer.json`).

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Flash drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`hids_readme` library:

  * Updated the report length of the HID boot mouse to ``3``.
    The :c:func:`bt_hids_boot_mouse_inp_rep_send` function only allows to provide the state of the buttons and mouse movement (for both X and Y axes).
    No additional data can be provided by the application.

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

* :ref:`lte_lc_readme` library:

  * Added:

    * Support for environment evaluation.
    * Support for NTN NB-IoT system mode.
    * eDRX support for NTN NB-IoT.
    * Support for new modem events :c:enumerator:`LTE_LC_MODEM_EVT_RF_CAL_NOT_DONE`, :c:enumerator:`LTE_LC_MODEM_EVT_INVALID_BAND_CONF`, and :c:enumerator:`LTE_LC_MODEM_EVT_DETECTED_COUNTRY`.
    * Description of new features supported by mfw_nrf91x1 and mfw_nrf9151-ntn in receive only functional mode.
    * Sending of the ``LTE_LC_EVT_PSM_UPDATE`` event with ``tau`` and ``active_time`` set to ``-1`` when registration status is ``LTE_LC_NW_REG_NOT_REGISTERED``.

  * Updated:

    * The type of the :c:member:`lte_lc_evt.modem_evt` field to :c:struct:`lte_lc_modem_evt`.
    * Replaced modem events ``LTE_LC_MODEM_EVT_CE_LEVEL_0``, ``LTE_LC_MODEM_EVT_CE_LEVEL_1``, ``LTE_LC_MODEM_EVT_CE_LEVEL_2`` and ``LTE_LC_MODEM_EVT_CE_LEVEL_3`` with the :c:enumerator:`LTE_LC_MODEM_EVT_CE_LEVEL` modem event.
    * The order of the ``LTE_LC_MODEM_EVT_SEARCH_DONE`` modem event, and registration and cell related events.
      See the :ref:`migration guide <migration_3.2_required>` for more information.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* Added missing brackets that caused C++ compilation to fail in the following libraries:

  * :ref:`lib_nrf_cloud_pgps`
  * :ref:`lib_nrf_cloud_fota`

* Updated the following libraries to use the new ``SEC_TAG_TLS_INVALID`` definition for checking whether a security tag is valid:

  * :ref:`lib_aws_fota`
  * :ref:`lib_fota_download`
  * :ref:`lib_ftp_client`

* :ref:`lib_nrf_provisioning` library:

  * Added a blocking call to wait for a functional-mode change, relocating the logic from the app into the library.

  * Updated:

    * By making internal scheduling optional.
      Applications can now trigger provisioning manually using the :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` Kconfig option.
    * By moving root CA provisioning to modem initialization callback to avoid blocking and ensure proper state.
    * By expanding the event handler to report more provisioning events, including failures.
    * By making the event handler callback mandatory to notify the application of failures and prevent silent errors.
    * By unifying the device‐mode and modem‐mode callbacks into a single handler for cleaner integration.
    * The documentation and sample code accordingly.

  * Fixed multiple bugs and enhanced error handling.

* :ref:`lib_nrf_cloud_rest` library:

  * Deprecated the library.
    Use the :ref:`lib_nrf_cloud_coap` library instead.

* :ref:`lib_nrf_cloud_fota` library:

  * Fixed occasional message truncation notifying that the download was complete.

* :ref:`lib_nrf_cloud_log` library:

  * Updated by adding a missing CONFIG prefix.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* :ref:`nrf_profiler` library:

  * Updated the documentation by separating out the :ref:`nrf_profiler_script` documentation.

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

* Added:

  * The :ref:`esb_sniffer_scripts` scripts for the :ref:`esb_monitor` sample.
  * The documentation page for :ref:`nrf_profiler_script`.
    The page also describes the script for calculating statistics (:file:`calc_stats.py`).

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Removed the Fast Pair TinyCrypt cryptographic backend (``CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT``), because the TinyCrypt library support was removed from Zephyr.
  You can use either the Fast Pair Oberon cryptographic backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON`) or the Fast Pair PSA cryptographic backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).

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

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``81315483fcbdf1f1524c2b34a1fd4de6c77cd0f4``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

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

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

* Updated:

  * The TF-M version to 2.2.0.
  * Documentation to clarify the support for TF-M on devices emulated using the nRF54L15 DK.
    nRF54L05 does not support TF-M.
    nRF54L10 supports TF-M experimentally.

Documentation
=============

* Updated:

  * The :ref:`emds_readme_application_integration` section in the :ref:`emds_readme` library documentation to clarify the EMDS storage context usage.
  * The Emergency data storage section in the :ref:`bluetooth_mesh_light_lc` sample documentation to clarify the EMDS storage context implementation and usage.
  * The :ref:`ble_mesh_dfu_distributor` sample documentation to clarify the external flash support.
  * The :ref:`ble_mesh_dfu_target` sample documentation to clarify the external flash support.
