:orphan:

.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.8.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Highlights
**********

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.8.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

There are no entries for this section yet.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth
---------

There are no entries for this section yet.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

There are no entries for this section yet.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

There are no entries for this section yet.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

There are no entries for this section yet.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

Updated:

There are no entries for this section yet.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Modem library
+++++++++++++

There are no entries for this section yet.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Partition Manager
-----------------

There are no entries for this section yet.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed support for Low Power in Zephyr's single-thread mode.
  See ``MCUBOOT_CPU_IDLE()`` macro.
* Switched USB CDC ACM serial recovery I/O device configuration from Kconfig to Devicetree with the compatible ``"zephyr,cdc-acm-uart"`` property.
* Switched UART serial recovery I/O device configuration from Kconfig to Devicetree using Zephyr's ``zephyr,console`` property of the chosen node.
* Fixed a deadlock issue with cryptolib selectors in Kconfig.
* Fixed an issue with the serial recovery skipping on nRF5340.
* Added cleanup of UARTE devices before chainloading the application.
  This allows the application to initialize the devices correctly and fixes potential missing output of the application's log.

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``3f82656``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 3f82656 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^3f82656

The current |NCS| main branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``bbd19d92f6d58ef79c98793fe0dfb2979db6336d``.

The following list summarizes the most important changes inherited from the upstream Matter:

* No changes yet

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* No changes yet
