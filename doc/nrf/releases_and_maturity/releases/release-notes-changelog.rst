.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.0.99
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
See `known issues for nRF Connect SDK v3.0.0`_ for the list of issues valid for the latest release.

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

Developing with coprocessors
============================

* Added the :ref:`ug_hpf_softperipherals_comparison` documentation page, describing potential use cases and differences between the two solutions.

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

* Removed:

  * The nRF Connect Matter Manufacturer Cluster Editor tool page.
    The tool is now available in the `nRF Connect for Desktop`_ app as the Matter Cluster Editor app.
    For installation instructions and more information about the tool, see the `Matter Cluster Editor app`_ documentation.

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

|no_changes_yet_note|

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

* Added:

  * Experimental support for Audio on the nRF5340 DK, with LED state indications and button controls.
  * Experimental Support for stereo in :ref:`broadcast sink app<nrf53_audio_broadcast_sink_app>`.
    The broadcast sink can now receive audio from two BISes and play it on the left and right channels of the audio output, if the correct configuration options are enabled.
    The I2S output will be stereo, but :zephyr:board:`nrf5340_audio_dk` will still only have one audio output channel, since it has a mono codec (CS47L63).
    See :file:`overlay-broadcast_sink.conf` for more information.
  * The audio devices are now set up with a location bitfield according to the BT Audio specification, instead of a channel.
    Since a device can have multiple locations set, the location name has been removed from the device name during DFU.
  * Experimental Support for stereo in :ref:`unicast server app<nrf53_audio_unicast_server_app>`.
    The unicast server can now receive audio from two CISes and play it on the left and right channels of the audio output, if the correct configuration options are enabled.
    The I2S output will be stereo, but :zephyr:board:`nrf5340_audio_dk` will still only have one audio output channel, since it has a mono codec (CS47L63).
    See :file:`overlay-unicast_server.conf` for more information.

* Updated:

  * The application to use the ``NFC.TAGHEADER0`` value from FICR as the broadcast ID instead of using a random ID.
  * The application to change from Newlib to Picolib to align with |NCS| and Zephyr.
  * The application to use the :ref:`net_buf_interface` API to pass audio data between threads.
    The :ref:`net_buf_interface` will also contain the metadata about the audio stream in the ``user_data`` section of the API.
    This change was done to transition to standard Zephyr APIs, as well as to have a structured way to pass N-channel audio between modules.
  * The optional buildprog tool to use `nRF Util`_ instead of nrfjprog that has been deprecated.
  * The documentation pages with information about the :ref:`SD card playback module <nrf53_audio_app_overview_architecture_sd_card_playback>` and :ref:`how to enable it <nrf53_audio_app_configuration_sd_card_playback>`.
  * The buffer count (:kconfig:option:`CONFIG_BT_ISO_TX_BUF_COUNT` and :kconfig:option:`CONFIG_BT_BUF_ACL_TX_COUNT`) to be in-line with SoftDevice Controller (SDC) defaults.
    This can be changed and optimized for specific use cases.

* Removed:

  * The uart_terminal tool to use standardized tools.
    Similar functionality is provided through the `nRF Terminal <nRF Terminal documentation_>`_ in the |nRFVSC|.
  * The functionality to jump between BIS0 and BIS1 in the :ref:`broadcast sink <nrf53_audio_broadcast_sink_app>` application.
    Button 4 is no longer needed for this purpose due to added support for stereo audio.

nRF Desktop
-----------

|no_changes_yet_note|

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

|no_changes_yet_note|

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Bluetooth Fast Pair samples
---------------------------

|no_changes_yet_note|

Cellular samples
----------------

|no_changes_yet_note|

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

|no_changes_yet_note|

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

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``9a6f116a6aa9b70b517a420247cd8d33bbbbaaa3``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 9a6f116a6a ^fdeb735017

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^9a6f116a6a

The current |NCS| main branch is based on revision ``9a6f116a6a`` of Zephyr.

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
