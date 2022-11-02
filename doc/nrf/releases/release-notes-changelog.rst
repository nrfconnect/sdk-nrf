.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.1.99
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
See `known issues for nRF Connect SDK v2.1.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

|no_changes_yet_note|

For details, see :ref:`nrfxlib:softdevice_controller_changelog`.

Bluetooth mesh
--------------

|no_changes_yet_note|

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

Matter
------

* Added:

  * Documentation page about :ref:`ug_matter_overview_multi_fabrics` and entry about binding to :ref:`ug_matter_network_topologies_concepts`.
  * Documentation page about :ref:`ug_matter_overview_commissioning`, which is based on an earlier subsection of :ref:`ug_matter_overview_network_topologies`.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``bc6b43882a56ddb3e94d3e64956bd5f3292b4058``.

The following list summarizes the most important changes inherited from the upstream Matter:

* |no_changes_yet_note|

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

ESB
---

|no_changes_yet_note|

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

* Updated:

  * The application now uses the new LwM2M location assistance objects through the :ref:`lib_lwm2m_location_assistance` library.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * Added optional data modem flow control option :ref:`CONFIG_SLM_DATAMODE_URC <CONFIG_SLM_DATAMODE_URC>`.

* Updated:

  * Removed automatic quit of data mode in GNSS, FTP and HTTP services.

nRF5340 Audio
-------------

* Added:

  * Kconfig options for different sample rates and BAP presets.
  * Added Kconfig options for different sample rates and BAP presets.
  * Added bidirectional mode for the CIS mode.
  * Added a walkie talkie demo for bidirectional CIS.

* Updated:

  * LE Audio Controller Subsystem for nRF53 from version 3303 to version 3307.
  * Removed support for the nRF5340 Audio DK (PCA10121) board version 0.7.1 or older

* Fixed:

  * An issue with the figure for :ref:`nrf53_audio_app_overview_architecture_i2s` in the :ref:`nrf53_audio_app` documentation.
    The figure now correctly shows the interaction with the Bluetooth modules.
  * SMP is not advertising in CIS mode.
  * MCUMGR command can't receive in BIS mode.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Thingy:52 (``thingy52_nrf52832``) is no longer supported.

nRF Desktop
-----------

* The UUID16 values of GATT Human Interface Device Service (HIDS) and GATT Battery Service (BAS) are moved from advertising data to scan response data.

Thingy:53 Zigbee weather station
--------------------------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`peripheral_cgms` sample.

* :ref:`ble_throughput` sample:

  * Added terminal commands for selecting the role.
  * Updated the ASCII art used for showing progress to feature the current Nordic Semiconductor logo.

* :ref:`peripheral_fast_pair` sample:

  * Switched to using :ref:`bt_le_adv_prov_readme` to generate Bluetooth advertising and scan response data.
  * After the device reaches the maximum number of paired devices (:kconfig:option:`CONFIG_BT_MAX_PAIRED`), the device stops looking for new peers.
    Therefore, the Fast Pair payload is no longer included in the advertising data.

* :ref:`peripheral_mds` sample:

  * Changed:

    * Added a documentation section about testing with the `nRF Memfault for Android`_ and the `nRF Memfault for iOS`_ mobile applications to the documentation.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_light_switch`

  * Added:

    * Support for running the light switch as a Low Power node.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Added:

    * Ability to limit the reported temperatures based on :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` as a setting for the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor type.
    * Ability to persistently store the above-mentioned setting.
    * A sensor descriptor of the temperature sensor.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added:

    * Ability to use buttons to send get and set messages for a sensor setting, as well as a get message for a sensor descriptor.

nRF9160 samples
---------------

* :ref:`lwm2m_client` sample:

  * Added:

    * Ability to use buttons to generate location assistance requests.
    * Documentation on :ref:`lwmwm_client_testing_shell`.

  * Updated:

    * The sample now uses the new LwM2M location assistance objects through the :ref:`lib_lwm2m_location_assistance` library.
    * Removed all read callbacks from sensor code because of an issue of read callbacks not working properly when used with LwM2M observations.
      This is due to the fact that the engine does not know when data is changed.
    * Sensor samples are now enabled by default for Thingy:91 and disabled by default on nRF9160 DK.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Updated:

   * Sample moved from :file:`samples/nrf9160/nrf_cloud_rest_cell_pos` to :file:`samples/nrf9160/nrf_cloud_rest_cell_location`.

* :ref:`modem_shell_application` sample:

  * Added:

    * LED 1 (nRF9160 DK)/Purple LED (Thingy:91) is lit for five seconds indicating that a current location has been acquired by using the ``location get`` command.
    * Overlay files for nRF9160 DK with nRF7002 EK to enable Wi-Fi scanning support.
      With this configuration, you can, for example, obtain the current location by using the ``location get`` command.

* :ref:`location_sample`:

  * Added:

    * Overlay files for nRF9160 DK with nRF7002 EK to obtain the current location by using Wi-Fi scanning results.

Thread samples
--------------

* :ref:`ot_cli_sample` sample:

  * Removed:

    * Thread Certification support files in favor of regular sample overlays.

Matter samples
--------------

* Updated ZAP configuration of the samples to conform with device types defined in Matter 1.0 specification.

* :ref:`matter_light_bulb_sample`:

  * Introduced support for Matter over Wi-Fi on standalone ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached.

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

|no_changes_yet_note|

Wi-Fi samples
-------------

* Added:

  * :ref:`wifi_radio_test` sample with the radio test support and :ref:`subcommands for FICR/OTP programming <wifi_ficr_prog>`.
  * :ref:`wifi_scan_sample` sample that demonstrates how to scan for the access points.
  * :ref:`wifi_station_sample` sample that demonstrates how to connect the Wi-Fi station to a specified access point.
  * :ref:`wifi_provisioning` sample that demonstrates how to provision a device with Nordic Semiconductor's Wi-Fi chipsets over Bluetooth® Low Energy.

Other samples
-------------

* :ref:`esb_prx_ptx` sample:

  * Removed the FEM support section.
* Added :ref:`hw_id_sample` sample.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* |no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* Added:

  * :ref:`cgms_readme` library.

* :ref:`bt_le_adv_prov_readme`:

  * Added:

    * Google Fast Pair advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`).
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` option to TX power advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`).
      The option adds a predefined value to the TX power, that is included in the advertising data.
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_GAP_APPEARANCE_SD` option to GAP appearance data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_GAP_APPEARANCE`).
      The option can be used to move the GAP appearance value to the scan response data.
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME_SD` option to Bluetooth device name data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`).
      The option can be used to move the Bluetooth device name to the advertising data.

  * Changed :c:member:`bt_le_adv_prov_adv_state.bond_cnt` to :c:member:`bt_le_adv_prov_adv_state.pairing_mode`.
    The information about whether the advertising device is looking for a new peer is more meaningful for the Bluetooth LE data providers.

