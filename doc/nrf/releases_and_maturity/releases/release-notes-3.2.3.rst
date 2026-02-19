.. _ncs_release_notes_323:

|NCS| v3.2.3 Release Notes
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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.2.2 <ncs_release_notes_322>` and :ref:`nRF Connect SDK v3.2.1 <ncs_release_notes_321>` and :ref:`nRF Connect SDK v3.2.0 <ncs_release_notes_320>`:

.. TODO

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.2.3**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.2.3`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.2.3.
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
     - `Changelog <Modem library changelog for v3.2.3_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.2.3_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.2.3`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_323_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Fixed an assertion caused by a stack overflow in the main thread when running on the nRF54H20 DK in debug configurations.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``911b3da1394dc6846c706868b1d407495701926f``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.2.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.2.0
