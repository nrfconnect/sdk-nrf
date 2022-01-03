.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.8.99
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

* |no_changes_yet_note|

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.8.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

Using Edge Impulse
------------------

* Added instruction on how to download a model from a public Edge Impulse project.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth
---------

* |no_changes_yet_note|

Matter
------

* Added ``EXPERIMENTAL`` select in Kconfig that informs that Matter support is experimental.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* |no_changes_yet_note|

nRF Desktop
-----------

* Added:

  * Added possibility to ask for bootloader variant using config channel.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`multiple_adv_sets` sample.

* Updated:

  * :ref:`peripheral_rscs` - Corrected the number of bytes for setting the Total Distance Value and specified the data units.
  * :ref:`direct_test_mode` - Added support for front-end module devices that support 2-pin PA/LNA interface with additional support for the Skyworks SKY66114-11 and the Skyworks SKY66403-11.
  * :ref:`peripheral_hids_mouse` - Added a notice about encryption requirement.
  * :ref:`peripheral_hids_keyboard` - Added a notice about encryption requirement.


nRF9160 samples
---------------

* :ref:`modem_shell_application` sample:

  * Added a new shell command ``cloud`` for establishing an MQTT connection to nRF Cloud.

* :ref:`gnss_sample` sample:

  * Added support for minimal assistance using factory almanac, time and location.

Other samples
-------------

:ref:`radio_test` - Added support for front-end module devices that support 2-pin PA/LNA interface with additional support for the Skyworks SKY66114-11 and the Skyworks SKY66403-11.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* |no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

Updated:

* :ref:`rscs_readme` library:

  * Added units for :c:struct:`bt_rscs_measurement` members.

Common Application Framework (CAF)
----------------------------------

* Migrated :ref:`nRF Desktop settings loader <nrf_desktop_settings_loader>` to :ref:`lib_caf` as :ref:`CAF: Settings loader module <caf_settings_loader>`.

* Updated:

  * Unify module id reference location.
    The array holding module reference objects is explicitly defined in linker script to avoid creating an orphan section.
    ``MODULE_ID`` macro and :c:func:`module_id_get` function now returns module reference from dedicated section instead of module name.
    The module name can not be obtained from reference object directly, a helper function (:c:func:`module_name_get`) should be used instead.

Bootloader libraries
--------------------

* Added a separate section for :ref:`lib_bootloader`.

Modem libraries
---------------

* :ref:`at_cmd_parser_readme` library:
  * Can now parse AT command responses containing the response result, for example, ``OK`` or ``ERROR``.

Libraries for networking
========================

* :ref:`lib_fota_download` library:

  * Fixed an issue where the application would not be notified of errors originating from inside :c:func:`download_with_offset`. In the http_update samples, this would result in the dfu start button interrupt being disabled after a connect error in :c:func:`download_with_offset` after a disconnect during firmware download.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Other libraries
---------------

* Moved :ref:`lib_bootloader` to a section of their own.
  * Added write protection by default for the image partition.

Modem library
+++++++++++++

* |no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Partition Manager
-----------------

* |no_changes_yet_note|

HID Configurator
----------------

* Added:

  * HID Configurator now recognizes the bootloader variant as a DFU module variant for the configuration channel communication.
    The new implementation is not backward compatible, since new version of script checks for variant it expects firmware to support this request.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``3f82656``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 3f82656 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^3f82656

The current |NCS| main branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``bbd19d92f6d58ef79c98793fe0dfb2979db6336d``.

The following list summarizes the most important changes inherited from the upstream Matter:

* |no_changes_yet_note|

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* Reorganized the contents of the :ref:´ug_app_dev` section:

  * Added new subpage :ref:`app_optimize` and moved the optimization sections under it.
  * Added new subpage :ref:`ext_components` and moved the sections for using external components or modules under it.

* Reorganized the contents of the :ref:`protocols` section:

  * Reduced the ToC levels of the subpages.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
