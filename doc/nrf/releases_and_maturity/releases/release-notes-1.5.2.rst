.. _ncs_release_notes_152:

|NCS| v1.5.2 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the TF-M, MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices are now supported for production.
Wireless protocol stacks and libraries may indicate support for development or support for production for different series of devices based on verification and certification status. See the release notes and documentation for those protocols and libraries for further information.

Highlights
**********

* Fixed several nRF9160 issues.
  See `Changelog`_ for details.

Installation
************

The |NCS| v1.5.2 is not available for installation from the :ref:`Toolchain Manager <gs_app_tcm>`.
To upgrade from v1.5.1 to v1.5.2, complete the following steps:

1. If not done already, install nRF Connect SDK v1.5.1 by following the :ref:`Toolchain Manager <gs_app_tcm>` section.
#. In the command line, go to the :file:`nrf` directory in the :file:`ncs` installation directory.
   The default location is :file:`C:\\Users\\<username>\\ncs\\v1.5.1\\nrf`.
#. Fetch the v1.5.2 release tag with the ``git fetch origin v1.5.2`` command.
#. Checkout the release tag with the ``git checkout v1.5.2`` command.
#. Update the |NCS| installation with the ``west update`` command.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.5.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

This version of the |NCS| has been tested with the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.2.3
* mfw_nrf9160_1.1.4

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* :ref:`liblwm2m_carrier_readme` library:

  * Fixed an issue where the LwM2M carrier library would wait for the SIM to receive MSISDN from the network.
  * Removed a :c:macro:`CODE_UNREACHABLE` statement that would cause undefined behavior if the LwM2M carrier library encountered a non-recoverable error.

* Modem library:

  * Fixed a bug where stopping GNSS using ``NRF_SO_GNSS_STOP`` would let ongoing :c:func:`nrf_recv` calls on the GNSS socket block indefinitely.

Known issues
************

See `known issues for nRF Connect SDK v1.5.2`_ for the list of issues valid for this release.
