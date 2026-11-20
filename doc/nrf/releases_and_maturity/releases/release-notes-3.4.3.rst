.. _ncs_release_notes_343:

|NCS| v3.4.3 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.4.2 <ncs_release_notes_342>`:

* The |NCS| v3.4.3 is based on Zephyr 4.4.2, Mbed TLS 4.1.1, and TF-M 2.3.1.

  This patch release is part of the v3.4 release branch, which has long-term support (LTS) for five years from the v3.4.0 release.
  During this period, patch releases will provide updates for security vulnerabilities and critical bug fixes.
  These patch releases will not contain breaking changes unless security fixes require them.

* The v3.4 release branch is the last |NCS| release branch that includes support for nRF52 Series devices.
  The nRF52 Series devices are considered feature complete, and support for them will be removed from samples and applications in the main branch.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.4.3**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`command-line installation <command_line_installation>` and :ref:`updating the repositories <updating_repos>` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.4.3`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.4.3.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9151 SoC`_
* `Modem firmware compatibility matrix for the nRF9160 SoC`_

Use the latest version of the `Programmer app`_ of `nRF Connect for Desktop`_ to update the modem firmware.
See `Programming nRF91 Series DK firmware`_ for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v3.4.3_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.4.3_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.4.3`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_343_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Security
========

Trusted Firmware-M (TF-M)
-------------------------

* Removed the "TF-M: Platform security architecture test" documentation page from the sample documentation section.
  The test is still available as a standalone test in the :file:`tests/tfm/tfm_psa_test` directory.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

DFU libraries
-------------

* :ref:`lib_dfu_target` library:

  * Updated the library to clear the last page of the flash area when scheduling an update, in case the page contains leftover data from a previous operation.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom` script:

  * Added:

    * The ``--package-download-format`` option to control the SPDX PackageDownloadLocation format.
    * The ``--input-dir`` option to the :ref:`west ncs-sbom <west_sbom>` command.
      It recursively adds all files in the given directory to the report, equivalent to ``--input-files DIR/**/*``.
    * The ``--spdx-version`` option to generate SPDX 3.0 (JSON-LD) reports.
      The default remains SPDX 2.3.

  * Updated:

    * The SPDX output format from ``SPDX-2.2`` to ``SPDX-2.3``.
    * The ScanCode Toolkit detector to support macOS on ARM64 and Linux aarch64.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Memfault integration
--------------------

* Updated Memfault to version 1.47.0.
  See the `Memfault firmware SDK changelog`_ for details.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``dccb09599635bdff17633fa7e9dab014b91dce90`` (Zephyr v4.4.2), with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since v3.4.0, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline dccb095996 ^684c9e8f32

For the list of |NCS| specific commits added since v3.4.0, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline --cherry-pick --right-only ncs-v3.4.0...manifest-rev ^dccb095996

Additions specific to |NCS|
---------------------------

* Fixed an issue where the GRTC settings for nrfx were not applied according to Kconfig.
  When one of the :kconfig:option:`CONFIG_NRF_GRTC_TIMER_AUTO_KEEP_ALIVE`, :kconfig:option:`CONFIG_NRF_GRTC_START_SYSCOUNTER`, :kconfig:option:`CONFIG_NRF_GRTC_TIMER_STOP_AT_UNINIT`, or :kconfig:option:`CONFIG_NRF_GRTC_TIMER_CLEAR_AT_UNINIT` Kconfig options was disabled, the default nrfx configuration was used instead.
  As a result, the corresponding behavior could still be enabled.
  For example, the GRTC counter could be cleared when the GRTC driver was uninitialized.
