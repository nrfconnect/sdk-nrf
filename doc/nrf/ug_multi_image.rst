.. _ug_multi_image:

Multi-image builds
##################

In many cases, the firmware that is programmed to a device consists of not only one application, but several separate images, where one of the images (the *parent image*) requires one or more other images (the *child images*) to be present.
The most common use case for builds consisting of multiple images is an application that requires a bootloader to be present.

In the context of |NCS|, multiple images are required in the following scenarios:

nRF9160 SPU configuration
   The nRF9160 SiP application MCU is divided into a secure and a non-secure domain.
   The code in the secure domain can configure the System Protection Unit (SPU) to allow non-secure access to the CPU resources that are required by the application, and then jump to the code in the non-secure domain.
   Therefore, the nRF9160 samples (the parent image) require the :ref:`secure_partition_manager` (the child image) to be programmed in addition to the actual application.

   See :ref:`zephyr:nrf9160_pca10090` and :ref:`ug_nrf9160` for more information.

MCUboot bootloader
   The MCUboot bootloader establishes a root of trust by verifying the next step in the boot sequence.
   This first-stage bootloader is immutable, which means it must never be updated or deleted.
   However, it allows to update the application, and therefore MCUboot and the application must be located in different images.
   In this scenario, the application is the parent image and MCUboot is the child image.

   See :ref:`about_mcuboot` for more information.

In such cases where the firmware consists of several images, you can build each of the images separately and program it to the correct place in flash.

Alternatively, you can build the related images as one solution, starting from the parent image.
This is referred to as *multi-image build*, and it is the default way how the |NCS| samples are set up.
When building the parent image, you can configure if a child image should automatically be built and included in the parent image (the default), if a prebuilt HEX file should be included instead of the child image, or if the child image should be ignored.
If you are creating your own application, see *Building and Configuring multiple images* in :ref:`zephyr:application` for more information about how to turn your application into a parent image that requires specific child images.

In a multi-image build, all images must be placed in memory so that they do not overlap.
The flash start address for each image must be specified by, for example, :option:`CONFIG_FLASH_LOAD_OFFSET`.
Hardcoding the image locations like this works fine for simple use cases like a bootloader that prepares a device, where there are no further requirements on where in memory each image must be placed.
However, more advanced use cases require a memory layout where images are located in a specific order relative to one another.
The |NCS| provides a Python tool that allows to specify this kind of relative placement, or even a static placement based on start addresses and sizes for the different images.
See :ref:`partition_manager` for more information about how to set up your partitions and configure your build system.
