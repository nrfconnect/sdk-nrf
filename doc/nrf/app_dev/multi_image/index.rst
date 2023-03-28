.. _ug_multi_image:

Multi-image builds
##################

.. contents::
   :local:
   :depth: 2

The firmware programmed to a device can be composed of either one application or several separate images.
In the latter case, one of the images (the *parent image*) requires one or more other images (the *child images*) to be present.
The child image then *chain-loads*, or *boots*, the parent image, which could also be a child image to another parent image, and boots that one.

The most common use cases for builds composed of multiple images are applications that require a bootloader to be present or applications for multi-core CPUs.

.. _ug_multi_image_what_are_images:

What image files are
********************

.. include:: ../build_and_config_system/index.rst
   :start-after: output_build_files_info_start
   :end-before: output_build_files_info_end

Using multiple images has the following advantages:

* You can run the linker multiple times and partition the final firmware into several regions.
  This partitioning is often useful for bootloaders.
* Since there is a symbol table for each image, the same symbol names can exist multiple times in the final firmware.
  This is useful for bootloader images, which can require their own copy of a library that the application uses, but in a different version or configuration.
* In multi-core builds, the build configuration of a child image in a separate core can be made known to the parent image.

.. include:: ../build_and_config_system/index.rst
   :start-after: output_build_files_table_start
   :end-before: output_build_files_table_end

For more information on other build output files refer to :ref:`app_build_system` page.

.. _ug_multi_image_when_to_use_images:

When to use multiple images
***************************

In the |NCS|, multiple images are required in the following scenarios:

First-stage and second-stage bootloaders
   The first-stage bootloader establishes a root of trust by verifying the next step in the boot sequence.
   This first-stage bootloader is immutable, which means it cannot be updated or deleted.
   If a second-stage bootloader is present, then the first-stage bootloader is responsible for booting and updating the second-stage bootloader, which in turn is responsible for booting and updating the application.
   As such, the first-stage bootloader, the second-stage bootloader, and the application must be located in different images.
   In this scenario, the application is the parent image and the bootloaders are two separate child images.

   See :ref:`ug_bootloader` and :ref:`ug_bootloader_adding` for more information.

nRF5340 development kit support
   The nRF5340 development kit (DK) contains two separate processors: a network core and an application core.
   When programming an application for the nRF5340 DK, the application must be divided into at least two images, one for each core.

   See :ref:`ug_nrf5340` for more information.

nRF5340 Audio development kit support
   The nRF5340 Audio development kit (DK) is based on the nRF5340 development kit and also contains two separate processors.
   When programming an application for the nRF5340 Audio DK, the application core image is built from a combination of different configuration files.
   The network core image is programmed with an application-specific precompiled Bluetooth Low Energy Controller binary file that contains the LE Audio Controller Subsystem for nRF53.

   See the :ref:`nrf53_audio_app` application documentation for more information.

.. _ug_multi_image_default_config:

Default configuration
*********************

The |NCS| samples are set up to build all related images as one solution, starting from the parent image.
This is referred to as a *multi-image build*.

When building the parent image, you can configure how the child image should be handled:

* Build the child image from the source and include it with the parent image.
  This is the default setting.
* Use a prebuilt HEX file of the child image and include it with the parent image.
* Ignore the child image.

When building the child image from the source or using a prebuilt HEX file, the build system merges the HEX files of both the parent and child image, so that they can be programmed in one single step.
This means that you can enable and integrate an additional image just by using the default configuration.

