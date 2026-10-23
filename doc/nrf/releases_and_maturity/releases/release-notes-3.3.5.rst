.. _ncs_release_notes_335:

|NCS| v3.3.5 Release Notes
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

This patch release adds changes on top of the following releases:

* :ref:`nRF Connect SDK v3.3.4 <ncs_release_notes_334>`
* :ref:`nRF Connect SDK v3.3.3 <ncs_release_notes_333>`
* :ref:`nRF Connect SDK v3.3.2 <ncs_release_notes_332>`
* :ref:`nRF Connect SDK v3.3.1 <ncs_release_notes_331>`
* :ref:`nRF Connect SDK v3.3.0 <ncs_release_notes_330>`

The changes affect Bluetooth®.

See :ref:`ncs_release_notes_335_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.3.5**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.3.5`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.3.5.
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
     - `Changelog <Modem library changelog for v3.3.5_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.3.5_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.3.5`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_335_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Bluetooth
---------

* Fixed:

  * An issue where the controller, acting as a CS Reflector, would shift the CS tone phase incorrectly when the Inline Phase Correction Term Transfer (IPT) feature was used with multiple antenna paths.
  * An issue where a peripheral connection could drop if a control procedure was received shortly after a Connection Subrate Update indication.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Fixed the validation of the Data Length parameter in requests that the Find Hub Network (FHN) extension receives on the Beacon Actions characteristic.
    Requests that declare a Data Length above the maximum supported value are now rejected with the Invalid Value error.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added missing bindings for the NETCORE DTS partition scheme used by the nRF5340 SoC.

* Fixed:

  * A compilation error when address verification is enabled on updatable MCUboot.

  * Gate RAM unlock on ``BASE_BOARD_IS_NON_SECURE`` when built as part of a sysbuild project with TF-M.

  * A possible infinite loop in the bootloader information sharing procedure when the Kconfig options :kconfig:option:`CONFIG_BOOT_SHARE_DATA_BOOTINFO` and :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION` are enabled.
    Iterating boot-info security counters could spin forever if the :c:func:`boot_nv_security_counter_get` function failed for an image index (for example, backends that expose only one NV counter when :kconfig:option:`CONFIG_BOOT_IMAGE_NUMBER` is greater than one).

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``93b5f19f994b9a9376985299c1427a1630f6950e``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.3.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.3.0

Documentation
=============

* Updated the :ref:`abi_compatibility` page to link to the `IronSide SE binaries changelog on the main branch`_ for the full list of changes to |ISE|.
