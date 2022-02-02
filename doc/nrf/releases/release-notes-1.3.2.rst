.. _ncs_release_notes_132:

|NCS| v1.3.2 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices (which are pre-production) and Thread, Zigbee, and Bluetooth Mesh protocols are supported for development in v1.3.2 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.


Highlights
**********

* Zephyr Bluetooth LE Controller is now Bluetooth Core Specification v5.1 qualified.


Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.3.2**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.


Supported modem firmware
************************

This version of the |NCS| has been tested with the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.2.1

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

* Zephyr Bluetooth LE Controller (see :ref:`zephyr:bluetooth-arch`):

  * Fixed invalid disconnect reason in the disconnect complete event.
  * Changed the controller HCI version to Bluetooth Core Specification v5.1.
