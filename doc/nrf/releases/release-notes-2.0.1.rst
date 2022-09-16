.. _ncs_release_notes_201:

|NCS| v2.0.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
The feature is made available in its current state though the design and interfaces can change between release tags.
The feature will also be labeled with "EXPERIMENTAL" in Kconfig files to indicate this status.
Build warnings will be generated to indicate when features labeled EXPERIMENTAL are included in builds unless the Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is disabled.

Highlights
**********

This minor release adds the following fixes on top of the :ref:`nRF Connect SDK v2.0.0 <ncs_release_notes_200>`:

* Fixed an issue in :ref:`nrfxlib:ot_libs` in nrfxlib that was preventing Thread 1.2 certification of the Minimal Thread Device (MTD) role.

See :ref:`ncs_release_notes_201_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.0.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.0.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.0.1.
SEGGER Embedded Studio Nordic Edition is no longer tested or recommended for new projects.

:ref:`Toolchain Manager <gs_app_tcm>`, used to install the |NCS| automatically from `nRF Connect for Desktop`_, is available for Windows, Linux, and macOS.

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
     - `Changelog <Modem library changelog for v2.0.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.0.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.0.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_201_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Thread
------

* Fixed a bug in which a Minimal Thread Device was not able to handle Address Error Notification messages.
* Updated the values in the memory requirement tables in :ref:`thread_ot_memory` after the update to the :ref:`nrfxlib:ot_libs` in nrfxlib.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``53fbf40227de087423620822feedde6c98f3d631``, plus some |NCS| specific additions.
This is the same commit ID as the one used for |NCS| :ref:`v2.0.0 <ncs_release_notes_200>`.

For a complete list of |NCS| specific commits since v2.0.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.0.99-ncs1
