.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.2.99
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
See `known issues for nRF Connect SDK v3.2.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

|no_changes_yet_note|

Board support
=============

|no_changes_yet_note|

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

|no_changes_yet_note|

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

|no_changes_yet_note|

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

Security
========

* Added:

  * Support for the WPA3-SAE and WPA3-SAE-PT in the :ref:`CRACEN driver <crypto_drivers_cracen>`.
  * Support for the HMAC KDF algorithm in the CRACEN driver.
    The algorithm implementation is conformant to the NIST SP 800-108 Rev. 1 recommendation.

Protocols
=========

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

* Fixed invalid radio configuration for legacy ESB protocol.

Gazell
------

|no_changes_yet_note|

Matter
------

* Updated the :ref:`matter_test_event_triggers_default_test_event_triggers` section with the new Closure Control cluster test event triggers.

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

Wi-Fi®
------

|no_changes_yet_note|

Applications
============

Connectivity bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

|no_changes_yet_note|

nRF5340 Audio
-------------

* Added dynamic configuration of the number of channels for the encoder based on the configured audio locations.
  The number of channels is set during runtime using the :c:func:`audio_system_encoder_num_ch_set` function.
  This allows configuring mono or stereo encoding depending on the configured audio locations, potentially saving CPU and memory resources.
* Added high CPU load callback using the Zephyr CPU load subsystem.
  The callback uses a :c:func:`printk` function, as the logging subsystem is scheduled out if higher priority threads take all CPU time.
  This makes debugging high CPU load situations easier in the application.
  The threshold for high CPU load is set in :file:`peripherals.c` using :c:macro:`CPU_LOAD_HIGH_THRESHOLD_PERCENT`.
* Updated the buildprog/programming script.
  Devices are now halted before programming.
  Furthermore, the devices are kept halted until they are all programmed, and then started together
  with the headsets starting first.
  This eases sniffing of advertisement packets.

nRF Desktop
-----------

* Updated the :option:`CONFIG_DESKTOP_BT` Kconfig option to no longer select the deprecated :kconfig:option:`CONFIG_BT_SIGNING` Kconfig option.
  Application relies on Bluetooth LE security mode 1 and security level of at least 2 to ensure data confidentiality through encryption.

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

|no_changes_yet_note|

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Bluetooth Fast Pair samples
---------------------------

|no_changes_yet_note|

Cellular samples
----------------

* :ref:`nrf_cloud_mqtt_fota` and :ref:`nrf_cloud_mqtt_device_message` samples:

  * Added support for JWT authentication by enabling the :kconfig:option:`CONFIG_MODEM_JWT` Kconfig option.
    Enabling this option in the :file:`prj.conf` is necessary for using UUID as the device ID.

Cryptography samples
--------------------

* :ref:`crypto_aes_ccm` sample:

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.

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

|ISE| samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* :ref:`matter_manufacturer_specific_sample`:

  * Added support for the ``NRF_MATTER_CLUSTER_INIT`` macro.

* :ref:`matter_closure_sample`:

  * Added support for the Closure Control cluster test event triggers.

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

|no_changes_yet_note|

Other samples
-------------

|no_changes_yet_note|

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

|no_changes_yet_note|

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

  * Added support for new PDN events :c:enumerator:`LTE_LC_EVT_PDN_SUSPENDED` and :c:enumerator:`LTE_LC_EVT_PDN_RESUMED`.

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

* :ref:`lib_hw_id` library:

  * The ``CONFIG_HW_ID_LIBRARY_SOURCE_BLE_MAC`` Kconfig option has been renamed to :kconfig:option:`CONFIG_HW_ID_LIBRARY_SOURCE_BT_DEVICE_ADDRESS`.

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

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

* Added:

  * The option ``CONFIG_MEMFAULT_NCS_POST_INITIAL_HEARTBEAT_ON_NETWORK_CONNECTED`` to control whether an initial heartbeat is sent when the device connects to a network.
    Useful to be able to show device status and initial metrics in the Memfault dashboard as soon as possible after boot.

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

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``911b3da1394dc6846c706868b1d407495701926f``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 911b3da139 ^0fe59bf1e4

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^911b3da139

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

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

|no_changes_yet_note|
