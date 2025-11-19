.. _ncs_release_notes_291:

|NCS| v2.9.1 Release Notes
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

This patch release adds changes on top of the :ref:`nRF Connect SDK v2.9.0 <ncs_release_notes_290>`.

|NCS| v2.9.1 has workarounds for the following nRF54L15, nRF54L10, and nRF54L05 Errata:

* Errata 39 - Workaround is implemented in the :ref:`nrfxlib:mpsl` (MPSL) library and Zephyr.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.9.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.9.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.9.1.
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
     - `Changelog <Modem library changelog for v2.9.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.9.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.9.1`_ for the list of issues valid for the latest release.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``beb733919d8d64a778a11bd5e7d5cbe5ae27b8ee``.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.9.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.7.99-ncs2
