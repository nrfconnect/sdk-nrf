.. _ncs_release_notes_changelog:

Changelog for |NCS| v1.7.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Highlights
**********

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.7.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Updated:

  * :ref:`lwm2m_client` sample:

    * Added support for Thingy:91.
    * Added more LwM2M objects.
    * LwM2M sensor objects now uses the actual sensors available to the Thingy:91. If the nRF9160 DK is used, it uses simulated sensors instead.
    * Added possibility to poll sensors and notify the server if the measured changes are large enough.

  * A-GPS library:

    * Removed GNSS socket API support.

  * :ref:`lib_nrf_cloud` library:

    * Removed GNSS socket API support from A-GPS and P-GPS.
    * Added support for sending data to a new bulk endpoint topic that is supported in nRF Cloud.
      A message published to the bulk topic is typically a combination of multiple messages.

  * :ref:`asset_tracker_v2` application:

    * Updated the application to start sending batch messages to the new bulk endpoint topic supported in nRF Cloud.

  * :ref:`multicell_location` sample:

    * Updated to only request neighbor cell measurements when connected and to only copy neighbor cell measurements if they exist.

  * :ref:`lte_lc_readme` library:

    * Changed the value of an invalid E-UTRAN cell ID from zero to UINT32_MAX for the LTE_LC_EVT_NEIGHBOR_CELL_MEAS event.

nRF5
====

nRF Desktop
-----------

* Updated:

  * Updated information about custom build types.

Zigbee
------

* Added:

   * :ref:`Zigbee shell <zigbee_shell_sample>`.

Common
======

Edge Impulse
------------

* Updated information about custom build types.

Common Application Framework (CAF)
----------------------------------


Updated:

* The power management support in modules is now enabled by default when the :kconfig:`CONFIG_CAF_PM_EVENTS` Kconfig option is enabled.

Hardware unique key
-------------------

* Make the checking for hw_unique_key_write_random() more strict; panic if any key is unwritten after writing random keys.


MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* No changes yet

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``14f09a3b00``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 14f09a3b00 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^14f09a3b00

The current |NCS| master branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``b77bfb047374b7013dbdf688f542b9326842a39e``.

The following list summarizes the most important changes inherited from the upstream Matter:

* No changes yet

Partition Manager
=================

* Added the ``share_size`` functionality to let a partition share size with a partition in another region.

Documentation
=============

There are no entries for this section yet.
