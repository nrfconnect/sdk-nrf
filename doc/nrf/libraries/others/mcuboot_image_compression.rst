.. _mcuboot_image_compression:

MCUboot image compression
#########################

MCUboot in the |NCS| optionally supports compressed image updates.
The system includes the following features and limitation:

* Allows slot 1 to be approximately 70% the size of slot 0.
* Supports a single image only.
  It does not support network core updates for the nRF5340 SoC or NSIB updates.
* Does not support reverting to previous versions.
  MCUboot must be configured for upgrade-only mode.
* Validates the compressed image during the update process before the main image is erased, ensuring the update does not lead to a bricked module due to un-loadable firmware.
* Does not support image encryption.
* Uses LZMA2 compression with ARM thumb filter for compressed images.
* Requires a static Partition Manager file.
* Must use sysbuild.

Sample
******

A sample demonstrating compressed MCUboot image support is offered, see :ref:`mcuboot_compressed_update` for details.

Setup
*****

Static Partition Manager file for MCUboot
-----------------------------------------

The static Partition Manager file should include the following partitions:

  * ``boot_partition``: Requires a minimum size of 48 KiB for a minimal build without logging.
  * ``slot1_partition``: Should be approximately 70% of the size of slot 0 partition for optimal configuration, assuming that image savings will be 30%.
    The total compression depends on the data within the image.

The following shows an example static partition manager layout for image compression:

.. tabs::

    .. group-tab:: nRF52840

        .. literalinclude:: ../../../../tests/subsys/nrf_compress/decompression/mcuboot_update/pm_static_nrf52840dk_nrf52840.yml
             :language: yaml

    .. group-tab:: nRF5340

        .. literalinclude:: ../../../../tests/subsys/nrf_compress/decompression/mcuboot_update/pm_static_nrf5340dk_nrf5340_cpuapp.yml
             :language: yaml

    .. group-tab:: nRF54L15

        .. literalinclude:: ../../../../tests/subsys/nrf_compress/decompression/mcuboot_update/pm_static_nrf54l15dk_nrf54l15_cpuapp.yml
             :language: yaml

Sysbuild
********

The following sysbuild Kconfig options are required to enable support for compressed images:

.. code-block:: cfg

    SB_CONFIG_BOOTLOADER_MCUBOOT=y
    SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY=y
    SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT=y
