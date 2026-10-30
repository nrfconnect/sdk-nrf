.. _ncs_release_notes_342:

|NCS| v3.4.2 Release Notes
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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.4.1 <ncs_release_notes_341>` and :ref:`nRF Connect SDK v3.4.0 <ncs_release_notes_340>`:
#TODO

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.4.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.4.2`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.4.2.
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
     - `Changelog <Modem library changelog for v3.4.2_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.4.2_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.4.2`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_342_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated the required `SEGGER J-Link`_ version to v9.82.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

* Fixed an issue on the Thingy:91 X where the CMSIS-DAP interface could not connect to the nRF91 Series SiP.
  The connection failed because the SiP was held in reset while the debug port was enabled.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated the TX power calibration for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.
    The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-13`` dBm to ``0`` dBm to meet the Fast Pair distance certification requirements.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated the library to v3.8.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Fixed the validation of the Data Length parameter in requests that the Find Hub Network (FHN) extension receives on the Beacon Actions characteristic.
    Requests that declare a Data Length above the maximum supported value are now rejected with the Invalid Value error.

Modem libraries
---------------

* :ref:`sms_readme` library:

  * Fixed an issue where the :c:func:`sms_register_listener` function returned a positive value instead of a negative error code when the modem responded with an AT error during SMS client registration.
    The function now returns ``-EFAULT`` in this case.

Libraries for networking
------------------------

* :ref:`lib_nrf_provisioning` library:

  * Updated the library to send the :c:enumerator:`NRF_PROVISIONING_EVENT_FATAL_ERROR` event when the modem fails to generate the attestation token for an unclaimed device.
    Previously, the :c:enumerator:`NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED` event was sent without a token.

Other libraries
---------------

* :ref:`lib_date_time` library:

  * Fixed an issue where periodic time updates stopped permanently if the time was obtained, for example, from an ``%XTIME`` notification shortly before a scheduled update.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Memfault integration
--------------------

* Updated Memfault to version 1.45.0.
  See the `Memfault firmware SDK changelog`_ for details.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``684c9e8f32e4373a21098559f748f06915f950c9``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.4.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.4.0
