.. _ncs-app-dev:

Application development
#######################

.. contents::
   :local:
   :depth: 2

This user guide provides fundamental information on developing a new application based on the |NCS|.

Build and configuration system
******************************

The |NCS| build and configuration system is based on the one from Zephyr, with some additions.

Zephyr's build and configuration system
=======================================

Zephyr's build and configuration system uses the following building blocks as a foundation:

* CMake, the cross-platform build system generator,
* Kconfig, a powerful configuration system also used in the Linux kernel,
* Device Tree, a hardware description language that is used to describe the hardware that the |NCS| is to run on.

Since the build and configuration system used by the |NCS| comes from Zephyr, references to the original Zephyr documentation are provided here in order to avoid duplication.
See the following links for information about the different building blocks mentioned above:

* :ref:`zephyr:application` is a complete guide to application development with Zephyr, including the build and configuration system.
* :ref:`zephyr:cmake-details` describes in-depth the usage of CMake for Zephyr-based applications.
* :ref:`zephyr:application-kconfig` contains a guide for Kconfig usage in applications.
* :ref:`zephyr:set-devicetree-overlays` explains how to use Device Tree and its overlays to customize an application's Device Tree.

|NCS| additions
===============

The |NCS| adds some functionality on top of the Zephyr build and configuration system.
Those additions are automatically included into the Zephyr build system using a :ref:`cmake_build_config_package`.

You must be aware of these additions when you start writing your own applications based on this SDK.

* The |NCS| provides an additional :file:`boilerplate.cmake` that is automatically included when using the Zephyr CMake package in the :file:`CMakeLists.txt` file of your application::

    find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

* The |NCS| allows you to :ref:`create custom build type files <gs_modifying_build_types>` instead of using a single :file:`prj.conf` file.
* The |NCS| build system extends Zephyr's with support for multi-image builds.
  You can find out more about these in the :ref:`ug_multi_image` section.
* The |NCS| adds a partition manager, responsible for partitioning the available flash memory.

Custom boards
*************

Defining your own board is a very common step in application development, since applications are typically designed to run on boards that are not directly supported by the |NCS|, given that they are typically custom designs and not available publicly.
In order to define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

.. _memory_footprint_optimization:

Memory footprint optimization
*****************************

When developing an application, ROM and RAM footprint are important factors, especially when the firmware runs on the most resource-constrained devices like nRF52810 or nRF52811.

To reduce the memory footprint, ensure that your application uses the minimum required resources and tune the |NCS| configuration parameters.
Complete the following actions to optimize your application:

* Follow the guides for :ref:`optimizing Zephyr <zephyr:optimizations>`.
  Also see the implementation of the :ref:`zephyr:minimal_sample` sample.
* Analyze stack usage in each thread of your application by using the :ref:`zephyr:thread_analyzer`.
  Reduce the stack sizes where possible.
* Limit or disable debugging features such as logging or asserts.
* Go through each component and subsystem and turn off all features that your application does not use.

The following subsections give more information on how to optimize specific subsystems.


Bluetooth
=========

Complete the following actions to optimize the Bluetooth part of your application:

* Disable features that your application does not use.
  For example, disable the following features:

  * Data Length Update
  * Extended Advertising
  * PHY Update
  * Security Manager Protocol (if no encryption and authentication is needed)
  * GATT Caching
  * GATT Service Changed

* Reduce the stack sizes of the Bluetooth internal threads where possible.
  Use the :ref:`zephyr:thread_analyzer` to analyze the stack usage.

  The following configuration options affect the stack sizes of the Bluetooth threads:

  * :option:`CONFIG_SDC_RX_STACK_SIZE`
  * :option:`CONFIG_BT_RX_STACK_SIZE`
  * :option:`CONFIG_BT_HCI_TX_STACK_SIZE`
  * :option:`CONFIG_MPSL_SIGNAL_STACK_SIZE`

* Reduce the overall number and the sizes of the Bluetooth buffers, based on the expected data traffic in your application.

  The following configuration options affect the Bluetooth buffers:

  * :option:`CONFIG_BT_DISCARDABLE_BUF_COUNT`
  * :option:`CONFIG_BT_DISCARDABLE_BUF_SIZE`
  * :option:`CONFIG_BT_RX_BUF_COUNT`
  * :option:`CONFIG_BT_CONN_TX_MAX`
  * :option:`CONFIG_BT_L2CAP_TX_BUF_COUNT`
  * :option:`CONFIG_BT_ATT_TX_MAX`
  * :option:`CONFIG_BT_CTLR_RX_BUFFERS`
  * :option:`CONFIG_BT_CTLR_TX_BUFFERS`
  * :option:`CONFIG_BT_CTLR_TX_BUFFER_SIZE`

For reference, you can find a minimal footprint configuration of the :ref:`peripheral_lbs` sample in :file:`nrf/samples/bluetooth/peripheral_lbs/minimal.conf`.

Thread
======

Complete the following actions to optimize the Thread part of your application:

* Disable features that your application does not use.
  For example, disable the following features:

  * Asserts
  * Logging
  * Network shell and OpenThread CLI shell support (see :ref:`ug_thread_configuring_additional`)

* :ref:`Configure the OpenThread stack. <ug_thread_configuring_basic_building>`
* :ref:`Select the appropriate OpenThread device type. <thread_ug_device_type>`
* Reduce the stack sizes of the Thread internal threads where possible.
  Use the :ref:`zephyr:thread_analyzer` to analyze the stack usage.

  The following configuration options affect the stack sizes of the Thread threads:

  * :option:`CONFIG_OPENTHREAD_THREAD_STACK_SIZE`
  * :option:`CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE`
  * :option:`CONFIG_NET_RX_STACK_SIZE`
  * :option:`CONFIG_NET_TX_STACK_SIZE`
  * :option:`CONFIG_NET_MGMT_EVENT_STACK_SIZE`
  * :option:`CONFIG_IEEE802154_NRF5_RX_STACK_SIZE`
  * :option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
  * :option:`CONFIG_MPSL_SIGNAL_STACK_SIZE`
  * :option:`CONFIG_SHELL_STACK_SIZE`
  * :option:`CONFIG_IDLE_STACK_SIZE`
  * :option:`CONFIG_MAIN_STACK_SIZE`
  * :option:`CONFIG_ISR_STACK_SIZE`

For reference, you can find minimal footprint configurations for the single protocol and multiprotocol variants of the :ref:`ot_cli_sample` sample in :file:`nrf/samples/openthread/cli/overlay-minimal_*protocol.conf`.
