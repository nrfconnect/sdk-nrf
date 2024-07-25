.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.7.99
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
See `known issues for nRF Connect SDK v2.7.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

Build and configuration system
==============================

* Added the ``SB_CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM`` sysbuild Kconfig option to system to allow utilizing all available RAM when using TF-M on an nRF5340 device.

  .. note::
     This has security implications and may allow secrets to be leaked to the non-secure application in RAM.

Working with nRF91 Series
=========================

|no_changes_yet_note|

Working with nRF70 Series
=========================

|no_changes_yet_note|

Working with nRF54H Series
==========================

|no_changes_yet_note|

Working with nRF54L Series
==========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with RF front-end modules
=================================

|no_changes_yet_note|

Working with PMIC
=================

|no_changes_yet_note|

Security
========

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

|no_changes_yet_note|

Bluetooth® LE
-------------

* The correct SoftDevice Controller library :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE_MULTIROLE` will now be selected automatically when using coexistence based on :kconfig:option:`CONFIG_MPSL_CX` for nRF52-series devices.

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

* Added:

  * The Kconfig options to configure parameters impacting persistent subscriptions re-establishment:

    * :kconfig:option:`CONFIG_CHIP_MAX_ACTIVE_CASE_CLIENTS`
    * :kconfig:option:`CONFIG_CHIP_MAX_ACTIVE_DEVICES`
    * :kconfig:option:`CONFIG_CHIP_SUBSCRIPTION_RESUMPTION_MIN_RETRY_INTERVAL`
    * :kconfig:option:`CONFIG_CHIP_SUBSCRIPTION_RESUMPTION_RETRY_MULTIPLIER`


Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.3.0.0`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

Zigbee
------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Asset Tracker v2
----------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

|no_changes_yet_note|

Matter Bridge
-------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the application.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to application's location that contains the ZAP file and :file:`zap-generated` directory.
  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20`.

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Desktop
-----------

* Added a debug configuration enabling the `Fast Pair`_ feature on the nRF54L15 PDK with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.

* Updated:

  * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in configurations with the Fast Pair support.
    The value is now aligned with the Fast Pair requirements.
  * The :kconfig:option:`CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE` Kconfig option value in the nRF54L15 PDK configurations to ensure short write slots.
    It prevents timeouts in the MPSL flash synchronization caused by allocating long write slots while maintaining a Bluetooth LE connection with short intervals and no connection latency.


nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

|no_changes_yet_note|

Thingy:53: Matter weather station
---------------------------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the application.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to application's location that contains the ZAP file and :file:`zap-generated` directory.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Amazon Sidewalk samples
-----------------------

|no_changes_yet_note|

Bluetooth samples
-----------------

|no_changes_yet_note|

Bluetooth Fast Pair samples
---------------------------

* Updated:

  * The values for the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option in all configurations, and for the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL` Kconfig option in configurations with the Find My Device Network (FMDN) extension support.
    The values are now aligned with the Fast Pair requirements.

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Cellular samples
----------------

* :ref:`nrf_cloud_rest_fota` sample:

  * Added support for setting the FOTA update check interval using the config section in the shadow.

* :ref:`nrf_cloud_multi_service` sample:

  * Updated Wi-Fi overlays from newlibc to picolib.
  * Added the :kconfig:option:`CONFIG_TEST_COUNTER_MULTIPLIER` Kconfig option to multiply the number of test counter messages sent, for testing purposes.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Removed the dictionary-based comments in the :file:`overlay_nrfcloud_logging.conf` file.
    Dictionary-based logging is not available for the REST protocol at the moment.

Cryptography samples
--------------------

|no_changes_yet_note|

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

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

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the sample.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to sample's location that contains the ZAP file and :file:`zap-generated` directory.

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

* :ref:`802154_sniffer` sample:

  * Increased the number of RX buffers to reduce the chances of frame drops during high traffic periods.

PMIC samples
------------

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

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Updated the logging mode to minimal (:kconfig:option:`CONFIG_LOG_MODE_MINIMAL`) to reduce the sample's memory footprint and ensure no logging interference with the running benchmark.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Wi-Fi drivers
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

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option allowing the user to control the support for the Fast Pair subsequent pairing feature.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE` Kconfig choice option allowing the user to select their target Fast Pair use case.
      The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_UNKNOWN` and :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_LOCATOR_TAG` Kconfig options represent the supported use cases that can be selected as part of this Kconfig choice option.

* :ref:`bt_le_adv_prov_readme`:

  * Updated the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_SHOW_UI_PAIRING` Kconfig option and the :c:func:`bt_le_adv_prov_fast_pair_show_ui_pairing` function to require the enabling of the :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option.

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

Modem libraries
---------------

* :ref:`lte_lc_readme` library:

  * Removed:

    * The :c:func:`lte_lc_init` function.
      All instances of this function can be removed without any additional actions.
    * The :c:func:`lte_lc_deinit` function.
      Use the :c:func:`lte_lc_power_off` function instead.
    * The :c:func:`lte_lc_init_and_connect` function.
      Use the :c:func:`lte_lc_connect` function instead.
    * The :c:func:`lte_lc_init_and_connect_async` function.
      Use the :c:func:`lte_lc_connect_async` function instead.

* :ref:`nrf_modem_lib_lte_net_if` library:

  * Added a log warning suggesting a SIM card to be installed if a UICC error is detected by the modem.

Multiprotocol Service Layer libraries
-------------------------------------

* The Kconfig option ``CONFIG_MPSL_CX_THREAD`` has been renamed to :kconfig:option:`CONFIG_MPSL_CX_3WIRE` to better indicate multiprotocol compatibility.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud_rest` library:

  * Added the function :c:func:`nrf_cloud_rest_shadow_transform_request` to request shadow data using a JSONata expression.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * The function :c:func:`nrf_cloud_client_id_runtime_set` to set the device ID string if the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option is enabled.
    * The functions :c:func:`nrf_cloud_sec_tag_set` and :c:func:`nrf_cloud_sec_tag_get` to set and get the sec tag used for nRF Cloud credentials.

  * Updated:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option to be available with CoAP and REST.
    * The JSON string representing longitude in ``PVT`` reports from ``lng`` to ``lon`` to align with nRF Cloud.
      nRF Cloud still accepts ``lng`` for backward compatibility.


* :ref:`lib_nrf_cloud_coap` library:

  * Fixed a hard fault that occurred when encoding AGNSS request data and the ``net_info`` field of the :c:struct:`nrf_cloud_rest_agnss_request` structure is NULL.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

* Updated the internal Bluetooth serialization API and Bluetooth callback proxy API to become part of the public NRF RPC API.

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

|no_changes_yet_note|

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

nRF Cloud integation
--------------------

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

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``ea02b93eea35afef32ebb31f49f8e79932e7deee``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline ea02b93eea ^23cf38934c

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^ea02b93eea

The current |NCS| main branch is based on revision ``ea02b93eea`` of Zephyr.

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

* Added the :ref:`peripheral_sensor_node_shield` page.
