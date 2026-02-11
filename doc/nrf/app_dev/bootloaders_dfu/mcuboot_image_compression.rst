.. _mcuboot_image_compression:

MCUboot image compression
#########################

.. contents::
   :local:
   :depth: 2

:ref:`MCUboot <mcuboot:mcuboot_wrapper>` in the |NCS| optionally supports compressed image updates.

The system includes the following features and limitation:

* Allows slot ``1`` to be approximately 70% the size of slot ``0``.
* Supports a single-image only.
  It does not support network core updates for the nRF5340 SoC or :ref:`bootloader` updates.
* Does not support reverting to previous versions.
  MCUboot must be configured for upgrade-only mode.
* Validates the compressed image during the update process before the main image is erased, ensuring the update does not lead to a bricked module due to un-loadable firmware.
* Does not support image encryption.
* Uses LZMA2 compression with ARM thumb filter for compressed images.
* Must use :ref:`configuration_system_overview_sysbuild`.

Sample
******

For a demonstration of this feature, see the :ref:`nrf_compression_mcuboot_compressed_update` sample.
This sample already implements the configuration requirements mentioned in :ref:`mcuboot_image_compression_setup` below.

.. _mcuboot_image_compression_setup:

Required setup
**************

You must meet the following configuration requirements for this feature to work.

Devicetree partitioning for MCUboot
===================================

The devicetree NVM partitioning for the board file should include the following partitions:

  * ``boot_partition`` - Requires a minimum size of 48 KiB for a minimal build without logging.
  * ``slot0_partition`` - Should be able to fit the largest image, including signing/imgtool overheads.
  * ``slot1_partition`` - Should be approximately 70% of the size of the slot ``0`` partition for optimal configuration, assuming that image savings will be 30%.
    The total compression depends on the data within the image.

Example layout
--------------

The following shows example partitioning for image compression:

.. tabs::

    .. group-tab:: nRF52840

        .. literalinclude:: ../../../../samples/nrf_compress/mcuboot_update/boards/nrf52840dk_nrf52840.overlay
             :language: devicetree
             :lines: 12-

    .. group-tab:: nRF5340

        .. literalinclude:: ../../../../samples/nrf_compress/mcuboot_update/boards/nrf5340dk_nrf5340_cpuapp.overlay
             :language: devicetree
             :lines: 12-

    .. group-tab:: nRF54L15

        .. literalinclude:: ../../../../samples/nrf_compress/mcuboot_update/boards/nrf54l15dk_nrf54l15_cpuapp.overlay
             :language: devicetree
             :lines: 12-

Required sysbuild configuration options
=======================================

The following :ref:`configuration_system_overview_sysbuild` Kconfig options are required to enable support for compressed images:

* :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`
* :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY`
* :kconfig:option:`SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT`

See :ref:`configuring_kconfig` for different methods of configuring these options.
You want to have the following configuration in your application:

.. code-block:: cfg

    SB_CONFIG_BOOTLOADER_MCUBOOT=y
    SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY=y
    SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT=y
