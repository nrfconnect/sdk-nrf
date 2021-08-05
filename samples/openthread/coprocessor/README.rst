.. _ot_coprocessor_sample:

Thread: Co-processor
####################

.. contents::
   :local:
   :depth: 2

The :ref:`Thread <ug_thread>` Co-processor sample demonstrates how to implement OpenThread's :ref:`thread_architectures_designs_cp` inside the Zephyr environment.
Depending on the configuration, the sample uses the :ref:`thread_architectures_designs_cp_ncp` architecture or :ref:`thread_architectures_designs_cp_rcp` architecture.

The sample is based on Zephyr's :ref:`zephyr:coprocessor-sample` sample.
However, it customizes Zephyr's sample to the |NCS| requirements (for example, by increasing the stack size dedicated for the user application), and also extends it with several features:

* Increased mbedTLS heap size.
* Lowered main stack size to increase user application space.
* No obsolete configuration options.
* Vendor hooks for co-processor architecture allowing users to extend handled properties by their own, customized functionalities.

This sample supports optional :ref:`ot_coprocessor_sample_vendor_hook_extension` and :ref:`logging extension <ot_coprocessor_sample_logging>`, which can be turned on or off independently.
See :ref:`ot_coprocessor_sample_config_files` for details.

Overview
********

The sample demonstrates using a co-processor target on the MCU to communicate with Userspace WPAN Network Daemon (`wpantund`_) on Unix-like operating system.
According to the co-processor architecture, the MCU part must cooperate with user higher layer process to establish the complete full stack application.
The sample shows how to set up the connection between the co-processor and wpantund.

