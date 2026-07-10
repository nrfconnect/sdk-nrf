.. _ncs_release_notes_341:

|NCS| v3.4.1 Release Notes
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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.4.0 <ncs_release_notes_340>`:

* Libraries:

  * Added support for the nRF54LC10A SoC in the :ref:`lib_ram_pwrdn` library.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.4.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.4.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.4.1.
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
     - `Changelog <Modem library changelog for v3.4.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.4.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.4.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_341_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Security
========


Mbed TLS
--------

* Updated Mbed TLS to v4.1.1 (from v4.1.0) and TF-PSA-Crypto to v1.1.1 (from v1.1.0).
  For more information, see the upstream `Mbed TLS 4.1.1 release notes`_ and `TF-PSA-Crypto 1.1.1 release notes`_.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated the TX power calibration for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
    The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-13`` dBm to ``-11`` dBm to meet the Fast Pair distance certification requirements.

Enhanced ShockBurst samples
---------------------------

* Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` and ``nrf54ls05dk/nrf54ls05a/cpuapp`` board targets in all samples.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Other libraries
---------------

* :ref:`lib_ram_pwrdn` library:

  * Added support for the nRF54LC10A SoC.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed an issue where UICR was not provisioned with monotonic counter structures, when :kconfig:option`SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION` was enabled, MCUboot was the only bootloader, and Partition Manager was disabled.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``684c9e8f32e4373a21098559f748f06915f950c9``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.4.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.4.1

Documentation
=============

* Added the :ref:`kconfig:kconfig_diff` page, displaying differences between available Kconfig options across releases.
  To generate the new documentation page, set the ``KCONFIGDIFF`` CMake option to ``ON``.
