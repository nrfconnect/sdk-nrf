.. _ug_multi_image:

Multi-image builds
##################

In many cases, the firmware that is programmed to a device consists of not only one application, but several separate images, where one of the images (the *parent image*) requires one or more other images (the *child images*) to be present.
The child image then *chain-loads* (or *boots*) the parent image, which in turn might be a child image to another parent image and boot that one.
The most common use case for builds consisting of multiple images is an application that requires a bootloader to be present.


When to use multiple images
***************************

An *image* (also referred to as executable, program, or elf file) consists of pieces of code and data that are identified by image-unique names recorded in a single symbol table.
The symbol table exists as metadata in a ``.elf`` or ``.exe`` file and is not included when the image is converted to a HEX file for programming.
Instead, the code and data are placed at addresses by a linker.
This linking process is what distinguishes images from object files (which do not require linking).
Therefore, to determine if you have zero, one, or more images, count the number of times the linker runs.

Using multiple images has the following advantages:

* You can run the linker multiple times and partition the final firmware into several regions.
  This partitioning is often useful for bootloaders.
* Since there is a symbol table for each image, the same symbol names can exist multiple times in the final firmware.
  This is useful for bootloader images, which might required their own copy of a library that the application uses, but in a different version or configuration.

In the |NCS|, multiple images are required in the following scenarios:

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
   The MCUboot bootloader is used in the :ref:`http_application_update_sample` sample.

nRF5340 support
   nRF5340 contains two separate processors: a network core and an application core.
   When programming applications to the nRF5340 PDK, they must be divided into at least two images, one for each core.
   See :ref:`ug_nrf5340` for more information.

   .. important::
      Currently, the two images must be built and programmed separately.
      Multi-image builds are not yet supported for nRF5340.


Default configuration
*********************

The |NCS| samples are set up to build all related images as one solution, starting from the parent image.
This is referred to as *multi-image build*.

When building the parent image, you can configure how the child image should be handled:

* Build the child image from source and include it with the parent image.
  This is the default setting.
* Use a prebuilt HEX file of the child image and include it with the parent image.
* Ignore the child image.

When building the child image from source or using a prebuilt HEX file, the build system merges the HEX files of the parent and child image together so that they can easily be programmed in one single step.
This means that you might enable and integrate an additional image without even realizing it, just by using the default configuration.

To change the default configuration and configure how a child image is handled, locate the BUILD_STRATEGY configuration options for the respective child image in the parent image configuration.
For example, to use a prebuilt HEX file of the :ref:`secure_partition_manager` instead of building it, select :option:`CONFIG_SPM_BUILD_STRATEGY_USE_HEX_FILE` instead of the default :option:`CONFIG_SPM_BUILD_STRATEGY_FROM_SOURCE`, and specify the HEX file in :option:`CONFIG_SPM_HEX_FILE`.
To ignore an MCUboot child image, select :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_SKIP_BUILD` instead of :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE`.


Defining and enabling a child image
***********************************

You can enable existing child images in the |NCS| by enabling the respective modules in the parent image and selecting the desired build strategy.
To turn an application that you have implemented into a child image that can be included in a parent image, you must update the build scripts to make it possible to enable the child image and add the required configuration options.
You should also know how image-specific variables are disambiguated and what targets of the child images are available.

Updating the build scripts
==========================

To make it possible to enable a child image from a parent image, you must include the child image in the build script.

This code should be put in place in the cmake tree that is conditional on a configuration option for having the parent image use the child image.
In the |NCS|, the code is included in the :file:`CMakeLists.txt` file for the samples, and in the MCUboot repository.

See the following example code:

.. code-block:: cmake

   if (CONFIG_SPM)
     add_child_image(spm ${CMAKE_CURRENT_LIST_DIR}/nrf9160/spm)
   endif()

   if (CONFIG_SECURE_BOOT)
     add_child_image(b0 ${CMAKE_CURRENT_LIST_DIR}/bootloader)
   endif()

   if (CONFIG_BOOTLOADER_MCUBOOT)
     add_child_image(mcuboot ${MCUBOOT_BASE}/boot/zephyr)
   endif()

