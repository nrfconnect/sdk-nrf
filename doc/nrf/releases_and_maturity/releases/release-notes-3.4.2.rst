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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.4.1 <ncs_release_notes_341>`:

* The |NCS| v3.4.2 is based on Zephyr 4.4.2, Mbed TLS 4.1.1, and TF-M 2.3.1.

  This patch release is part of the v3.4 release branch, which has long-term support (LTS) for five years from the v3.4.0 release.
  During this period, patch releases will provide updates for security vulnerabilities and critical bug fixes.
  These patch releases will not contain breaking changes unless security fixes require them.

* The v3.4 release branch is the last |NCS| release branch that includes support for nRF52 Series devices.
  The nRF52 Series devices are considered feature complete, and support for them will be removed from samples and applications in the main branch.

Added the following supported features:

* nRF54L Series:

  * Support for the nRF54LS05A and nRF54LS05B SoCs.
  * Support for the nRF54LV10A SoC.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.4.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`command-line installation <command_line_installation>` and :ref:`updating the repositories <updating_repos>` for more information.

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

Bootloaders and DFU
===================

* Fixed:

  * An issue with signing MCUboot direct-XIP variant images, where sysbuild used the Zephyr signing path instead of the |NCS| signing script for the secondary slot image.
    Signed variant images could miss |NCS|-specific image metadata, such as the security counter or ROM-fixed flag, which caused the secondary slot image to be rejected unexpectedly.
  * A potential out-of-bounds access in application's SMP.
    The image and slot indexes are now validated before use, and invalid inputs return safe fallback values.

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

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed:

  * An issue where MCUboot did not clear a serial recovery boot request after entering serial recovery when the :kconfig:option:`CONFIG_NRF_BOOT_SERIAL_BOOT_REQ` Kconfig option is enabled, causing the device to re-enter serial recovery on every reset instead of booting the application.
  * A bus fault in the custom type-length-value (TLV) validation hook when MCUboot runs in single-slot mode on the nRF54H20 SoC.
  * An issue where MCUboot did not validate image header size, image size, and protected TLV size against the boundaries of the flash partition that holds the installed image.
  * An issue where MCUboot serial recovery with progressive erase (:kconfig:option:`CONFIG_BOOT_ERASE_PROGRESSIVELY`) could erase the last image sector of a large upload when the image extended into the slot trailer area in external flash (for example, on the nRF54H20 SoC).
  * An issue where multi-image operation denied key revocation because the state check was done beyond the state table.
    This affected MCUboot instances that are used as a second-stage bootloader above nRF Secure Immutable Bootloader (NSIB) in the S0/S1 update scheme on nRF54L SoCs.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``dccb09599635bdff17633fa7e9dab014b91dce90`` (Zephyr v4.4.2), with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since v3.4.0, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline dccb095996 ^684c9e8f32

For the list of |NCS| specific commits added since v3.4.0, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline --cherry-pick --right-only ncs-v3.4.0...manifest-rev ^dccb095996

Documentation
=============

* Added the :guilabel:`Open in VS Code` button for each sample page.
  It lets you quickly open the sample and install the required version of the |NCS| toolchain.

* Updated the :ref:`install_ncs` page by adding a button for installing the |nRFVSC| with the latest stable release of the |NCS|.
  This has also simplified the structure of the page.
