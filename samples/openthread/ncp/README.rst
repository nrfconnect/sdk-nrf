.. _ot_ncp_sample:

Thread: NCP
###########

The Thread NCP sample demonstrates the usage of OpenThread Network Co-Processor architecture inside the Zephyr environment.

The sample is based on Zephyr's :ref:`zephyr:ncp-sample` sample.
However, it customizes Zephyr's sample to the NCS requirements (for example, by increasing the stack size dedicated for the user application), and also extends it with several features:

* Increased mbedTLS heap size.
* Lowered main stack size to increase user application space.
* No obsolete configuration options.
* Vendor hooks for NCP allowing User to extend handled properties by its own, customized functionalities.

Overview
********

The sample demonstrates using an NCP target on the MCU to communicate with Userspace WPAN Network Daemon (wpantund) on Unix-like operating system.
According to the NCP architecture, the MCU part needs to cooperate with user higher layer process to establish the complete full stack application.
The sample shows how to set connection between NCP and wpantund.

This sample has Commissioner, Joiner, and Border Router roles enabled by default.
Additionally, it comes with the following functionalities enabled:

* :option:`CONFIG_OPENTHREAD_BORDER_AGENT` - Enables Border Agent support.
* :option:`CONFIG_OPENTHREAD_DHCP6_SERVER` - Enables DHCPv6 server capability.

.. _ot_ncp_sample_features:

Additional features for NCP sample
==================================

Besides its basic functionalities, this sample supports additional features, which can be turned on or off independently.
These features are defined as overlays to the main project configuration file.

The following features are available in the NCP sample:

* :file:`overlay-vendor_hook.conf` - Enables vendor hooks feature support.

Vendor hooks
------------

The vendor hook feature allows you to define your own commands and properties for the Spinel protocol, and extend the standard set used in communication with NCP.
Thanks to this feature, you can add new custom functionalities and manage them from host device by using serial interface - in the same way as the default functionalities.

For more detailed information about the vendor hooks feature and host device configuration, see :ref:`ug_thread_vendor_hooks`.
For information about how to enable the vendor hook feature for this sample, see `Enabling vendor hook feature`_.

Requirements
************

The sample supports the following development kits for testing the network status:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set8_start
   :end-before: set8_end

To test the sample, you need at least one development kit NCP board.
Additional NCP boards can be used for the :ref:`optional testing of network joining <ot_ncp_sample_testing_more_boards>`.

Moreover, the sample requires a Userspace higher layer process running on user's device in order to communicate with the MCU NCP part.
This sample uses `wpantund`_ as reference.

User interface
**************

All the interactions with the application are handled using serial communication.

For the interaction with the application, this sample uses :ref:`ug_thread_tools_wpantund` process with ``wpanctl`` commands.
It is also possible to communicate with NCP board using `PySpinel`_ commands.

You can use your own application instead of wpantund and PySpinel provided that it supports the spinel communication protocol.

