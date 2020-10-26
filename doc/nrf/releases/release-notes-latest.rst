.. _ncs_release_notes_latest:

Changes in |NCS| v1.3.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.
Note that this file is a work in progress and might not cover all relevant changes.


Changelog
*********

See the `master branch section in the changelog`_ for a list of the most important changes.


sdk-mcuboot
===========

The MCUboot fork in |NCS| contains all commits from the upstream MCUboot repository up to and including ``5a6e18148d``, plus some |NCS| specific additions.

The following list summarizes the most important changes inherited from upstream MCUboot:

  * Fixed an issue where after erasing an image, an image trailer might be left behind.
  * Added a ``CONFIG_BOOT_INTR_VEC_RELOC`` option to relocate interrupts to application.
  * Fixed single image compilation with serial recovery.
  * Added support for single-image applications (see `CONFIG_SINGLE_IMAGE_DFU`).
  * Added a ``CONFIG_BOOT_SIGNATURE_TYPE_NONE`` option to disable the cryptographic check of the image.
  * Reduced the minimum number of members in SMP packet for serial recovery.
  * Introduced direct execute-in-place (XIP) mode (see ``CONFIG_BOOT_DIRECT_XIP``).
  * Fixed kramdown CVE-2020-14001.
  * Modified the build system to let the application use a private key that is located in the user project configuration directory.
  * Added support for nRF52840 with ECC keys and CryptoCell.
  * Allowed to set VTOR to a relay vector before chain-loading the application.
  * Allowed using a larger primary slot in swap-move.
    Before, both slots had to be the same size, which imposed an unused sector in the secondary slot.
  * Fixed bootstrapping in swap-move mode.
  * Fixed an issue that caused an interrupted swap-move operation to potentially brick the device if the primary image was padded.
  * Various fixes, enhancements, and changes needed to work with the latest Zephyr version.

sdk-nrfxlib
===========

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for the most current information.

sdk-zephyr
==========

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

The following list contains |NCS| specific additions:

* Added support for the |NCS|'s :ref:`partition_manager`, which can be used for flash partitioning.
* Added the following network socket and address extensions to the :ref:`zephyr:bsd_sockets_interface` interface to support the functionality provided by the :ref:`nrfxlib:bsdlib`:

  AF_LTE, NPROTO_AT, NPROTO_PDN, NPROTO_DFU, SOCK_MGMT, SO_RCVTIMEO, SO_BINDTODEVICE, SOL_PDN, SOL_DFU, SO_PDN_CONTEXT_ID, SO_PDN_STATE, SOL_DFU, SO_DFU_ERROR, TLS_SESSION_CACHE, SO_SNDTIMEO, MSG_TRUNC, SO_SILENCE_ALL, SO_IP_ECHO_REPLY, SO_IPV6_ECHO_REPLY
* Added support for enabling TLS caching when using the :ref:`zephyr:mqtt_socket_interface` library.
  See :c:macro:`TLS_SESSION_CACHE`.
* Updated the nrf9160ns DTS to support accessing the CryptoCell CC310 hardware from non-secure code.
