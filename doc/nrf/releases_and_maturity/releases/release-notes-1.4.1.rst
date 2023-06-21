.. _ncs_release_notes_141:

|NCS| v1.4.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices (which are pre-production) and Zigbee and Bluetooth mesh protocols are supported for development in v1.4.1 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.


Highlights
**********

* Qualified Zephyr Host to be conformant with Bluetooth Core Specification v5.2
* Updated Carrier Library to v0.10.1

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.4.1**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

This version of the |NCS| has been tested with the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.2.2

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v0.10.1.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.
  * Updated the :ref:`lwm2m_certification` page by replacing the version dependency table with a link to Mobile network operator certifications.

Multiprotocol Service Layer (MPSL)
==================================

* Fixed the DRGN-15064 known issue where the External Full swing and External Low swing configuration sources of Low Frequency Clock were not working correctly.

Zephyr
======

sdk-zephyr
----------

.. note::
    Except for changes in Zephyr that were cherry-picked into |NCS| and described in the following section, there were no upstream commits incorporated into |NCS| in this release compared with the v1.4.0 release.

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The `sdk-zephyr`_ fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``7a3b253ced``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 7a3b253ced ^v2.3.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^7a3b253ced

The current |NCS| release is based on Zephyr v2.4.0.
See the :ref:`Zephyr v2.4.0 release notes <zephyr:zephyr_2.4>` for the list of changes.

Zephyr changes incorporated into |NCS|
--------------------------------------

This section contains changes in Zephyr that were cherry-picked into |NCS| for this release.

* Added support for the :ref:`zephyr:actinius_icarus` board.

Bluetooth Host
~~~~~~~~~~~~~~

* Qualified Zephyr Host to be conformant with Bluetooth Core Specification v5.2.
  These changes were cherry-picked in the |NCS| v1.4.1 Bluetooth Host qualification update, which contains the following changes:

  * Added qualification for Bluetooth Core Specification v5.2.
    Qualified new features are the following:

    * Advertising extension
    * Legacy OOB pairing

* Fixed an issue where a pairing fail could lead to a GATT procedure failure.
  This fixes the NCSDK-6844 known issue in |NCS|.

Known issues
************

See `known issues for nRF Connect SDK v1.4.1`_ for the list of issues valid for this release.