In this code, ``add_child_image`` registers the child image with the given name and file path and executes the build scripts of the child image.
Note that both the child image's application build scripts and the core build scripts are executed.
The core build scripts might use a different configuration and possibly different DeviceTree settings.

Adding configuration options
============================

When enabling a child image, you select the build strategy, thus how the image is included.
The three options are:

* Build the child image from source along with the parent image - *IMAGENAME*\_BUILD_STRATEGY_FROM_SOURCE
* Merge the specified HEX file of the child image with the parent image - *IMAGENAME*\_BUILD_STRATEGY_USE_HEX_FILE, and *IMAGENAME*\_HEX_FILE to specify the HEX file
* Ignore the child image when building and build only the parent image - *IMAGENAME*\_BUILD_STRATEGY_SKIP_BUILD

You must add these four configuration options to the Kconfig file for your child image, replacing *IMAGENAME* with the (uppercase) name of your child image (as specified in ``add_child_image``).

The following example shows the configuration options for MCUboot:

.. code-block:: Kconfig

   choice
  	prompt "MCUboot build strategy"
  	default MCUBOOT_BUILD_STRATEGY_FROM_SOURCE

   config MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE
  	# Mandatory option when being built through 'add_child_image'
  	bool "Use HEX file instead of building MCUboot"

   if MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE

   config MCUBOOT_HEX_FILE
  	# Mandatory option when being built through 'add_child_image'
  	string "MCUboot HEX file"

   endif # MCUBOOT_USE_HEX_FILE

   config MCUBOOT_BUILD_STRATEGY_SKIP_BUILD
  	# Mandatory option when being built through 'add_child_image'
  	bool "Skip building MCUboot"

   config MCUBOOT_BUILD_STRATEGY_FROM_SOURCE
  	# Mandatory option when being built through 'add_child_image'
  	bool "Build from source"

   endchoice


Image-specific variables
========================

The child image and parent image are executed in different CMake processes and thus have different namespaces.
Variables in the parent image are not propagated to the child image, with the following exceptions:

* Any variable named *IMAGENAME*\_FOO in a parent image is propagated to the child image named *IMAGENAME* as FOO.
* Variables that are in the list ``SHARED_MULTI_IMAGE_VARIABLES`` are propagated to all child images.

With these two mechanisms, it is possible to set variables in child images from either parent images or the command line, and it is possible to set variables globally across all images.
For example, to change the ``CONF_FILE`` variable for the MCUboot image and the parent image, specify the CMake command as follows::

   cmake -Dmcuboot_CONF_FILE=prj_a.conf -DCONF_FILE=app_prj.conf

The command line that is used to create the child images can be extended by adding flags to the CMake variable ``EXTRA_MULTI_IMAGE_CMAKE_ARGS``.
This could for instance be used to get more debug information with the flag ``---trace-expand``.

Similarly the CMake variable ``EXTRA_MULTI_IMAGE_BUILD_OPT`` can be used to modify the options used when ninja is invoked on the child images.

Child image targets
===================

You can indirectly invoke a selection of child image targets from the parent image.
Currently, the child targets that can be invoked from the parent targets are ``menuconfig``, ``guiconfig``, and any targets listed in ``EXTRA_KCONFIG_TARGETS``.

To disambiguate targets, the same prefix convention is used as for variables.
This means that to run menuconfig, for example, you invoke the ``menuconfig`` target to configure the parent image and ``mcuboot_menuconfig`` to configure the MCUboot child image.

You can also invoke any child target directly from its build directory.
Child build directories are located at the top of the parent's build directory.

Memory placement
****************

In a multi-image build, all images must be placed in memory so that they do not overlap.
The flash start address for each image must be specified by, for example, :option:`CONFIG_FLASH_LOAD_OFFSET`.
Hardcoding the image locations like this works fine for simple use cases like a bootloader that prepares a device, where there are no further requirements on where in memory each image must be placed.
However, more advanced use cases require a memory layout where images are located in a specific order relative to one another.
The |NCS| provides a Python tool that allows to specify this kind of relative placement, or even a static placement based on start addresses and sizes for the different images.
See :ref:`partition_manager` for more information about how to set up your partitions and configure your build system.
