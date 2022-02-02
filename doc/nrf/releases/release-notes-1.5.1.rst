.. _ncs_release_notes_151:

|NCS| v1.5.1 Release Notes
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

* Added Radio front-end module (FEM) support for Bluetooth LE in single protocol configuration and multiprotocol configuration with Thread
* Integrated Apple HomeKit ADK v5.2 (access by request to MFI licensees)
* Added production support for Apple HomeKit for nRF52840
* Added production support for Zigbee for nRF52833 and nRF52840
* Updated ZBOSS Zigbee stack to version 3_3_0_7+03_22_2021

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.5.1**.
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

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

SoftDevice Controller
---------------------

See the :ref:`softdevice_controller_changelog` for detailed information.

* Updated:

  * The :ref:`softdevice_controller` can now be qualified on nRF52832.


Zigbee
------

* Updated:

  * ZBOSS Zigbee stack to version 3_3_0_7+03_22_2021. See :ref:`zboss_configuration` for detailed information.

Project Connected Home over IP (Project CHIP)
---------------------------------------------

* Updated:

  * Changed the remote URL of the pigweed submodule.

Apple HomeKit Ecosystem support
-------------------------------

* Integrated HomeKit Accessory Development Kit (ADK) v5.2.

Known issues
************

See `known issues for nRF Connect SDK v1.5.1`_ for the list of issues valid for this release.
