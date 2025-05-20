.. _ncs_release_notes_192:

|NCS| v1.9.2 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and re-distributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
The feature is made available in its current state though the design and interfaces can change between release tags.
The feature will also be labeled with "EXPERIMENTAL" in Kconfig files to indicate this status.
Build warnings will be generated to indicate when features labeled EXPERIMENTAL are included in builds unless the Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is disabled.

Highlights
**********

* Fixed a Multiprotocol Service Layer issue where the High Frequency Clock would stay active if it was turned on between timing events.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.9.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v1.9.2`_.

IDE support
***********

|NCS| v1.9.2 is supported by both `nRF Connect extensions for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ and SEGGER Embedded Studio Nordic Edition.
In future releases, documentation on the setup and usage of SEGGER Embedded Studio Nordic Edition will no longer be available and all references to IDE support will instruct users with nRF Connect extensions for Visual Studio Code.
In future releases, SEGGER Embedded Studio will no longer be tested with the build system and it is not recommended for new projects.

nRF Connect extensions for Visual Studio Code will be under continued active development and supported and tested with future |NCS| releases.
It is recommended for all new projects.

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
     - `Changelog <Modem library changelog for v1.9.2_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v1.9.2_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.9.2`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Multiprotocol Service Layer
===========================

The following change was introduced in the :ref:`nrfxlib:mpsl` in nrfxlib:

* Fixed an issue where the High Frequency Clock would stay active if it was turned on between timing events.
  This could occur during Low Frequency Clock calibration when using the RC oscillator as the Low Frequency Clock source (DRGN-17014).

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``45ef0d2``, plus some |NCS| specific additions.
This is the same commit ID as the one used for |NCS| :ref:`v1.9.0 <ncs_release_notes_190>`.

For a complete list of |NCS| specific commits since v1.9.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v2.7.99-ncs1