This sample comes with the :ref:`full set of OpenThread functionalities <thread_ug_feature_sets>` enabled (:kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER`).

.. _ot_coprocessor_sample_vendor_hook_extension:

Vendor hooks extension
======================

The vendor hook feature extension allows you to define your own commands and properties for the `Spinel protocol`_, and extend the standard set used in communication with the co-processor.
Thanks to this feature, you can add new custom functionalities and manage them from host device by using serial interface - in the same way as the default functionalities.

For more detailed information about the vendor hooks feature and host device configuration, see :ref:`ug_thread_vendor_hooks`.
For information about how to enable the vendor hook feature for this sample, see :ref:`ot_coprocessor_sample_features_enabling_hooks`.

.. _ot_coprocessor_sample_logging:

Logging extension
=================

This sample by default uses :ref:`Spinel logging backend <ug_logging_backends_spinel>`, which allows sending log messages to the host device using the Spinel protocol.
This feature is very useful, because it does not require having separate interfaces to communicate with the co-processor through the Spinel protocol and collect log messages.
Moreover, selecting the Spinel logging backend (by setting :kconfig:`CONFIG_LOG_BACKEND_SPINEL`) does not exclude using another backend like UART or RTT at the same time.

By default, the log levels for all modules are set to critical to not engage the microprocessor in unnecessary activities.
To make the solution flexible, you can change independently the log levels for your modules, for the whole Zephyr system, and for OpenThread.
Use the :file:`overlay-logging.conf` overlay file as reference for this purpose.

FEM support
===========

.. |fem_file_path| replace:: :file:`samples/openthread/common`

.. include:: /includes/sample_fem_support.txt

Requirements
************

The sample supports the following development kits for testing the network status:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf21540dk_nrf52840

To test the sample, you need at least one development kit.
Additional development kits programmed with the Co-processor sample can be used for the :ref:`optional testing of network joining <ot_coprocessor_sample_testing_more_boards>`.

Moreover, the sample requires a Userspace higher layer process running on user's device in order to communicate with the MCU co-processor part.
This sample uses `wpantund`_ as reference.

User interface
**************

All the interactions with the application are handled using serial communication.

You can interact with the sample through :ref:`ug_thread_tools_wpantund`, using ``wpanctl`` commands.
If you use the RCP architecture (see :kconfig:`CONFIG_OPENTHREAD_COPROCESSOR_RCP`), you can alternatively use ``ot-daemon`` or ``ot-cli`` with commands listed in `OpenThread CLI Reference`_.
See :ref:`ug_thread_tools_ot_apps` for more information.

Both NCP and RCP support communication with the kit using `Pyspinel`_ commands.

You can use your own application instead of the ones listed above, provided that it supports the Spinel communication protocol.

.. note::
    |thread_hwfc_enabled|
    In addition, the Co-processor sample by default reconfigures the baud rate to 1000000 bit/s.

Configuration
*************

|config|

Check and configure the following library options that are used by the sample:

* :kconfig:`CONFIG_OPENTHREAD_COPROCESSOR_NCP` - Selects the NCP architecture for the sample.
  This is the default.
* :kconfig:`CONFIG_OPENTHREAD_COPROCESSOR_RCP` - Selects the RCP architecture for the sample.

.. _ot_coprocessor_sample_config_files:

Configuration files
===================

The sample provides predefined configuration files for typical use cases, and to activate sample extensions.
You can find the configuration files in the root directory of the sample.

Specify the corresponding file names in the :makevar:`OVERLAY_CONFIG` option when building.
See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

The following configuration files are available:

* :file:`overlay-vendor_hook.conf` - Enables the vendor hooks extension.
  This file enables the vendor hooks feature and specifies the source file to use.
  See :ref:`ot_coprocessor_sample_features_enabling_hooks` for more information.
* :file:`overlay-logging.conf` - Enables the logging extension.
  This file configures different log levels for the sample, the Zephyr system, and OpenThread.
* :file:`overlay-rcp.conf` - Enables the RCP architecture.
  This file configures the sample to use the RCP architecture instead of the NCP architecture.
* :file:`overlay-minimal_rcp.conf` - Enables a minimal configuration that reduces the code size and RAM usage.
  This file enables the RCP architecture with basic functionality and optimizes stacks and buffer sizes.
  For more information, see :ref:`app_memory`.
* :file:`overlay-usb.conf` - Enables emulating a serial port over USB for Spinel communication with the host.

Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/coprocessor`

|enable_thread_before_testing|

.. include:: /includes/build_and_run.txt

.. _ot_coprocessor_sample_features_enabling_hooks:

Activating the vendor hook feature
==================================

Handling of extension commands and properties is done through the vendor hook :file:`.cpp` file, which is attached to the Co-processor sample during the linking.

To enable the feature:

1. Provide the implementation of this file.
#. Insert information about the file location in the ``CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE`` field.
   This field is located in the overlay configuration file (see :file:`overlay-vendor_hook.conf`).
   The inserted path must be relative to the Co-processor sample directory.

The Co-processor sample provides the vendor hook :file:`user_vendor_hook.cpp` file in the :file:`src` directory that demonstrates the proposed implementation of handler methods.
You can either:

* Use the provided :file:`user_vendor_hook.cpp` file.
* Provide your own implementation and replace the ``CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE`` option value in the overlay file with the path to your file.

For information about how to test the vendor hook feature, see :ref:`ug_thread_vendor_hooks_testing`.

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

1. Connect the development kit's SEGGER J-Link USB port to the PC USB port with an USB cable.
#. Get the kit's serial port name (for example, :file:`/dev/ttyACM0`).
#. Run and configure wpantund and wpanctl as described in :ref:`ug_thread_tools_wpantund_configuring`.
#. In the wpanctl shell, run the following command to check the kit state:

   .. code-block:: console

      wpanctl:leader_if> status

   The output will look similar to the following:

   .. code-block:: console

      leader_if => [
        "NCP:State" => "offline"
        "Daemon:Enabled" => true
        "NCP:Version" => "OPENTHREAD/gde3f05d8; NONE; Jul  7 2020 10:04:51"
        "Daemon:Version" => "0.08.00d (0.07.01-343-g3f10844; Jul  2 2020 09:07:40)"
        "Config:NCP:DriverName" => "spinel"
        "NCP:HardwareAddress" => [E8F947748F493141]
      ]

   This output means that NCP is offline.
#. In the wpanctl shell, run the following command to set up a Thread network:

   .. code-block:: console

      wpanctl:leader_if> form "My_OpenThread_network"

   The output will look similar to the following:

   .. code-block:: console

      Forming WPAN "New_network" as node type "router"
      Successfully formed!

   This output means that the network was formed successfully.
#. In the wpanctl shell, run the status command again to see that "My_OpenThread_network" was formed by NCP:

   .. code-block:: console

      wpanctl:leader_if> status

The final output will be similar to the following:

.. code-block:: console

   leader_if => [
     "NCP:State" => "associated"
     "Daemon:Enabled" => true
     "NCP:Version" => "OPENTHREAD/gde3f05d8; NONE; Jul  7 2020 10:04:51"
     "Daemon:Version" => "0.08.00d (0.07.01-343-g3f10844; Jul  2 2020 09:07:40)"
     "Config:NCP:DriverName" => "spinel"
     "NCP:HardwareAddress" => [E8F947748F493141]
     "NCP:Channel" => 26
     "Network:NodeType" => "leader"
     "Network:Name" => "My_OpenThread_network"
     "Network:XPANID" => 0x048CA9024CD7D40F
     "Network:PANID" => 0xDB92
     "IPv6:MeshLocalAddress" => "fd04:8ca9:24c:0:ebb8:4ef3:d96:c4bd"
     "IPv6:MeshLocalPrefix" => "fd04:8ca9:24c::/64"
     "com.nestlabs.internal:Network:AllowingJoin" => false
   ]

This output means that you have successfully formed the Thread network.

.. _ot_coprocessor_sample_testing_more_boards:

Testing network joining with more kits
--------------------------------------

Optionally, if you are using more than one kit, you can test the network joining process by completing the following steps:

1. Connect the second kit's SEGGER J-Link USB port to the PC USB port with an USB cable.
#. Get the kit's serial port name.
#. Open a shell and run another wpantund process for the second kit as described in :ref:`ug_thread_tools_wpantund_configuring`.
   Make sure to use the correct serial port name for the second kit (for example, :file:`/dev/ACM1`) and a different network interface name (for example, ``joiner_if``).
#. Open another shell and run another wpanctl process for the second kit by using following command:

   .. code-block:: console

      wpanctl -I joiner_if

#. In the wpanctl shell, run the following command to check the kit state:

   .. code-block:: console

      wpanctl:joiner_if> status

   The output will look similar to the following:

   .. code-block:: console

      joiner_if => [
         "NCP:State" => "offline"
         "Daemon:Enabled" => true
         "NCP:Version" => "OPENTHREAD/gde3f05d8; NONE; Jul  7 2020 10:04:51"
         "Daemon:Version" => "0.08.00d (0.07.01-343-g3f10844; Jul  2 2020 09:07:40)"
         "Config:NCP:DriverName" => "spinel"
         "NCP:HardwareAddress" => [E8F947748F493141]
      ]

   This output means that NCP is offline.
#. In the wpanctl shell of the first kit, run the following command to get the network key from the leader kit:

   .. code-block:: console

      wpanctl:leader_if> get Network:Key

   The output will look similar to the following:

   .. code-block:: console

      Network:Key = [2429EFAF21421AE3CB30B9204016EDC9]

#. Copy the network key form the output and set it on the second (joiner) kit by running the following command in the second kit's wpanctl shell:

   .. code-block:: console

      wpanctl:joiner_if> set Network:Key 2429EFAF21421AE3CB30B9204016EDC9

#. In the second kit's wpanctl shell, run the following command to scan your neighborhood and find the network formed with the leader kit:

   .. code-block:: console

      wpanctl:joiner_if> scan

   The output will look similar to the following:

   .. code-block:: console

      | Joinable | NetworkName             | PAN ID | Ch | XPanID           | HWAddr           | RSSI
    --+----------+-------------------------+--------+----+------------------+------------------+------
    1 |       NO | "OpenThread"            | 0xABCD | 11 | DEAD00BEEF00CAFE | 621757E184CEF8E5 |  -82
    2 |       NO | "My_OpenThread_network" | 0xF54B | 13 | 77969855F947758D | 62AAC622CB3ACD9F |  -34

   The first column is the network ID number.
   For the network formed for this testing procedure, the ID equals ``2``.
#. In the second kit's wpanctl shell, run the following command with the network ID as variable to join your joiner kit to the network:

   .. code-block:: console

      wpanctl:joiner_if> join 2

   The output will look similar to the following:

   .. code-block:: console

      Joining WPAN "My_OpenThread_network" as node type "end-device", channel:13, panid:0xF54B, xpanid:0x77969855F947758D [scanned network index 2]
      Successfully Joined!

This output means that the joiner kit node has successfully joined the network.

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:thread_protocol_interface`

* :ref:`zephyr:logging_api`:

  * ``include/logging/log.h``