* :ref:`bt_mesh` library:

  * Bluetooth mesh client models updated to reflect the changed mesh shell module structure.
    All Bluetooth mesh model commands are now located under **mesh models** in the shell menu.

  * :ref:`bt_mesh_dk_prov` module: Changed the UUID generation to prevent trailing zeros in the UUID.

    Migration note: To retain the legacy generation of UUID, enable the option :kconfig:option:`CONFIG_BT_MESH_DK_LEGACY_UUID_GEN`.

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

* :ref:`nrf_bt_scan_readme`:

  * Added the ability to use the module when the Bluetooth Observer role is enabled.

* :ref:`bt_fast_pair_readme` service:

  * Disabled automatic security re-establishment request as a peripheral (:kconfig:option:`CONFIG_BT_GATT_AUTO_SEC_REQ`) to allow the Fast Pair Seeker to control the security re-establishment.
  * Added API to check Account Key presence (:c:func:`bt_fast_pair_has_account_key`).

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`modem_info_readme` library:

  * Removed:

    * :c:func:`modem_info_json_string_encode` and :c:func:`modem_info_json_object_encode` functions.
    * network_mode field from :c:struct:`network_param`.
    * ``MODEM_INFO_NETWORK_MODE_MAX_SIZE``.
    * ``CONFIG_MODEM_INFO_ADD_BOARD``.

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE` Kconfig option to override the IPC IRQ priority from the devicetree.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO` Kconfig option to configure the IPC IRQ priority when the Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE` is enabled.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE` Kconfig option to enable the measurement of the modem trace backend bitrate.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG` Kconfig option to enable logging of the modem trace backend bitrate.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG` Kconfig option to enable logging of the modem trace bitrate.

  * Updated:

    * The IPC IRQ priority is now set using the devicetree.
    * The EGU peripheral is no longer used to generate software interrupts to process network data.
    * The :c:func:`getaddrinfo` function to return ``EAFNOSUPPORT`` instead of ``EPROTONOSUPPORT`` when socket family is not supported.
    * The :c:func:`bind` function to return ``EAFNOSUPPORT`` instead of ``ENOTSUP`` when socket family is not supported.
    * The :c:func:`sendto` function to return ``EAFNOSUPPORT`` instead of ``ENOTSUP`` when socket family is not supported.
    * The :c:func:`connect` function to not override the error codes set by the Modem library when called with raw parameters (non-IP).

  * Fixed:

    * An issue where the :c:func:`getsockopt` function causes segmentation fault when the ``optlen`` parameter is provided as ``NULL``.
    * An issue where the :c:func:`recvfrom` function causes segmentation fault when the ``from`` and ``fromlen`` parameters are provided as ``NULL``.

