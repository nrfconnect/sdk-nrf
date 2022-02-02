.. _ncs_release_notes_161:

|NCS| v1.6.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the TF-M, MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices are now supported for production.
Wireless protocol stacks and libraries may indicate support for development or support for production for different series of devices based on verification and certification status.
See the release notes and documentation for those protocols and libraries for further information.

Highlights
**********

This minor release adds the following fixes on top of :ref:`nRF Connect SDK v1.6.0 <ncs_release_notes_160>`:

* Fixed an issue in SoftDevice Controller that affected scanning on the nRF53 Series (DRGN-15852).
* Fixed an issue in the HomeKit component, where DFU would fail when using external flash memory for update image storage (KRKNWK-10611).

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.6.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Known issues
************

See `known issues for nRF Connect SDK v1.6.1`_ for the list of issues valid for this release.

Documentation
=============

* :ref:`zigbee_memory` - corrected KB to B for stack memory requirements.
