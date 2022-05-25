.. _ncs_release_notes_121:

|NCS| v1.2.1 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

|NCS| v1.2.1 supports product development with the nRF9160 Cellular IoT device.
It contains reference applications, sample source code, and libraries for developing low-power wireless applications with nRF52 and nRF53 Series devices, though support for these devices is incomplete and not recommended for production.

.. note::

   Before this release, the |NCS| moved to a new GitHub organization, `nrfconnect <https://github.com/nrfconnect>`_.
   All repositories were renamed to provide clear and concise names that better reflect the composition of the codebase.
   If you are a new |NCS| user, there are no actions you need to take.
   If you used previous releases of the |NCS|, you should :ref:`point your repositories to the right remotes <repo_move>`.

Highlights
**********

* Updates to LwM2M carrier library
* Updates to BSD library


Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.2.1**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories_win` for more information.


Supported modem firmware
************************

This version of the |NCS| supports the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.1.2

Use the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Tested boards
*************

The following boards have been used during testing of this release:

* PCA10090 (nRF9160 DK)
* PCA20035 (Thingy:91)
* PCA10095 (nRF5340 PDK)
* PCA10056 (nRF52840 DK)
* PCA10059 (nRF52840 Dongle)
* PCA10040 (nRF52 DK)

For the full list of supported devices and boards, see :ref:`zephyr:boards` in the Zephyr documentation.


Required tools
**************

In addition to the tools mentioned in :ref:`gs_installing`, the following tool versions are required to work with the |NCS|:

.. list-table::
   :header-rows: 1

   * - Tool
     - Version
     - Download link
   * - SEGGER J-Link
     - V6.60e
     - `J-Link Software and Documentation Pack`_
   * - nRF Command Line Tools
     - v10.6.0
     - `nRF Command Line Tools`_
   * - nRF Connect for Desktop
     - v3.3.0 or later
     - `nRF Connect for Desktop`_
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools`
   * - GCC
     - See Install the GNU Arm Embedded Toolchain
     - `GNU Arm Embedded Toolchain`_


As IDE, we recommend to use SEGGER Embedded Studio (Nordic Edition) version 4.52.
It is available from the following platforms:

* Windows x86
* Windows x64
* Mac OS x64
* Linux x86
* Linux x64

Changelog
*********

The following sections provide detailed lists of changes by component.


nRF9160
=======

* :ref:`lib_download_client`:

  * Fixed DNS lookup when using non-default PDN.

* :ref:`liblwm2m_carrier_readme`:

  * Updated to version 0.8.2.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

* BSD library:

  * Updated to version 0.6.1.2.
    See the :ref:`nrf_modem_changelog` for detailed information.
