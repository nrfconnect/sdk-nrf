.. _dfu_samples:

DFU samples
############


This page lists samples related to Device Firmware Update (DFU) in |NCS|.
The following categories of samples are available:

* DFU samples - Samples demonstrating the DFU capabilities of |NCS|.
* Firmware loaders - Samples demonstrating firmware loader implementations.
* MCUboot configurations - Samples demonstrating the use of MCUboot features.
* nRF Secure Immutable Bootloader - Sample which is the |NSIB| implementation.

.. include:: ../samples.rst
    :start-after: samples_general_info_start
    :end-before: samples_general_info_end

|filter_samples_by_board|

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

.. toctree::
   :maxdepth: 1
   :caption: Firmware loaders:
   :glob:

   ../../../samples/dfu/fw_loader/*/README

.. toctree::
   :maxdepth: 1
   :caption: MCuboot configurations:
   :glob:

   ../../../samples/mcuboot/*/README
   ../../../samples/dfu/mcuboot_with_encryption/README
   ../../../samples/zephyr/smp_svr_mini_boot/README
   ../../../samples/dfu/compressed_update/README

.. toctree::
   :maxdepth: 1
   :caption: nRF Secure Immutable Bootloader:
   :glob:

    ../../../samples/bootloader/README
