.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.8.99
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

Highlights
**********

* |no_changes_yet_note|

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.8.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

Using Edge Impulse
------------------

* Added instruction on how to download a model from a public Edge Impulse project.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth
---------

* |no_changes_yet_note|

Matter
------

* Added ``EXPERIMENTAL`` select in Kconfig that informs that Matter support is experimental.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* |no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* |no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* |no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* |no_changes_yet_note|

Modem libraries
---------------

* :ref:`at_cmd_parser_readme` library:
  * Can now parse AT command responses containing the response result, for example, ``OK`` or ``ERROR``.

Libraries for networking
========================

* :ref:`lib_fota_download` library:

  * Fixed an issue where the application would not be notified of errors originating from inside :c:func:`download_with_offset`. In the http_update samples, this would result in the dfu start button interrupt being disabled after a connect error in :c:func:`download_with_offset` after a disconnect during firmware download.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Modem library
+++++++++++++

* |no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Partition Manager
-----------------

* |no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* |no_changes_yet_note|

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

* |no_changes_yet_note|

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* |no_changes_yet_note|

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
