.. _ug_multi_image:

Multi-image builds
##################

.. contents::
   :local:
   :depth: 2

The firmware programmed to a device can be composed of either one application or several separate images.
In the latter, one of the images, the *parent image*, requires one or more other images, the *child images*, to be present.
The child image then *chain-loads*, or *boots*, the parent image, which could also be a child image to another parent image, and boots that one.

The most common use cases for builds consisting of multiple images are applications that require a bootloader to be present or applications for multi-core CPUs.

When to use multiple images
***************************

An *image*, also referred to as *executable*, *program*, or *elf file*, consists of pieces of code and data that are identified by image-unique names recorded in a single symbol table.
The symbol table exists as metadata in a ``.elf`` or ``.exe`` file and is not included when the image is converted to a HEX file for programming.
Instead, a linker places the code and data at their addresses.

Only images require this linking process.
Object files do not require linking.
To determine if you have zero, one, or more images, count the number of times the linker runs.

Using multiple images has the following advantages:

* You can run the linker multiple times and partition the final firmware into several regions.
  This partitioning is often useful for bootloaders.
* Since there is a symbol table for each image, the same symbol names can exist multiple times in the final firmware.
  This is useful for bootloader images, which can require their own copy of a library that the application uses, but in a different version or configuration.
* In multi-core builds, the build configuration of a child image in a separate core can be made known to the parent image.

In the |NCS|, multiple images are required in the following scenarios:

nRF9160 SPU configuration
   The nRF9160 SiP application MCU is divided into a secure and non-secure domain.
   The code in the secure domain can configure the System Protection Unit (SPU) to allow non-secure access to the CPU resources that are required by the application, and then jump to the code in the non-secure domain.
   Therefore, each nRF9160 sample, the parent image, requires the :ref:`secure_partition_manager` (or :ref:`TF-M <ug_tfm>`), the child image, to be programmed together with the actual application.

   See :ref:`zephyr:nrf9160dk_nrf9160` and :ref:`ug_nrf9160` for more information.

MCUboot bootloader
   The MCUboot bootloader establishes a root of trust by verifying the next step in the boot sequence.
   This first-stage bootloader is immutable, which means it must never be updated or deleted.
   However, it allows to update the application, and therefore MCUboot and the application must be located in different images.
   In this scenario, the application is the parent image and MCUboot is the child image.

   See :doc:`MCUboot documentation <mcuboot:index>` for more information.
   The MCUboot bootloader is used in the :ref:`http_application_update_sample` sample.

nRF5340 support
   nRF5340 contains two separate processors: a network core and an application core.
   When programming applications to the nRF5340 DK, they must be divided into at least two images, one for each core.

   See :ref:`ug_nrf5340` for more information.

Default configuration
*********************

The |NCS| samples are set up to build all related images as one solution, starting from the parent image.
This is referred to as a *multi-image build*.

When building the parent image, you can configure how the child image should be handled:

* Build the child image from the source and include it with the parent image.
  This is the default setting.
* Use a prebuilt HEX file of the child image and include it with the parent image.
* Ignore the child image.

When building the child image from the source or using a prebuilt HEX file, the build system merges the HEX files of the parent and child image together, so that they can easily be programmed in one single step.
This means that you can enable and integrate an additional image just by using the default configuration.

To change the default configuration and configure how a child image is handled, locate the BUILD_STRATEGY configuration options for the child image in the parent image configuration.
For example, to use a prebuilt HEX file of the :ref:`secure_partition_manager` instead of building it, select :option:`CONFIG_SPM_BUILD_STRATEGY_USE_HEX_FILE` instead of the default :option:`CONFIG_SPM_BUILD_STRATEGY_FROM_SOURCE`, and specify the HEX file in :option:`CONFIG_SPM_HEX_FILE`.
To ignore an MCUboot child image, select :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_SKIP_BUILD` instead of :option:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE`.

.. _ug_multi_image_defining:

Defining and enabling a child image
***********************************

You can enable existing child images in the |NCS| by enabling the respective modules in the parent image and selecting the desired build strategy.

To turn an application that you have implemented into a child image that can be included in a parent image, you must update the build scripts to enable the child image and add the required configuration options.
You should also know how image-specific variables are disambiguated and what targets of the child images are available.

Updating the build scripts
==========================

To make it possible to enable a child image from a parent image, you must include the child image in the build script.

To do so, place the code in the following example in the cmake tree that is conditional on a configuration option.
In the |NCS|, the code is included in the :file:`CMakeLists.txt` file for the samples, and in the MCUboot repository.