To change the default configuration and configure how a child image is handled, locate the BUILD_STRATEGY configuration options for the child image in the parent image configuration.
For example, to use a prebuilt HEX file of the MCUboot instead of building it, select :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE` instead of the default :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE`, and specify the HEX file in :kconfig:option:`CONFIG_MCUBOOT_HEX_FILE`.
To ignore an MCUboot child image, select :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_SKIP_BUILD` instead of :kconfig:option:`CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE`.

.. _ug_multi_image_defining:

Defining and enabling a child image
***********************************

You can enable existing child images in the |NCS| by enabling the respective modules in the parent image and selecting the desired build strategy.

To turn an application that you have implemented into a child image that can be included in a parent image, you must update the build scripts to enable the child image and add the required configuration options.
You should also know how image-specific variables are disambiguated and what targets of the child images are available.

.. _ug_multi_image_build_scripts:

Updating the build scripts
==========================

To make it possible to enable a child image from a parent image, you must include the child image in the build script.
If you need to perform this operation out-of-tree (that is, without modifying |NCS| code), or from the top-level CMakeLists.txt in your sample, see :ref:`ug_multi_image_add_child_image_oot`.

To do so, place the code from the following example in the CMake tree that is conditional on a configuration option.
In the |NCS|, the code is included in the :file:`CMakeLists.txt` file for the samples, and in the MCUboot repository.

.. code-block:: cmake

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

A *domain* is well-defined if there is the ``CONFIG_DOMAIN_${DOMAIN}_BOARD`` configuration option in Kconfig.

.. _ug_multi_image_add_child_image_oot:

Adding a child image using Zephyr modules
=========================================

Any call to ``add_child_image`` must be done *after* :file:`nrf/cmake/extensions.cmake` is invoked, but *before* :file:`multi_image.cmake` is invoked.
In some scenarios, this is not possible without modifying the |NCS| build code, for example, from top-level sample files and project :file:`CMakeLists.txt` files.

To avoid this issue, use the *Modules* mechanism provided by the Zephyr build system.
The following example shows how to add the required module from a top-level sample :file:`CMakeLists.txt`.

.. literalinclude:: ../../../../samples/multicore/hello_world/CMakeLists.txt
    :language: cmake
    :start-at: cmake_minimum_required
    :end-at: target_sources

A :file:`zephyr/module.yml` file is needed at the base of the added module.
The following example specifies only the path to the :file:`CMakeLists.txt` of the new module.
See :ref:`modules` for more details.

.. literalinclude:: ../../../../samples/multicore/hello_world/zephyr/module.yml
    :language: yaml

The :file:`CMakeLists.txt` located in the directory pointed to by :file:`zephyr/module.yml` will be invoked when ``add_child_image`` can be invoked.

.. literalinclude:: ../../../../samples/multicore/hello_world/aci/CMakeLists.txt
    :language: cmake
    :start-at: add_child_image
    :end-before: endif()


Adding configuration options
============================

When enabling a child image, you must select the build strategy to define how the image should be included.
The following three options are available:

* ``<IMAGE_NAME>_BUILD_STRATEGY_FROM_SOURCE`` - Build the child image from source along with the parent image.
* ``<IMAGE_NAME>_BUILD_STRATEGY_USE_HEX_FILE`` - Merge the specified HEX file of the child image with the parent image, using ``<IMAGE_NAME>_HEX_FILE`` to specify the HEX file.
* ``<IMAGE_NAME>_BUILD_STRATEGY_SKIP_BUILD`` - Ignore the child image when building and build only the parent image.

.. note::

   Child images that are built with the build strategy ``<IMAGE_NAME>_BUILD_STRATEGY_SKIP_BUILD`` or ``<IMAGE_NAME>_BUILD_STRATEGY_USE_HEX_FILE`` must define a :ref:`static partition <ug_pm_static_providing>`.

Add these configuration options to the Kconfig file of your child image, replacing ``<IMAGE_NAME>`` with the uppercase name of your child image that is specified in ``add_child_image``.
To do this, include the :file:`Kconfig.template.build_strategy` template as follows:

.. code-block:: Kconfig

   module=MCUBOOT
   source "${ZEPHYR_NRF_MODULE_DIR}/subsys/partition_manager/Kconfig.template.build_strategy"

.. _ug_multi_image_variables:

Image-specific variables
========================

The child and parent images are executed in different CMake processes and thus have different namespaces.

Variables in the parent image are not propagated to the child image, with the following exceptions:

* Any variable named ``<IMAGE_NAME>_VARIABLEONE`` in a parent image is propagated to the child image named ``<IMAGE_NAME>`` as ``VARIABLEONE``.
* CMake build settings, such as ``BOARD_DIR``, build type, toolchain info, partition manager info, and similar are always passed to child images.

With these two mechanisms, you can set variables in child images from either parent images or the command line, and you can also set variables globally across all images.
For example, to change the ``VARIABLEONE`` variable for the ``childimageone`` child image and the parent image, specify the CMake command as follows:

  .. parsed-literal::
    :class: highlight

     cmake -D\ *childimageone*\_\ *VARIABLEONE*\=value -D\ *VARIABLEONE*\=value

You can extend the CMake command used to create the child images by adding flags to the CMake variable ``EXTRA_MULTI_IMAGE_CMAKE_ARGS``.
For example, add ``--trace-expand`` to that variable to output more debug information.

With west, you can pass these configuration variables into CMake by using the ``--`` separator:

.. code-block:: console

   west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- \
   -Dmcuboot_CONF_FILE=prj_a.conf \
   -DCONF_FILE=app_prj.conf

You can make a project pass Kconfig configuration files, fragments, and devicetree overlays to child images by placing them in the :file:`child_image` folder in the application source directory.
The listing below describes how to leverage this functionality, where ``ACI_NAME`` is the name of the child image to which the configuration will be applied.

.. literalinclude:: ../../../../cmake/multi_image.cmake
    :language: c
    :start-at: It is possible for a sample to use a custom set of Kconfig fragments for a
    :end-before: set(ACI_CONF_DIR ${APPLICATION_CONFIG_DIR}/child_image)

Variables in child images
-------------------------

It is possible to provide configuration settings for child images, either as individual settings or using Kconfig fragments.
Each child image is referenced using its image name.

The following example sets the configuration option ``CONFIG_VARIABLEONE=val`` in the child image ``childimageone``:

  .. parsed-literal::
    :class: highlight

     cmake -D\ *childimageone*\_\ *CONFIG_VARIABLEONE=val*\ [...]

You can add a Kconfig fragment to the child image default configuration in a similar way.
The following example adds an extra Kconfig fragment ``extrafragment.conf`` to ``childimageone``:

  .. parsed-literal::
    :class: highlight

     cmake -D\ *childimageone*\_OVERLAY_CONFIG=\ *extrafragment.conf*\ [...]

It is also possible to provide a custom configuration file as a replacement for the default Kconfig file for the child image.
The following example uses the custom configuration file ``myfile.conf`` when building ``childimageone``:

  .. parsed-literal::
    :class: highlight

     cmake -D\ *childimageone*\_CONF_FILE=\ *myfile.conf*\ [...]

If your application includes multiple child images, then you can combine all the above as follows:

* Setting ``CONFIG_VARIABLEONE=val`` in the main application.
* Adding a Kconfig fragment ``extrafragment.conf`` to the ``childimageone`` child image, using ``-Dchildimageone_OVERLAY_CONFIG=extrafragment.conf``.
* Using ``myfile.conf`` as configuration for the ``quz`` child image, using ``-Dquz_CONF_FILE=myfile.conf``.

  .. parsed-literal::
    :class: highlight

     cmake -DCONFIG_VARIABLEONE=val -D\ *childimageone*\_OVERLAY_CONFIG=\ *extrafragment.conf*\ -Dquz_CONF_FILE=\ *myfile.conf*\ [...]

See :ref:`ug_bootloader` for more details.

.. note::

   The build system grabs the Kconfig fragment or configuration file specified in a CMake argument relative to that image's application directory.
   For example, the build system uses ``nrf/samples/bootloader/my-fragment.conf`` when building with the ``-Db0_OVERLAY_CONFIG=my-fragment.conf`` option, whereas ``-DOVERLAY_CONFIG=my-fragment.conf`` grabs the fragment from the main application's directory, such as ``zephyr/samples/hello_world/my-fragment.conf``.

You can also merge multiple fragments into the overall configuration for an image by giving a list of Kconfig fragments as a string, separated using ``;``.
The following example shows how to combine ``abc.conf``, Kconfig fragment of the ``childimageone`` child image, with the ``extrafragment.conf`` fragment:

  .. parsed-literal::
    :class: highlight

     cmake -D\ *childimageone*\_OVERLAY_CONFIG='\ *extrafragment.conf*\;\ *abc.conf*\'

When the build system finds the fragment, it outputs their merge during the CMake build output as follows:

.. parsed-literal::
   :class: highlight

   ...
   Merged configuration '\ *extrafragment.conf*\'
   Merged configuration '\ *abc.conf*\'
   ...

Child image devicetree overlays
===============================

You can provide devicetree overlays for a child image using ``*.overlay`` files.

The following example sets the devicetree overlay ``extra.overlay`` to ``childimageone``:

.. parsed-literal::
   :class: highlight

   cmake -D\ *childimageone*\_DTC_OVERLAY_FILE='\ *extra.overlay*\'

The build system does also automatically apply any devicetree overlay located in the ``child_image`` folder and named as follows (where ``ACI_NAME`` is the name of the child image):

* ``child_image/<ACI_NAME>.overlay``
* ``child_image/<ACI_NAME>/<board>.overlay``
* ``child_image/<ACI_NAME>/<board>_<revision>.overlay``
* ``child_image/<ACI_NAME>/boards/<board>.overlay``
* ``child_image/<ACI_NAME>/boards/<board>_<revision>.overlay``

.. note::

   The build system grabs the devicetree overlay files specified in a CMake argument relative to that image's application directory.
   For example, the build system uses ``nrf/samples/bootloader/my-dts.overlay`` when building with the ``-Db0_DTC_OVERLAY_FILE=my-dts.overlay`` option, whereas ``-DDTC_OVERLAY_FILE=my-dts.overlay`` grabs the fragment from the main application's directory, such as ``zephyr/samples/hello_world/my-dts.overlay``.

Child image targets
===================

You can indirectly invoke a selection of child image targets from the parent image.
Currently, the child targets that can be invoked from the parent targets are ``menuconfig``, ``guiconfig``, and any targets listed in ``EXTRA_KCONFIG_TARGETS``.

To disambiguate targets, use the same prefix convention used for variables.
For example, to run menuconfig, invoke the ``menuconfig`` target to configure the parent image and ``mcuboot_menuconfig`` to configure the MCUboot child image.

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

You can use the CMake environment variables `VERBOSE`_ and `CMAKE_BUILD_PARALLEL_LEVEL`_ to control the verbosity and the number of parallel jobs for a build:

When using the command line or |VSC| terminal window, you must set them before invoking west.
They apply to both the parent and child images.
For example, to build with verbose output and one parallel job, use the following command, where *build_target* is the target for the development kit for which you are building:

.. parsed-literal::
   :class: highlight

   west build -b *build_target* -- -DCMAKE_VERBOSE_MAKEFILE=1 -DCMAKE_BUILD_PARALLEL_LEVEL=1

Memory placement
****************

In a multi-image build, all images must be placed in memory so that they do not overlap.
The flash memory start address for each image must be specified by, for example, :kconfig:option:`CONFIG_FLASH_LOAD_OFFSET`.

Hardcoding the image locations like this works fine for simple use cases like a bootloader that prepares a device, where there are no further requirements on where in memory each image must be placed.
However, more advanced use cases require a memory layout where images are located in a specific order relative to one another.

The |NCS| provides a Python tool that allows you to specify this kind of relative placement or even a static placement based on start addresses and sizes for the different images.

See :ref:`partition_manager` for more information about how to set up your partitions and configure your build system.
