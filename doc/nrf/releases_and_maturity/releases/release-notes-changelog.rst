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

|no_changes_yet_note|

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

|no_changes_yet_note|

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

|no_changes_yet_note|

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

Machine learning
----------------

|no_changes_yet_note|

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

|no_changes_yet_note|

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_settings_loader` to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology.
    As a result, all :ref:`zephyr:nrf54h20dk_nrf54h20` configurations were migrated from the NVS settings backend to the ZMS settings backend.
  * The :ref:`zephyr:nrf54h20dk_nrf54h20` release configuration to enable the :ref:`nrf_desktop_watchdog`.
  * The configuration files of the :ref:`nrf_desktop_click_detector` (:file:`click_detector_def.h`) to allow using them also when Bluetooth LE peer control using a dedicated button (:ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>`) is disabled.

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

Bluetooth Fast Pair samples
---------------------------

|no_changes_yet_note|

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Cellular samples
----------------

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

* :ref:`matter_template_sample` sample:

  * Updated the internal configuration for the :ref:`zephyr:nrf54l15dk_nrf54l15` target to use the DFU image compression and provide more memory space for the application.

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

  * Updated:

    * Configuration for the :ref:`zephyr:nrf54h20dk_nrf54h20` board to support multi-domain logging using the ARM Coresight STM.
    * The logging format in the standard logging mode to align it with the format used in the multi-domain logging mode.

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

|no_changes_yet_note|