.. code-block:: cmake

   if (CONFIG_SPM)
     add_child_image(
       NAME spm
       SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/spm
       )
   endif()

   if (CONFIG_SECURE_BOOT)
     add_child_image(
       NAME b0
       SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/bootloader
       )
   endif()

   if (CONFIG_BOOTLOADER_MCUBOOT)
      add_child_image(
        NAME mcuboot
        SOURCE_DIR ${MCUBOOT_DIR}/boot/zephyr
        )
   endif()

In this code, ``add_child_image`` registers the child image with the given name and file path and executes the build scripts of the child image.
Note that both the child image's application build scripts and the core build scripts are executed.
The core build scripts might use a different configuration and possibly different devicetree settings.

If you have to execute a child image on a different core, you must specify the namespace for the child image as *domain* when adding the child image.
See the following example:

.. code-block:: cmake

   add_child_image(
      NAME hci_rpmsg
      SOURCE_DIR ${ZEPHYR_BASE}/samples/bluetooth/hci_rpmsg
      DOMAIN CPUNET
      )

A *domain* is well-defined if there is a configuration ``CONFIG_DOMAIN_${DOMAIN}_BOARD`` in Kconfig.

Adding configuration options
============================

When enabling a child image, you must select the build strategy to define how the image should be included.
The following three options are available:

* Build the child image from source along with the parent image - ``<IMAGE_NAME>\_BUILD_STRATEGY_FROM_SOURCE``.
* Merge the specified HEX file of the child image with the parent image - ``<IMAGE_NAME>\_BUILD_STRATEGY_USE_HEX_FILE``, using ``<IMAGE_NAME>\_HEX_FILE`` to specify the HEX file.
* Ignore the child image when building and build only the parent image - ``<IMAGE_NAME>\_BUILD_STRATEGY_SKIP_BUILD``.

.. note::

   Child images that are built with the build strategy ``<IMAGE_NAME>\ _BUILD_STRATEGY_SKIP_BUILD`` or ``<IMAGE_NAME>\ _BUILD_STRATEGY_USE_HEX_FILE`` must define a :ref:`static partition <ug_pm_static_providing>`.

Add these configuration options to the Kconfig file of your child image, replacing ``<IMAGE_NAME>`` with the uppercase name of your child image, as specified in ``add_child_image``.

This can be done by including the :file:`Kconfig.template.build_strategy` template as follows:

.. code-block:: Kconfig

   module=MCUBOOT
   source "${ZEPHYR_NRF_MODULE_DIR}/subsys/partition_manager/Kconfig.template.build_strategy"

.. _ug_multi_image_variables:

Image-specific variables
========================

The child and parent images are executed in different CMake processes and thus have different namespaces.

Variables in the parent image are not propagated to the child image, with the following exceptions:

* Any variable named ``<IMAGE_NAME>\_FOO`` in a parent image is propagated to the child image named ``<IMAGE_NAME>`` as ``FOO``.
* CMake build settings, such as ``BOARD_DIR``, build type, toolchain info, partition manager info, and similar are always passed to child images.

With these two mechanisms, it is possible to set variables in child images from either parent images or the command line, and it is possible to set variables globally across all images.
For example, to change the ``FOO`` variable for the ``bar`` child image and the parent image, specify the CMake command as follows::

   cmake -Dbar_FOO=value -DFOO=value

You can extend the CMake command that is used to create the child images by adding flags to the CMake variable ``EXTRA_MULTI_IMAGE_CMAKE_ARGS``.
For example, add ``--trace-expand`` to that variable to output more debug information.

With ``west``, these configuration variables are passed into CMake by using the ``--`` separator:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -Dmcuboot_CONF_FILE=prj_a.conf -DCONF_FILE=app_prj.conf

It is possible for a project to pass Kconfig configuration files and fragments to child images by placing them in a :file:`child_image` folder in the application source directory.
The listing below describes how leverage this functionality, ``ACI_NAME`` is the name of the child image that the configuration will be applied to.

.. literalinclude:: ../../cmake/multi_image.cmake
    :language: c
    :start-at: It is possible for a sample to use a custom set of Kconfig fragments for a
    :end-before: set(ACI_CONF_DIR ${APPLICATION_SOURCE_DIR}/child_image)

Variables in child images
-------------------------

It is possible to provide configuration settings for child images, either as individual settings or using Kconfig fragments.
Each child image is referenced using its image name.

The following example sets the configuration option ``CONFIG_FOO=val`` in the child image ``bar``:

  .. parsed-literal::
    :class: highlight

     cmake -D*bar*_CONFIG_FOO=val [...]

You can add a Kconfig fragment to the child image default configuration in a similar way.
The following example adds an extra Kconfig fragment ``baz.conf`` to ``bar``:

  .. parsed-literal::
    :class: highlight

     cmake -D*bar*_OVERLAY_CONFIG=*baz.conf* [...]

