.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.9.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections is kept when the changelog is cleaned.
   "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Highlights
**********

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.9.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Matter
------

|no_changes_yet_note|

Thread
------

* Added:

  *  Support for the Link Metrics and CSL Thread v1.2 features for the nRF53 Series devices.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Updated:

  * For nRF Cloud builds, the configuration section in the shadow is now initialized during the cloud connection process.

nRF Desktop
-----------

* Added:

  * Documentation for selective HID report subscription in :ref:`nrf_desktop_usb_state` using :kconfig:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION` option.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Updated:

  * :ref:`direct_test_mode` sample:

    * Fixed handling of the disable Constant Tone Extension command.
    * The front-end module test parameters are not set to their default value after the DTM reset command.
    * Added the vendor-specific ``FEM_DEFAULT_PARAMS_SET`` command for restoring the default front-end module parameters.

|no_changes_yet_note|

Bluetooth mesh samples
----------------------

* Updated

  * All samples are using the Partition Manager, replacing the use of the Device Tree Source flash partitions.

Thread samples
--------------

* Updated:

  * The relevant sample documentation pages with information about support for :ref:`Trusted Firmware-M <ug_tfm>`.
  * :ref:`ot_cli_sample` sample:

    * Added :file:`prj_thread_1_2.conf` to support Thread v1.2 build for the nRF52 and nRF53 Series devices.
    * Added child image configuration files for network core builds for Thread v1.2 build.

nrf9160 Samples
---------------

* Added:

  * Shell functionality to HTTP Update samples.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud_rest` library:

   * Added JSON Web Token (JWT) autogeneration feature.

     If enabled, the nRF Cloud REST library automatically generates a JWT if none is provided by the user when making REST requests.

  * Updated:

    * During the connection process, shadow data is sent to the application even if no "config" section is present.
    * The application can now send shadow updates earlier in the connection process.

Bluetooth libraries and services
--------------------------------

* :ref:`gatt_dm_readme` library:

  * Fixed discovery of empty services.

Libraries for networking
------------------------

* :ref:`lib_download_client` library:

  * Fixed

    * An issue where downloads of COAP URIs would fail when they contained multiple path elements.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Unity
-----

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``1eedec3e79``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``45ef0d2``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 45ef0d2 ^zephyr-v2.7.0

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^45ef0d2

The current |NCS| main branch is based on revision ``45ef0d2`` of Zephyr, which is located between v2.7.0 and v3.0.0 of Zephyr.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``77ab003e5fcd409cd225b68daa3cdaf506ed1107``.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

cddl-gen
========

The `cddl-gen`_ module has been updated from version 0.1.0 to 0.3.0.
Release notes for 0.3.0 can be found in :file:`ncs/nrf/modules/lib/cddl-gen/RELEASE_NOTES.md`.

The change prompted some changes in the CMake for the module, notably:

|no_changes_yet_note|

Documentation
=============

* Added:

  * Documentation for :ref:`degugging of nRF5340 <debugging>` in :ref:`working with nRF5340 DK<ug_nrf5340>` user guide.

* Removed:

  * Documentation on the Getting Started Assistant, as this tool is no longer in use.
    Linux users can install the |NCS| by using the `Installing using Visual Studio Code <Installing on Linux_>`_ instructions or by following the steps on the :ref:`gs_installing` page.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
