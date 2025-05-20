.. _ncs_release_notes_241:

|NCS| v2.4.1 Release Notes
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

This minor release adds changes on top of the :ref:`nRF Connect SDK v2.4.0 <ncs_release_notes_240>`.
The changes affect Matter, Thread, 802.15.4 radio driver, and Apple Find My.

See :ref:`ncs_release_notes_241_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.4.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.4.1`_.

IDE and tool support
********************

`nRF Connect for Visual Studio Code extension <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.4.1.
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
     - `Changelog <Modem library changelog for v2.4.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.4.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.4.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_241_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Matter
------

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added the :kconfig:option:`CHIP_MALLOC_SYS_HEAP_WATERMARKS_SUPPORT` Kconfig option to manage watermark support.
* Updated the factory data guide with an additional rotating ID information.
* Fixed an IPC crash on nRF5340 when Zephyr's main thread takes a long time.
* Fixed RAM and ROM reports.

Thread
------

* Updated the Thread 1.3 compliant libraries.
  The update contains a fix for network forming with legacy devices and key ID mode.
  This is a fix for the `Unauthorized Thread Network Key Update (SA-2023-234) vulnerability`_.

nRF IEEE 802.15.4 radio driver
------------------------------

* Added a workaround for known issue KRKNWK-16976 allowing to detect crashes of the nRF5340 network core running the 802.15.4 stack.

Apple Find My
-------------

* Fixed two issues in the Apple Find My add-on for the |NCS|.
  See the detailed description in the Find My release notes.

Samples
=======

Matter samples
--------------

* :ref:`matter_lock_sample` sample:

  * Fixed the feature map for software diagnostic cluster.
    Previously, it was set incorrectly.
  * Fixed the cluster revision for basic information cluster.
    Previously, it was set incorrectly.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``4bbd91a9083a588002d4397577863e0c54ba7038``.
It also contains some |NCS| specific additions and commits cherry-picked from the upstream Zephyr repository including the following ones:

* Adjusted WEST_PYTHON to posix path to be consistent with other Python scripts that pass paths to the Zephyr CMake build system.
* Added a workaround against nRF5340 network core crashes which occurred after waking up from sleep state on WFI/WFE instructions.
* Fixed building with picolib enabled on Windows.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.4.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.3.99-ncs1
