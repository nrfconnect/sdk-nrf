.. _ncs_release_notes_212:

|NCS| v2.1.2 Release Notes
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

This minor release adds the following changes on top of the :ref:`nRF Connect SDK v2.1.0 <ncs_release_notes_210>` and :ref:`nRF Connect SDK v2.1.1 <ncs_release_notes_211>`:

* Changes to the Bluetooth® mesh solution to improve performance and stability using extended advertiser.
* Changes to Emergency Data Storage to improve performance and stability.

See :ref:`ncs_release_notes_212_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.1.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.1.2`_.

IDE and tool support
********************

`nRF Connect for Visual Studio Code extension <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.1.2.
SEGGER Embedded Studio Nordic Edition is no longer tested or recommended for new projects.

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
     - `Changelog <Modem library changelog for v2.1.2_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.1.2_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.1.2`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_212_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Bluetooth mesh
--------------

* Fixed bugs that appeared when using extended advertiser.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* :ref:`bluetooth_mesh_light_lc` sample:

  * Removed the :c:func:`bt_disable` function call from the interrupt context before calling the :c:func:`emds_store` function.
  * Added explicit IRQ locks for RTC0, TIMER0 and RADIO peripherals used by SoftDevice Controller.

Libraries
=========
This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated the glue layer to facilitate switching from NB-IoT to LTE-M during in-band FOTA.

Other libraries
---------------

* :ref:`emds_readme` library:

  * Removed the internal thread for storing the emergency data.
    The emergency data is now stored by the :c:func:`emds_store` function.
  * Updated the library implementation to bypass the flash driver when storing the emergency data.
    This allows calling the :c:func:`emds_store` function from an interrupt context.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``71ef669ea4a73495b255f27024bcd5d542bf038c``.
This is the same commit ID as the one used for |NCS| :ref:`v2.1.0 <ncs_release_notes_210>` and :ref:`v2.1.1 <ncs_release_notes_211>`.
It also includes some |NCS| specific additions and commits cherry-picked from the upstream Zephyr repository.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.1.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.1.99-ncs1
