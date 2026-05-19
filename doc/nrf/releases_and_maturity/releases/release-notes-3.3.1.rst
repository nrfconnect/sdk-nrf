.. _ncs_release_notes_331:

|NCS| v3.3.1 Release Notes
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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.3.0 <ncs_release_notes_330>`.
The changes affect the nRF54L Series, Bluetooth®, and the Peripheral samples.

Added the following features as supported:

* Bluetooth:

  * Support for Bluetooth Core version 6.3.
  * Support for Channel Sounding Inline Phase Correction Term (CS IPT).
    This feature allows a CS Initiator to calculate distance without exchanging tone data over the ACL, resulting in faster and more power-efficient distance estimation.

* Fixed an issue where USB battery charging detection operated correctly only during the first cable connection after an SoC reset.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.3.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.3.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.3.1.
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
     - `Changelog <Modem library changelog for v3.3.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.3.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.3.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_331_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Developing with nRF54L Series
=============================

* Updated builds without Partition Manager to generate the :file:`bootconf.hex` file when |NSIB| is used as the bootloader.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added the :ref:`channel_sounding_ipt_initiator` and :ref:`channel_sounding_ipt_reflector` samples to demonstrate how to use the Bluetooth Channel Sounding (CS) Inline Phase Correction Term Transfer (IPT) feature.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added the ``set_channel_sequence_hopping_mode`` shell command to set the hopping mode for the channel sequence.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``93b5f19f994b9a9376985299c1427a1630f6950e``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.3.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.3.0
