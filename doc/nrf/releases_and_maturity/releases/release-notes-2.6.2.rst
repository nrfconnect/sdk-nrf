.. _ncs_release_notes_262:

|NCS| v2.6.2 Release Notes
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

This patch release adds changes on top of the :ref:`nRF Connect SDK v2.6.0 <ncs_release_notes_260>` and :ref:`nRF Connect SDK v2.6.1 <ncs_release_notes_261>`.
The changes affect Wi-Fi® stack.

See :ref:`ncs_release_notes_262_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.6.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.6.2`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.6.2.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See the `Programming nRF91 Series DK firmware`_ page for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v2.6.2_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.6.2_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.6.2`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_262_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Wi-Fi
-----

* Fixed a bug that causes TX EVM degradation.

Thread
------

* Fixed issue with high power consumption after parent loss.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* Added:

  * A new recovery feature through a watchdog timer with Kconfig-based timeout configuration to recover the nRF70 Series chip in case of any lockups.
  * Wi-Fi interface de-initialization path clean-up and stabilization.
  * A debug statistics and logging mechanism.

* Fixed interoperability issues with some access points through the following enhancements:

  * Responding to any TIM fields instead of DTIM-only beacons.
  * Dynamic TSF adjustments where the TSF is not properly aligned to the TBTT.
  * QoS NULL-based packet retrieval through Kconfig, as well as a network management API.
  * Support for keep alive frame.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Libraries for networking
------------------------

* Added the :ref:`lib_wifi_ready` library to facilitate application development.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``23cf38934c0f68861e403b22bc3dd0ce6efbfa39``.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.6.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.5.99-ncs1
