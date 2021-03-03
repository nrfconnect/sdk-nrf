.. _ncs_release_notes_latest:

Changes in |NCS| v1.5.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Highlights
**********

There are no entries for this section yet.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF5
====

There are no entries for this section yet.

nRF9160
=======

* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Added cell-based location support to :ref:`lib_nrf_cloud_agps`.
    * Added Kconfig option :option:`CONFIG_NRF_CLOUD_AGPS_SINGLE_CELL_ONLY` to obtain cell-based location from nRF Connect for Cloud instead of using the modem's GPS.

  * :ref:`asset_tracker` application:

    * Updated to handle new Kconfig options:

      * :option:`CONFIG_NRF_CLOUD_AGPS_SINGLE_CELL_ONLY`
      * :option:`CONFIG_NRF_CLOUD_AGPS_REQ_CELL_BASED_LOC`

  * A-GPS library:

    * Added the Kconfig option :option:`CONFIG_AGPS_SINGLE_CELL_ONLY` to support cell-based location instead of using the modem's GPS.

  * :ref:`modem_info_readme` library:

    * Updated to prevent reinitialization of param list in :c:func:`modem_info_init`.

  * :ref:`lib_fota_download` library:

    * Added an API to retrieve the image type that is being downloaded.

Common
======

There are no entries for this section yet.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``3fc59410b6``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* No changes yet

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``ff720cd9b343``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline ff720cd9b343 ^v2.4.99-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^ff720cd9b343

The current |NCS| release is based on Zephyr v2.4.99.

The following list summarizes the most important changes inherited from upstream Zephyr:

* No changes yet

Documentation
=============

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.5.0`_ for the list of issues valid for this release.