It is also possible to provide a custom configuration file as a replacement for the default Kconfig file for the child image.
The following example uses the custom configuration file ``myfile.conf`` when building ``bar``:

  .. parsed-literal::
    :class: highlight

     cmake -D*bar*_CONF_FILE=*myfile.conf* [...]

If your application includes multiple child images, then you can combine all the above as follows:

* Setting ``CONFIG_FOO=val`` in main application.
* Adding a Kconfig fragment ``baz.conf`` to ``bar`` child image, using ``-Dbar_OVERLAY_CONFIG=baz.conf``.
* Using ``myfile.conf`` as configuration for the ``quz`` child image, using ``-Dquz_CONF_FILE=myfile.conf``.

  .. parsed-literal::
    :class: highlight

     cmake -DCONFIG_FOO=val -D*bar*_OVERLAY_CONFIG=*baz.conf* -Dquz_CONF_FILE=*myfile.conf* [...]

See :ref:`ug_bootloader` for more details.

.. note::

   The build system will grab the overlay or configuration file specified in a CMake argument relative to that image's application directory.
   For example, the build system would use :file:`nrf/samples/bootloader/overlay-minimal-size.conf` when building with the ``-Db0_OVERLAY_CONFIG_FILE=overlay-minimal-size.conf`` option, whereas ``-DOVERLAY_CONFIG=...`` would grab the overlay from the main application's directory, such as :file:`zephyr/samples/hello_world`.

Child image targets
===================

You can indirectly invoke a selection of child image targets from the parent image.
Currently, the child targets that can be invoked from the parent targets are ``menuconfig``, ``guiconfig``, and any targets listed in ``EXTRA_KCONFIG_TARGETS``.

To disambiguate targets, the same prefix convention used for variables is also used here.
This means that to run menuconfig, for example, you invoke the ``menuconfig`` target to configure the parent image and ``mcuboot_menuconfig`` to configure the MCUboot child image.

You can also invoke any child target directly from its build directory.
Child build directories are located at the root of the parent's build directory.

Controlling the build process
=============================

The child image is built using CMake's build command ``cmake --build``.
This mechanism allows additional control of the build process through CMake.

CMake options
-------------

The following CMake options are propagated from the CMake command of the parent image to the CMake command of the child image:

* ``CMAKE_BUILD_TYPE``
* ``CMAKE_VERBOSE_MAKEFILE``

You can add other CMake options to a specific child image in the same way as you can set :ref:`Image-specific variables <ug_multi_image_variables>`.
For example, add ``-Dmcuboot_CMAKE_VERBOSE_MAKEFILE`` to the parent's CMake command to build the ``mcuboot`` child image with verbose output.

To enable additional debug information for the multi-image build command, set the CMake option ``MULTI_IMAGE_DEBUG_MAKEFILE`` to the desired debug mode.
For example, add ``-DMULTI_IMAGE_DEBUG_MAKEFILE=explain`` to log the reasons why a command was executed.

See :ref:`cmake_options` for instructions on how to specify these CMake options for the build.

CMake environment variables
---------------------------

Unlike CMake options, CMake environment variables allow you to control the build process without re-invoking CMake.

You can use the CMake environment variables `VERBOSE`_ and `CMAKE_BUILD_PARALLEL_LEVEL`_ to control the verbosity and the number of parallel jobs for a build.

When using |SES|, you must set these environment variables before starting SES, and they will apply only to the build of the child images.
On the command line, you must set them before invoking ``west``, and they will apply to both the parent image and the child images.
For example, to build with verbose output and one parallel job, use the following commands (where *build_target* is the target for the board for which you are building):

* Linux/macOS:

     .. parsed-literal::
        :class: highlight

        $ VERBOSE=True CMAKE_BUILD_PARALLEL_LEVEL=1 west build -b *build_target*

* Windows:

     .. parsed-literal::
        :class: highlight

        > set VERBOSE=True && set CMAKE_BUILD_PARALLEL_LEVEL=1 && west build -b *build_target*

Memory placement
****************

In a multi-image build, all images must be placed in memory so that they do not overlap.
The flash memory start address for each image must be specified by, for example, :option:`CONFIG_FLASH_LOAD_OFFSET`.

Hardcoding the image locations like this works fine for simple use cases like a bootloader that prepares a device, where there are no further requirements on where in memory each image must be placed.
However, more advanced use cases require a memory layout where images are located in a specific order relative to one another.

The |NCS| provides a Python tool that allows you to specify this kind of relative placement or even a static placement based on start addresses and sizes for the different images.

See :ref:`partition_manager` for more information about how to set up your partitions and configure your build system.