* :ref:`lib_location` library:

  * Added timeout for the entire location request.

Libraries for networking
------------------------

* :ref:`lib_multicell_location` library:

  * Removed the Kconfig option :kconfig:option:`CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS`.
    The maximum number of supported neighbor cell measurements for HERE location services depends on the :kconfig:option:`CONFIG_LTE_NEIGHBOR_CELLS_MAX` Kconfig option.

* :ref:`lib_download_client` library:

  * Updated the library so that it does not retry download on disconnect.
  * Fixed a race condition when starting the download.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * Added a possibility to override used default OS memory alloc/free functions.

  * Updated:

    * The stack size of the MQTT connection monitoring thread can now be adjusted by setting the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD_STACK_SIZE` Kconfig option.

  * Removed:

    * An unused parameter of the :c:func:`nrf_cloud_connect` function.

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for using X509 certificates.

* :ref:`lib_fota_download` library:

  * Added an error code :c:enumerator:`FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL` to indicate that the source of error is not network related.

* Added:

  * :ref:`lib_lwm2m_location_assistance` library that has support for using A-GPS, P-GPS and ground fix assistance from nRF Cloud using an LwM2M server.

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added:
  * :ref:`lib_sfloat` library.
  * :ref:`lib_hw_id` library to retrieve a unique hardware ID.

* :ref:`emds_readme`:

  * Removed the internal thread for storing the emergency data.
    The emergency data is now stored by the :c:func:`emds_store` function.
  * Changed the library implementation to bypass the flash driver when storing the emergency data.
    This allows calling the :c:func:`emds_store` function from an interrupt context.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_sensor_manager`:

  * Added:

    * Dynamic control for the :ref:`sensor sample period<sensor_sample_period>`.
    * Test for the sample.

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

* :ref:`west_sbom`:

  * SPDX License List database updated to version 3.18.

* :ref:`partition_manager`:

  * Added:

    * :kconfig:option:`CONFIG_PM_PARTITION_ALIGN_SETTINGS_STORAGE` Kconfig option to specify the alignment of the settings storage partition.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``13f63976bca672ee018f9d55f1e31f02f4135b64``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``b96453f653b674629c617bdbc810c3d9f7916ef4``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline b96453f653 ^45ef0d2

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^b96453f653

The current |NCS| main branch is based on revision ``b96453f653`` of Zephyr.

Additions specific to |NCS|
---------------------------

* |no_changes_yet_note|

zcbor
=====

* |no_changes_yet_note|

Trusted Firmware-M
==================

* |no_changes_yet_note|

cJSON
=====

* |no_changes_yet_note|

Documentation
=============

* Added:

  * :ref:`app_memory`: Configuration options affecting memory footprint for Bluetooth mesh, that can be used to optimize the application.
  * Documentation for the :ref:`lib_bh1749`.
  * The :ref:`ug_nrf52_gs` page.

* Updated:

  * :ref:`gs_assistant` steps to reflect the fact that the |nRFVSC| is the default recommended IDE.
  * :ref:`ug_matter_gs_adding_cluster` documentation with new code snippets to align it with the source code of refactored Matter template sample.
  * Split the existing Working with the nRF52 Series information into :ref:`ug_nrf52_features` and :ref:`ug_nrf52_developing`.
  * :ref:`ug_tfm` with improved TF-M logging documentation on getting the secure output on nRF5340 DK.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
