.. _ncs_release_notes_latest:

Changes in |NCS| v1.4.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.

.. note::
    This file is a work in progress and might not cover all relevant changes.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

nRF5340 SoC
-----------

* Updated:

  * ``bl_boot`` library - Disabled clock interrupts before booting the application.
    This change fixes an issue where the :ref:`bootloader` sample would not be able to boot a Zephyr application on the nRF5340 SoC.

Thread
------

* Added:

  * Development support for the nRF5340 in single-protocol configuration for the :ref:`ot_cli_sample`, :ref:`coap_client_sample`, and :ref:`coap_server_sample` samples.

* Optimized ROM and RAM used by Thread samples.

Common
======

The following changes are relevant for all device families.

Crypto
------

* Added:

  * nrf_cc3xx_platform v0.9.5, with the following highlights:

    * Added correct TRNG characterization values for nRF5340 devices.

    See the :ref:`crypto_changelog_nrf_cc3xx_platform` for detailed information.
  * nrf_cc3xx_mbedcrypto version v0.9.5, with the following highlights:

    * Built to match the nrf_cc3xx_platform v0.9.5 including correct TRNG characterization values for nRF5340 devices.

    See the :ref:`crypto_changelog_nrf_cc3xx_mbedcrypto` for detailed information.

* Updated:

  * Rewrote the :ref:`nrfxlib:nrf_security`'s library stripping mechanism to not use the POST_BUILD option in a custom build rule.
    The library stripping mechanism was non-functional in certain versions of SEGGER Embedded Studio Nordic Edition.

MCUboot
=======

sdk-mcuboot
-----------

The MCUboot fork in |NCS| contains all commits from the upstream MCUboot repository up to and including ``5a6e18148d``, plus some |NCS| specific additions.
The list of the most important recent changes can be found in :ref:`ncs_release_notes_140`.

sdk-nrfxlib
===========

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

Zephyr
======

sdk-zephyr
----------

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| contains all commits from the upstream Zephyr repository up to and including ``7a3b253ced``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 7a3b253ced ^v2.3.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^7a3b253ced

The current |NCS| release is based on Zephyr 2.4.0.
See the :ref:`Zephyr 2.4.0 release notes <zephyr:zephyr_2.4>` for a list of changes.

For the list of the most recent additions specific to |NCS|, see :ref:`ncs_release_notes_140`.
