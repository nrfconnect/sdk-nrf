.. _ncs_release_notes_251:

|NCS| v2.5.1 Release Notes
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

This patch release adds changes on top of the :ref:`nRF Connect SDK v2.5.0 <ncs_release_notes_250>`.
The changes affect Matter, Wi-Fi®, nrfx, Zephyr, and Trusted Firmware-M (TF-M).

See :ref:`ncs_release_notes_251_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.5.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.5.1`_.

IDE and tool support
********************

`nRF Connect for Visual Studio Code extension <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.5.1.

:ref:`gs_app_tcm`, used to install the |NCS| automatically from `nRF Connect for Desktop`_, is available for Windows, Linux, and macOS.

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
     - `Changelog <Modem library changelog for v2.5.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.5.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.5.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_251_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Matter
------

* Updated the default values of attributes in ZAP files to comply with the Matter 1.1 specification.

* Fixed:

  * An issue with commissioning Matter over Wi-Fi devices to the fifth fabric.
  * An issue with the Matter Bridge not being operational in commercial ecosystems, `Apple Home <Apple Home integration with Matter_>`_ and `Samsung SmartThings <Samsung SmartThings integration with Matter_>`_.

Wi-Fi
-----

* Added improvements and fixes related to the following:

  * :term:`Delivery Traffic Indication Message (DTIM)` power save functionality
  * Firmware issues due to AP power cycle
  * MAC address configuration
  * General system stability

nrfx
====

* Updated the nrfx QSPI driver behavior.
  Now, QSPI peripheral is not activated during the driver initialization but rather when the first operation is requested or when the :c:func:`nrfx_qspi_activate` function is called.
* Fixed an issue when external memory requires a relatively long time for the operation to happen.
  This is done by adding a timeout member to :c:struct:`nrfx_qspi_config_t`, which can be configured at runtime.
  If the timeout value is ``0``, the old value is used instead (500 milliseconds).
* Removed the ongoing custom instruction long transfer check from the :c:func:`nrfx_qspi_deactivate` function.
  The check could trigger errors, and now you must make sure that the custom instruction long transfer has ended, otherwise it will be interrupted.
  If a custom instruction long transfer is ongoing when the :c:func:`nrfx_qspi_uninit` and :c:func:`nrfx_qspi_deactivate` functions are called, the transfer will be interrupted.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``a768a05e6205e415564226543cee67559d15b736``.
It also contains some |NCS| specific additions and commits cherry-picked from the upstream Zephyr repository including the following one:

* Added workaround in ``sdk-zephyr`` for the Anomaly 168 that is affecting the nRF5340 SoC.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.5.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.4.99-ncs1

Trusted Firmware-M
==================

* Fixed:

  * An issue where TF-M does not compile without the ``gpio0`` node enabled in the devicetree.
  * An issue where TF-M does not configure PDM and I2S as non-secure peripherals for the nRF91 Series devices.
    See the NCSDK-24986 issue in the `Known Issues <known issues page on the main branch_>`_ page for more details.
