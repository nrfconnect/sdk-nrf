.. _ncs_release_notes_242:

|NCS| v2.4.2 Release Notes
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

This patch release adds changes on top of the :ref:`nRF Connect SDK v2.4.0 <ncs_release_notes_240>` and :ref:`nRF Connect SDK v2.4.1 <ncs_release_notes_241>`.
The changes have an effect on the :ref:`nrfxlib:nrf_modem` library.

See :ref:`ncs_release_notes_242_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.4.2**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.4.2`_.

IDE and tool support
********************

`nRF Connect for Visual Studio Code extension <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.4.2.
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
     - `Changelog <Modem library changelog for v2.4.2_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.4.2_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.4.2`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_242_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Samples
=======

Cellular samples
----------------

:ref:`lwm2m_client` sample:

* Added missing ``lwm2m_rd_client_start()`` call error handling.

nrfxlib
=======

This patch release adds a workaround in the :ref:`nrfxlib:nrf_modem` for modem firmware v1.3.4 and v1.3.5 where attaching to the network would fail with error ``90`` (UICC initialization failure) after performing a modem firmware update, until the modem is re-initialized.

For more information, see item NRF91-1702 in the `Known issues page <known issues page on the main branch_>`_.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``4bbd91a9083a588002d4397577863e0c54ba7038``.

For a complete list of |NCS| specific commits and cherry-picked commits since v2.4.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^v3.3.99-ncs1
