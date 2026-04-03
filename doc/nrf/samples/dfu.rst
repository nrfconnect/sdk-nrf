.. _dfu_samples:

DFU samples
############

This page lists |NCS| samples demonstrating the use of Device Firmware Update (DFU) libraries.

.. include:: ../samples.rst
    :start-after: samples_general_info_start
    :end-before: samples_general_info_end

|filter_samples_by_board|

Samples demonstrating the DFU capabilities of |NCS|:

.. toctree::
   :maxdepth: 1
   :caption: DFU samples:
   :glob:

   ../../../samples/dfu/smp_svr/README
   ../../../samples/dfu/ab/README
   ../../../samples/dfu/ab_split/README
   ../../../samples/dfu/dfu_multi_image/README
   ../../../samples/dfu/dfu_target/README
   ../../../samples/dfu/single_slot/README

Samples demonstrating the firmware loader implementations.

.. toctree::
   :maxdepth: 1
   :caption: Firmware loaders:
   :glob:

   ../../../samples/dfu/fw_loader/*/README

Samples demonstrating the use of MCUboot peculiar features.

.. toctree::
   :maxdepth: 1
   :caption: MCuboot features demonstration:
   :glob:

   ../../../samples/mcuboot/*/README
   ../../../samples/dfu/mcuboot_with_encryption/README
   ../../../samples/zephyr/smp_svr_mini_boot/README
   ../../../samples/dfu/compressed_update/README

The sample which is |NSIB| implementation.

.. toctree::
   :maxdepth: 1
   :caption: NSIB:
   :glob:

    ../../../samples/bootloader/README
