.. _ncs_release_notes_261:

|NCS| v2.6.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

This patch release adds changes on top of the :ref:`nRF Connect SDK v2.6.0 <ncs_release_notes_260>`.
The changes affect Matter, Wi-Fi®, Thread, and Zephyr.

See :ref:`ncs_release_notes_261_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.6.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.6.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.6.1.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v2.6.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.6.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.6.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_261_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Matter
------

* Updated:

  * The default Message Reliability Protocol (MRP) retry intervals for Thread devices to two seconds to reduce the number of spurious retransmissions in Thread networks.
  * Increased the number of available packet buffers in the Matter stack to avoid packet allocation issues.

Wi-Fi
-----

* Added:

  * Build-time disconnection timeout configuration.
  * BSSID as a parameter for the Wi-Fi ``connect`` command.
  * Auto-security mode with a fresh key management type to enhance connection time.
  * The following fixes and improvements:

    * Wi-Fi stuck in an awake state.
    * Frequent Wi-Fi disconnections in a congested environment and low RSSI.
    * HTTPS upload and download performance issues.
    * MQTT disconnection in a congested environment and low RSSI.
    * Selecting the best RSSI access point.
    * Rate adaptation enhancements to improve range in the 2.4 GHz band.
    * Radio test - Error in setting ``reg_domain``.

 To accommodate all fixes and improvements, there is a 1 kB increase in the stack usage of the WPA supplicant.
 Users must increase the stack sizes appropriately in their applications.

Thread
------

* Fixed ``otPlatCryptoPbkdf2GenerateKey`` API implementation to allow a fallback to legacy Mbed TLS implementation instead of returning ``OT_ERROR_NOT_CAPABLE``.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Matter Bridge
-------------

* Added:

  * The :ref:`CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES <CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES>` Kconfig option to set the maximum number of scanned Bluetooth LE devices.
  * The :ref:`CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS <CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS>` Kconfig option to set the scan timeout.

* Fixed an issue where the recovery mechanism for Bluetooth LE connections might not work correctly.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Wi-Fi samples
-------------

* :ref:`wifi_shell_sample` sample:

  * Modified ``connect`` command to provide better control over connection parameters.
  * Added ``Auto-Security-Personal`` mode to the ``connect`` command.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``23cf38934c0f68861e403b22bc3dd0ce6efbfa39``.
It also contains some |NCS| specific additions and commits cherry-picked from the upstream Zephyr repository, including the following one:

* Fixed the GPIO configuration that is used in the ``pinctrl`` driver for the QSPI IO3 line.
  Due to these incorrect settings, the QSPI NOR flash driver could not initialize successfully for a flash chip configured to work in non-Quad (2 I/O) mode.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.6.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.5.99-ncs1
