.. _ncs_release_notes_3.1.1:

|NCS| v3.1.1 Release Notes
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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.1.0 <ncs_release_notes_3.1.0>`:

This is the first release of the |NCS| that brings comprehensive :ref:`experimental <software_maturity>` support for the nRF54LM20 DK and nRF54LM20A SoC.
See the `nRF54LM20A System-on-Chip`_ page to learn more.

* This release provides experimental support for the nRF54LM20A SoC with the following features:

  * Protocols:

    * Bluetooth® LE, including Channel Sounding
    * 2.4 Ghz proprietary
    * Thread 1.4
    * Matter-over-Thread 1.4.2

  * All nRF54LM20A SoC peripherals, including the new USB-HS interface.
  * DFU with MCUboot enabling dual-bank DFU with optional external flash.
  * Hardware crypto provided by :ref:`PSA Crypto APIs (hardware accelerated) <ug_nrf54l_cryptography>` for cryptographic operations.
  * Google Find My Device and Apple Find My for members of the Apple and Google programs.
  * Out-of-the-box support across many standard SDK samples.

* Matter:

  * Included fixes to achieve compatibility with Matter 1.4.2 Test Harness and to pass the following Matter certification test cases:

    * TC-IDM-10.3
    * TC-OPCREDS-3.8

* PMIC:

  * Added nPM1304 support to the :ref:`npm13xx_fuel_gauge` and :ref:`npm13xx_one_button` samples.

* Cellular:

  * Added support for the Sercomm TPM530M module and the TPM530M EVK.

See :ref:`ncs_release_notes_311_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.1.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.1.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.1.1.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the `Programmer app`_ of `nRF Connect for Desktop`_ to update the modem firmware.
See the `Programming nRF91 Series DK firmware`_ page for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v3.1.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.1.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.1.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_311_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Added macOS 26 support (Tier 3) to the table listing :ref:`supported operating systems for proprietary tools <additional_nordic_sw_tools_os_support>`.
* Updated the required `SEGGER J-Link`_ version to v8.60.

Developing with PMICs
=====================

* Added the :ref:`ug_npm1304_developing` documentation.

Developing with coprocessors
============================

* Added support for the nRF54LM20 FLPR.

Security
========

* Added CRACEN and nrf_oberon driver support for nRF54LM20.
  For the list of supported features and limitations, see the :ref:`ug_crypto_supported_features` page.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Matter
------

* Added support for the :zephyr:board:`nrf54lm20dk` board.
* Included fixes for the following certification test cases:

  * TC-IDM-10.3
  * TC-OPCREDS-3.8

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Desktop
-----------

* Added application configurations for the :zephyr:board:`nrf54lm20dk` board.
  The configurations are supported through the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.
  For details, see the :ref:`nrf_desktop_board_configuration` page.
  The :zephyr:board:`nrf54lm20dk` board support is experimental.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added experimental support for the :zephyr:board:`nrf54lm20dk` board in the following samples:

  * :ref:`bluetooth_central_hids`
  * :ref:`peripheral_hids_keyboard`
  * :ref:`peripheral_hids_mouse`

Bluetooth Fast Pair samples
---------------------------

* Added experimental support for the :zephyr:board:`nrf54lm20dk` board in all Fast Pair samples.

Matter samples
--------------

* Added support for the :zephyr:board:`nrf54lm20dk` board in all Matter samples.

PMIC samples
------------

* Updated:

  * By renaming the nPM1300: Fuel Gauge sample to :ref:`npm13xx_fuel_gauge`.
    The wiring is changed for all targets.
    Refer to the :ref:`Wiring table <npm13xx_fuel_gauge_wiring>` in the sample documentation for details.
  * By renaming the nPM1300: One button sample to :ref:`npm13xx_one_button`.
    The wiring is changed for all targets.
    Refer to the :ref:`Wiring table <npm13xx_one_button_wiring>` in the sample documentation for details.

Thread samples
--------------

* Added support for the :zephyr:board:`nrf54lm20dk` board in the :ref:`ot_cli_sample` and :ref:`ot_coprocessor_sample` samples.

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Added support for the :zephyr:board:`nrf54lm20dk` board.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``0fe59bf1e4b96122c3467295b09a034e399c5ee6``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.1.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.1.0
