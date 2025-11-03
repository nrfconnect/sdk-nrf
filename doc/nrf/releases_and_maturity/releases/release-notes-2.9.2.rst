.. _ncs_release_notes_292:

|NCS| v2.9.2 Release Notes
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

This patch release adds changes on top of the :ref:`nRF Connect SDK v2.9.0 <ncs_release_notes_290>` and :ref:`nRF Connect SDK v2.9.1 <ncs_release_notes_291>`.
The changes affect Matter, Zigbee, Bluetooth® Mesh, and nrfx.

* |NCS| v2.9.2 has support for the nRF54L15, nRF54L10, and nRF54L05 devices.
* The ``nrfutil-device`` and SEGGER J-Link versions are updated to v2.8.8 and |jlink_ver|, respectively, for this release.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.9.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.9.2`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.9.2.
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
     - `Changelog <Modem library changelog for v2.9.2_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.9.2_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.9.2`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_292_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Build and configuration system
==============================

* Use the ``nrfutil-device`` v2.8.8 and SEGGER J-Link |jlink_ver| for the |NCS| v2.9.2 release.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Matter
------

* Added:

  * Automatic Wi-Fi® reconnection after RPU recovery.
  * Fixes for CVE-2024-56318 and CVE-2024-56319.

* Fixed:

  * Problems with certification tests TC-DGGEN-2.1, TC-IDM-10.5, and TC-ACE-2.2.
  * Door lock issues related to credentials persistency and factory reset.

Matter fork
+++++++++++

* Added a new ``kFactoryReset`` event that is posted during factory reset.
  The application can register a handler and perform additional cleanup.

Zigbee
------

* Fixed:

  * Compilation errors that previously occurred when the :kconfig:option:`CONFIG_ZIGBEE_FACTORY_RESET` Kconfig option was disabled.
  * The :file:`zb_add_ota_header.py` script to allow a patch version higher than 9 in an ``APPLICATION_VERSION_STRING``.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Thingy:53: Zigbee weather station
---------------------------------

* Added:

  * A fix for logging negative temperature values.
  * Logging unification.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Cellular samples
----------------

* Decreased the fragment size on the following cellular samples that include the :ref:`liblwm2m_carrier_readme` library:

  * Serial LTE modem
  * :ref:`http_application_update_sample`
  * :ref:`lwm2m_carrier`
  * :ref:`modem_shell_application`

  This is done to ensure that FOTA issued from Verizon LwM2M servers has enough room for HTTP headers.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_mesh` library:

  * Fixed an issue where the ``bt_mesh_adv_unref()`` function could assert when messaging to a proxy node.

Libraries for networking
------------------------

* :ref:`lib_nrf_provisioning` library:

  * Fixed an issue where the results from the :c:func:`zsock_getaddrinfo` function were not freed when the CoAP protocol was used for connection establishment.

nrfx
====

* Updated MDK to v8.68.2 to improve errata handling for nRF54L05 and nRF54L10 devices.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``beb733919d8d64a778a11bd5e7d5cbe5ae27b8ee``.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.9.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.7.99-ncs2