Sample by default reconfigures baudrate 1000000 bit/s for serial communication.

Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/ncp`

|enable_thread_before_testing|

.. include:: /includes/build_and_run.txt

.. _ot_ncp_sample_features_enabling:

Enabling additional features
============================

When building the sample, you can enable :ref:`additional sample features <ot_ncp_sample_features>`.
To do this, modify :makevar:`OVERLAY_CONFIG`.
For example, to enable vendor hooks, set :file:`overlay-vendor_hook.conf`.
See :ref:`cmake_options` for instructions on how to add this option.

For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

Enabling vendor hook feature
----------------------------

For this sample, handling of extension commands and properties is done through the vendor hook :file:`.cpp` file, which is dynamically attached to the NCP component during the compilation.

To enable the feature:

1. Provide the implementation of this file.
#. Insert information about the file location in the ``CONFIG_OPENTHREAD_NCP_VENDOR_HOOK_SOURCE`` field.
   This field is located in the overlay configuration file (see :file:`overlay-vendor_hook.conf`).
   The inserted path must be relative to the NCP sample directory.

The NCP sample provides the vendor hook :file:`user_vendor_hook.cpp` file in the :file:`src` directory that demonstrates the proposed implementation of handler methods.
You can either:

* Use the provided :file:`user_vendor_hook.cpp` file.
* Provide your own implementation and replace the ``CONFIG_OPENTHREAD_NCP_VENDOR_HOOK_SOURCE`` option value in the overlay file with the path to your file.

For information about how to test the vendor hook feature, see :ref:`ug_thread_vendor_hooks_testing`.

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

1. Connect the NCP board's SEGGER J-Link USB port to the PC USB port with an USB cable.
#. Get the board's serial port name (e.g. /dev/ttyACM0).
#. Run and configure wpantund and wpanctl as described in :ref:`ug_thread_tools_wpantund_configuring`.
#. In the wpanctl shell, run the following command to check the NCP board state:

   .. code-block:: console

      wpanctl:leader_interface> status

   The output will look similar to the following:

   .. code-block:: console

      leader_interface => [
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

      wpanctl:leader_interface> form "My_OpenThread_network"

   The output will look similar to the following:

   .. code-block:: console

      Forming WPAN "New_network" as node type "router"
      Successfully formed!

   This output means that the network was formed successfully.
#. In the wpanctl shell, run the status command again to see that "My_OpenThread_network" was formed by NCP:

   .. code-block:: console

      wpanctl:leader_interface> status

The final output will be similar to the following:

.. code-block:: console

   leader_interface => [
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

.. _ot_ncp_sample_testing_more_boards:

Testing network joining with more NCP boards
--------------------------------------------

Optionally, if you are using more than one NCP board, you can test the network joining process by completing the following steps:

1. Connect the second NCP board's SEGGER J-Link USB port to the PC USB port with an USB cable.
#. Get the board's serial port name.
#. Open a shell and run another wpantund process for the second board by using the following command:

   .. code-block:: console

      wpantund -I <network_interface_name_board2> -s <serial_port_name_board2> -b <baudrate>

   For `baudrate`, use value 1000000.
   For `serial_port_name_board2`, use the value from the previous step.
   For `network_interface_name_board2`, use a name of your choice.
   In this testing procedure, this will be `joiner_interface`.
#. Open another shell and run another wpanctl process for the second board by using following command:

   .. code-block:: console

      wpanctl -I joiner_interface

#. In the wpanctl shell, run the following command to check the NCP board state:

   .. code-block:: console

      wpanctl:joiner_interface> status

   The output will look similar to the following:

   .. code-block:: console

      joiner_interface => [
         "NCP:State" => "offline"
         "Daemon:Enabled" => true
         "NCP:Version" => "OPENTHREAD/gde3f05d8; NONE; Jul  7 2020 10:04:51"
         "Daemon:Version" => "0.08.00d (0.07.01-343-g3f10844; Jul  2 2020 09:07:40)"
         "Config:NCP:DriverName" => "spinel"
         "NCP:HardwareAddress" => [E8F947748F493141]
      ]

   This output means that NCP is offline.
#. In the wpanctl shell of the first NCP board, run the following command to get the network key from the leader NCP board:

   .. code-block:: console

      wpanctl:leader_interface> get Network:Key

   The output will look similar to the following:

   .. code-block:: console

      Network:Key = [2429EFAF21421AE3CB30B9204016EDC9]

#. Copy the network key form the output and set it on the second (joiner) NCP board by running the following command in the second board's wpanctl shell:

   .. code-block:: console

      wpanctl:joiner_interface> set Network:Key 2429EFAF21421AE3CB30B9204016EDC9

#. In the second board's wpanctl shell, run the following command to scan your neighborhood and find the network formed with the leader NCP board:

   .. code-block:: console

      wpanctl:joiner_interface> scan

   The output will look similar to the following:

   .. code-block:: console

      | Joinable | NetworkName             | PAN ID | Ch | XPanID           | HWAddr           | RSSI
    --+----------+-------------------------+--------+----+------------------+------------------+------
    1 |       NO | "OpenThread"            | 0xABCD | 11 | DEAD00BEEF00CAFE | 621757E184CEF8E5 |  -82
    2 |       NO | "My_OpenThread_network" | 0xF54B | 13 | 77969855F947758D | 62AAC622CB3ACD9F |  -34

   The first column is the network ID number.
   For the network formed for this testing procedure, the ID equals ``2``.
#. In the second board's wpanctl shell, run the following command with the network ID as variable to join your joiner NCP board to the network:

   .. code-block:: console

      wpanctl:joiner_interface> join 2

   The output will look similar to the following:

   .. code-block:: console

      Joining WPAN "My_OpenThread_network" as node type "end-device", channel:13, panid:0xF54B, xpanid:0x77969855F947758D [scanned network index 2]
      Successfully Joined!

This output means that the joiner board node has successfully joined the network.

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:thread_protocol_interface`

* :ref:`zephyr:logging_api`:

  * ``include/logging/log.h``

----

Copyright disclaimer
    |Google_CCLicense|
