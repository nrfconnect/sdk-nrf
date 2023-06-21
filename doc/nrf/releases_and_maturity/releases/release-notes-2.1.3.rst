.. _ncs_release_notes_213:

|NCS| v2.1.3 Release Notes
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

This minor release adds the following changes on top of the :ref:`nRF Connect SDK v2.1.0 <ncs_release_notes_210>`, :ref:`nRF Connect SDK v2.1.1 <ncs_release_notes_211>`, and :ref:`nRF Connect SDK v2.1.2 <ncs_release_notes_212>`:

* Updates the Matter ZAP Tool revision to be compatible with the :file:`zap` files that define the Data Model in |NCS| Matter samples.
* Bluetooth® fixes:

  * An issue in MPSL where the controller would assert when a Bluetooth role was running.
  * An issue in MPSL where the controller would abandon a link, causing a disconnect on the remote side.
  * An issue where RPA (Random Private Address) would not be changed when it should when the :kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option was enabled.

* UART driver fixes:

  * Fixed an issue where re-enabling UART after calling :c:func:`uart_rx_disable` too early could cause random hardware faults.
  * Fixed an issue where NULL pointer was de-referenced if ``PM_DEVICE_ACTION_RESUME`` action was requested.

See :ref:`ncs_release_notes_213_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.1.3**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.1.3`_.

IDE and tool support
********************

`nRF Connect for Visual Studio Code extension <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.1.3.
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
     - `Changelog <Modem library changelog for v2.1.3_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.1.3_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.1.3`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_213_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Bluetooth
---------

Fixed:

* An issue in MPSL where the controller would assert when a Bluetooth role was running.
  This was previously listed as the known issue with the issue ID (DRGN-17851).
* An issue in MPSL where the controller would abandon a link, causing a disconnect on the remote side.
  This was previously listed as the known issue with the issue ID (DRGN-18105).
* An issue where RPA (Random Private Address) would not be changed when it should when the :kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option was enabled.

Matter
------

* Updated the ZAP submodule revision to fix opening the ZAP files included in Matter samples in |NCS|.

Zephyr
======

* Fixed:

  * An issue where re-enabling UART after calling :c:func:`uart_rx_disable` too early could cause random hardware faults.
  * An issue where NULL pointer was de-referenced if ``PM_DEVICE_ACTION_RESUME`` action was requested.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``71ef669ea4a73495b255f27024bcd5d542bf038c``.
This is the same commit ID as the one used for |NCS| :ref:`v2.1.0 <ncs_release_notes_210>`, :ref:`v2.1.1 <ncs_release_notes_211>` and :ref:`v2.1.3 <ncs_release_notes_213>`.
It also includes some |NCS| specific additions and commits cherry-picked from the upstream Zephyr repository.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.1.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.1.99-ncs1
