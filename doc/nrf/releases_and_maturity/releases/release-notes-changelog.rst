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

|no_changes_yet_note|

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

* Updated:

  * Workaround handling for the hardware erratas.
  * The implementation of the hardware errata HMPAN-103 for the nRF54H20 SoC.

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

|no_changes_yet_note|

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

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

|no_changes_yet_note|
