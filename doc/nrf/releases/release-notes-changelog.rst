:orphan:

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

|no_changes_yet_note|

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

|no_changes_yet_note|

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``25e241ebcbf11b1f63dbe25546b1f10219866ad0``.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

|no_changes_yet_note|

nRF9160: Serial LTE modem
-------------------------

  * Updated:

    * Updated the AT response and the URC sent when the application enters and exits data mode.

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

nRF Desktop
-----------

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

|no_changes_yet_note|


Bluetooth mesh samples
----------------------

|no_changes_yet_note|

nRF9160 samples
---------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Matter samples
--------------

|no_changes_yet_note|

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

|no_changes_yet_note|

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

|no_changes_yet_note|

Libraries for networking
------------------------

|no_changes_yet_note|

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

Common Application Framework (CAF)
----------------------------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

|no_changes_yet_note|

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

The `zcbor`_ module has been updated from version 0.3.0 to 0.4.0.
Release notes for 0.4.0 can be found in :file:`ncs/nrf/modules/lib/zcbor/RELEASE_NOTES.md`.

The following major changes have been implemented:

* |no_changes_yet_note|

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

* |no_changes_yet_note|

* Updated:

* |no_changes_yet_note|

* Removed:

* |no_changes_yet_note|

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
