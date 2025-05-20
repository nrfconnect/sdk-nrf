.. _ncs_release_notes_211:

|NCS| v2.1.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

This minor release adds the following changes on top of the :ref:`nRF Connect SDK v2.1.0 <ncs_release_notes_210>`:

* Updated the Matter solution by integrating a newer `Matter SDK tagged as v1.0.0`_.
  Support for Matter over Thread is no longer experimental.

See :ref:`ncs_release_notes_211_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.1.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.1.1`_.

IDE and tool support
********************

`nRF Connect for Visual Studio Code extension <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.1.1.
SEGGER Embedded Studio Nordic Edition is no longer tested or recommended for new projects.

:ref:`gs_app_tcm`, used to install the |NCS| automatically from `nRF Connect for Desktop`_, is available for Windows, Linux, and macOS.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v2.1.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.1.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.1.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_211_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Matter
------

* Added:

  * Feature-complete :ref:`production support <software_maturity>` for Matter over Thread.
  * Documentation about :ref:`ug_matter_device_types`.

* Updated documentation about Data Model and Interaction Model by moving it to separate pages: :ref:`ug_matter_overview_data_model` and :ref:`ug_matter_overview_int_model`, respectively.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``561d23d0db215a99705ff0696e73853c8edf11b2``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Updated Matter repository to the official Matter 1.0 version tag.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Thingy:53 Matter weather station
--------------------------------

* Added new factory data, PICS, and configuration overlay files.
  These are useful for getting to know the Matter certification process.
* Updated the documentation page with a new section about the certification files, available only in `Matter weather station application from the v2.1.1`_.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Matter samples
--------------

* Updated ZAP configuration of the samples to conform with device types defined in Matter 1.0 specification.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``71ef669ea4a73495b255f27024bcd5d542bf038c``.
This is the same commit ID as the one used for |NCS| :ref:`v2.1.0 <ncs_release_notes_210>`.
